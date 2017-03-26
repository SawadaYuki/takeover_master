/****************************************************************************
 *   Copyright (C) 2006-2010 by Jason Ansel, Kapil Arya, and Gene Cooperman *
 *   jansel@csail.mit.edu, kapil@ccs.neu.edu, gene@ccs.neu.edu              *
 *                                                                          *
 *   This file is part of the dmtcp/src module of DMTCP (DMTCP:dmtcp/src).  *
 *                                                                          *
 *  DMTCP:dmtcp/src is free software: you can redistribute it and/or        *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP:dmtcp/src is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <arpa/inet.h>

#include "dmtcp.h"
#include "shareddata.h"
#include "util.h"
#include "jsocket.h"
#include "jassert.h"
#include "jfilesystem.h"
#include "jconvert.h"

#include "socketconnection.h"
#include "socketwrappers.h"
#include "kernelbufferdrainer.h"
#include "connectionrewirer.h"

//sawada:start
#include <stdio.h> 
#include <stdlib.h> 
#include <map>
#include "../../../myisrestart.h"
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
//saawada:end

#ifdef REALLY_VERBOSE_CONNECTION_CPP
static bool really_verbose = true;
#else
static bool really_verbose = false;
#endif

// :9/16
extern char procname[100]; // from mtcpinterface.cpp
using namespace std;
map<uint64_t,int32_t> fdsAndHostid;
static int listenfd = 0;
// :end
using namespace dmtcp;

//this function creates a socket that is in an error state
static int _makeDeadSocket(const char *refillData = NULL, ssize_t len = -1)
{
  //it does it by creating a socket pair and closing one side
  int sp[2] = {-1,-1};
  JASSERT(_real_socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0) (JASSERT_ERRNO)
    .Text("socketpair() failed");
  JASSERT(sp[0]>=0 && sp[1]>=0) (sp[0]) (sp[1])
    .Text("socketpair() failed");
  if (refillData != NULL) {
    JASSERT(Util::writeAll(sp[1], refillData, len) == len);
  }
  _real_close(sp[1]);
  if (really_verbose) {
    JTRACE("Created dead socket.") (sp[0]);
  }
  return sp[0];
}

dmtcp::SocketConnection::SocketConnection(int domain, int type, int protocol)
  : _sockDomain(domain)
  , _sockType(type)
  , _sockProtocol(protocol)
  , _peerType(PEER_UNKNOWN)
{ }

void dmtcp::SocketConnection::addSetsockopt(int level, int option,
                                            const char* value, int len)
{
  _sockOptions[level][option] = jalib::JBuffer(value, len);
}

void dmtcp::SocketConnection::restoreSocketOptions(dmtcp::vector<int>& fds)
{
  typedef map<int64_t, map< int64_t, jalib::JBuffer> >::iterator levelIterator;
  typedef map<int64_t, jalib::JBuffer>::iterator optionIterator;

  for (levelIterator lvl = _sockOptions.begin();
       lvl!=_sockOptions.end(); ++lvl) {
    for (optionIterator opt = lvl->second.begin();
         opt!=lvl->second.end(); ++opt) {
      JTRACE("Restoring socket option.")
        (fds[0]) (opt->first) (opt->second.size());
      int ret = _real_setsockopt(fds[0], lvl->first, opt->first,
                                 opt->second.buffer(),
                                 opt->second.size());
      JASSERT(ret == 0) (JASSERT_ERRNO) (fds[0])
        (lvl->first) (opt->first) (opt->second.size())
        .Text("Restoring setsockopt failed.");
    }
  }
}

void dmtcp::SocketConnection::serialize(jalib::JBinarySerializer& o)
{
  JSERIALIZE_ASSERT_POINT("dmtcp::SocketConnection");
  o & _sockDomain  & _sockType & _sockProtocol & _peerType;

  JSERIALIZE_ASSERT_POINT("SocketOptions:");
  uint64_t numSockOpts = _sockOptions.size();
  o & numSockOpts;
  if (o.isWriter()) {
    //JTRACE("TCP Serialize ") (_type) (_id.conId());
    typedef map< int64_t, map< int64_t, jalib::JBuffer > >::iterator levelIterator;
    typedef map< int64_t, jalib::JBuffer >::iterator optionIterator;

    uint64_t numLvl = _sockOptions.size();
    o & numLvl;

    for (levelIterator lvl = _sockOptions.begin();
         lvl!=_sockOptions.end(); ++lvl) {
      int64_t lvlVal = lvl->first;
      uint64_t numOpts = lvl->second.size();

      JSERIALIZE_ASSERT_POINT("Lvl");

      o & lvlVal & numOpts;

      for (optionIterator opt = lvl->second.begin();
           opt!=lvl->second.end(); ++opt) {
        int64_t optType = opt->first;
        jalib::JBuffer& buffer = opt->second;
        int64_t bufLen = buffer.size();

        JSERIALIZE_ASSERT_POINT("Opt");

        o & optType & bufLen;
        o.readOrWrite(buffer.buffer(), bufLen);
      }
    }
  } else {
    uint64_t numLvl = 0;
    o & numLvl;

    while (numLvl-- > 0) {
      int64_t lvlVal = -1;
      int64_t numOpts = 0;

      JSERIALIZE_ASSERT_POINT("Lvl");

      o & lvlVal & numOpts;

      while (numOpts-- > 0) {
        int64_t optType = -1;
        int64_t bufLen = -1;

        JSERIALIZE_ASSERT_POINT("Opt");

        o & optType & bufLen;

        jalib::JBuffer buffer(bufLen);
        o.readOrWrite(buffer.buffer(), bufLen);

        _sockOptions[lvlVal][optType]=buffer;
      }
    }
  }

  JSERIALIZE_ASSERT_POINT("EndSockOpts");
}

/*****************************************************************************
 * TCP Connection
 *****************************************************************************/

/*onSocket*/
  dmtcp::TcpConnection::TcpConnection(int domain, int type, int protocol)
  : Connection(TCP_CREATED)
  , SocketConnection(domain, type, protocol)
  , _listenBacklog(-1)
  , _bindAddrlen(0)
  , _remotePeerId(ConnectionIdentifier::Null())
{
  if (domain != -1) {
    // Sometimes _sockType contains SOCK_CLOEXEC/SOCK_NONBLOCK flags.
    if ((type & 077) == SOCK_DGRAM) {
      JWARNING(false) (type)
        .Text("Datagram Sockets not supported. "
              "Hopefully, this is a short lived connection!");
    } else {
      JWARNING((domain == AF_INET || domain == AF_UNIX || domain == AF_INET6)
               && (type & 077) == SOCK_STREAM)
        (domain) (type) (protocol);
    }
    JTRACE("Creating TcpConnection.") (id()) (domain) (type) (protocol);
  }
  memset(&_bindAddr, 0, sizeof _bindAddr);
}

dmtcp::TcpConnection& dmtcp::TcpConnection::asTcp()
{
  return *this;
}

bool TcpConnection::isBlacklistedTcp(const sockaddr* saddr, socklen_t len)
{
  JASSERT(saddr != NULL);
  if (len <= sizeof(saddr->sa_family)) {
    return false;
  }

  if (saddr->sa_family == AF_INET) {
    struct sockaddr_in* addr =(sockaddr_in*)saddr;
    // Ports 389 and 636 are the well-known ports in /etc/services that
    // are reserved for LDAP.  Bash continues to maintain a connection to
    // LDAP, leading to problems at restart time.  So, we discover the LDAP
    // remote addresses, and turn them into dead sockets at restart time.
    //However, libc.so:getpwuid() can call libnss_ldap.so which calls
    // libldap-2.4.so to create the LDAP socket while evading our connect
    // wrapper.
    int blacklistedRemotePorts[] = {53,                 // DNS Server
                                    389, 636,           // LDAP
                                    -1};
    JTRACE("port check")(ntohs(addr->sin_port));
    for (size_t i = 0; blacklistedRemotePorts[i] != -1; i++) {
      if (ntohs(addr->sin_port) == blacklistedRemotePorts[i]) {
        JTRACE("LDAP port found") (ntohs(addr->sin_port))
          (blacklistedRemotePorts[0]) (blacklistedRemotePorts[1]);
        return true;
      }
    }
  } else if (saddr->sa_family == AF_UNIX) {
    struct sockaddr_un *uaddr = (struct sockaddr_un *) saddr;
    static dmtcp::string blacklist[] = {""};
    for (size_t i = 0; blacklist[i] != ""; i++) {
      if (Util::strStartsWith(uaddr->sun_path, blacklist[i].c_str()) ||
          Util::strStartsWith(&uaddr->sun_path[1], blacklist[i].c_str())) {
        JTRACE("Blacklisted socket address") (uaddr->sun_path);
        return true;
      }
    }
  }

  // FIXME:  Consider adding configure or dmtcp_launch option to disable
  //   all remote connections.  Handy as quick test for special cases.
  return false;
}

void dmtcp::TcpConnection::onBind(const struct sockaddr* addr, socklen_t len)
{
  if (really_verbose) {
    JTRACE("Binding.") (id()) (len);
  }

  // If the bind succeeded, we do not need any additional assert.
  //JASSERT(_type == TCP_CREATED) (_type) (id())
  //  .Text("Binding a socket in use????");

  if (_sockDomain == AF_UNIX && addr != NULL) {
    JASSERT(len <= sizeof _bindAddr) (len) (sizeof _bindAddr)
      .Text("That is one huge sockaddr buddy.");
    _bindAddrlen = len;
    memcpy(&_bindAddr, addr, len);
  } else {
    _bindAddrlen = sizeof(_bindAddr);
    // Do not rely on the address passed on to bind as it may contain port 0
    // which allows the OS to give any unused port. Thus we look ourselves up
    // using getsockname.
    /*
      アドレスに依存してはいけない、osが未使用のportを与えるのを許す。我々は我々自身で探す getsocknameを使って
     */
    JASSERT(getsockname(_fds[0], (struct sockaddr *)&_bindAddr, &_bindAddrlen) == 0)
      (JASSERT_ERRNO);
    sockaddr_in *sn =(sockaddr_in*) & _bindAddr;
    unsigned short port = htons(sn->sin_port);
    JTRACE("getsockname")(_fds[0])(port); 
  }
  
  _type = TCP_BIND;
}

void dmtcp::TcpConnection::onListen(int backlog)
{
  /* The application didn't issue a bind() call; the kernel will assign
   * a random address in this case. Call the regular onBind() post-
   * processing function which will save the address returned by the kernel
   * and change the state of the socket connection to TCP_BIND.
   */
  if (_type == TCP_CREATED) {
    onBind(NULL, 0);
  }

  if (really_verbose) {
    JTRACE("Listening.") (id()) (backlog);
  }
  JASSERT(_type == TCP_BIND) (_type) (id())
    .Text("Listening on a non-bind()ed socket????");
  // A -1 backlog is not an error.
  //JASSERT(backlog > 0) (backlog)
  //.Text("That is an odd backlog????");

  _type = TCP_LISTEN;
  _listenBacklog = backlog;
}

void dmtcp::TcpConnection::onConnect(const struct sockaddr *addr, socklen_t len)
{
  if (really_verbose) {
    JTRACE("Connecting.") (id());
  }
  JWARNING(_type == TCP_CREATED || _type == TCP_BIND) (_type) (id())
    .Text("Connecting with an in-use socket????");

  if (addr != NULL && isBlacklistedTcp(addr, len)) {
    _type = TCP_EXTERNAL_CONNECT;
    _connectAddrlen = len;
    memcpy(&_connectAddr, addr, len);
  } else {
    _type = TCP_CONNECT;
  }
}

/*onAccept*/
dmtcp::TcpConnection::TcpConnection(const TcpConnection& parent,
                                    const ConnectionIdentifier& remote)
  : Connection(TCP_ACCEPT)
  , SocketConnection(parent._sockDomain, parent._sockType, parent._sockProtocol)
  , _listenBacklog(-1)
  , _bindAddrlen(0)
  , _remotePeerId(remote)
{
  JTRACE("get remotePeerId:OnAccept")(_remotePeerId);// called only on dmtcp_launch
  if (really_verbose) {
    JTRACE("Accepting.") (id()) (parent.id()) (remote);
  }

  //     JASSERT(parent._type == TCP_LISTEN) (parent._type) (parent.id())
  //             .Text("Accepting from a non listening socket????");
  memset(&_bindAddr, 0, sizeof _bindAddr);
}

void dmtcp::TcpConnection::onError()
{
  JTRACE("Error.") (id());
  _type = TCP_ERROR;
  JTRACE("Creating dead socket.") (_fds[0]) (_fds.size());
  const vector<char>& buffer =
    KernelBufferDrainer::instance().getDrainedData(_id);
  Util::dupFds(_makeDeadSocket(&buffer[0], buffer.size()), _fds);
}

void dmtcp::TcpConnection::drain()
{

  JASSERT(_fds.size() > 0) (id());


  JTRACE("check")(TCP_CREATED)(TCP_ACCEPT)(TCP_LISTEN)(TCP_BIND)(TCP_CONNECT);
  JTRACE("called c->drain():TcpConnection::drain()")(_fds[0])(_fcntlFlags)(_type)(_fcntlFlags & O_ASYNC)(myIsRestart);
  // sawada original:start
  if(myIsRestart == true && _type != TCP_LISTEN){
    int isSetsockopt[3] = {0,0,0};
    int dumyone[3] = {1,1,1};
    
    // level 6 = tcp,IPPROTO_TCP is defined in /include/netinet/in.h 
    // option 1 = TCP_NODELAY,is defined in netinet/tcp.h
    isSetsockopt[0] = setsockopt(_fds[0], IPPROTO_TCP, TCP_NODELAY, &dumyone[0], sizeof(&dumyone[0]));
    isSetsockopt[1] = setsockopt(_fds[0], SOL_SOCKET, SO_SNDBUF, &dumyone[1], sizeof(&dumyone[1]));
    isSetsockopt[2] = setsockopt(_fds[0], SOL_SOCKET, SO_RCVBUF, &dumyone[2], sizeof(&dumyone[2]));
    _hasLock = true;
    _fcntlOwner = fcntl(_fds[0], F_SETOWN,0);
    _fcntlSignal = fcntl(_fds[0], F_SETSIG, 0);
    int64_t check_fcntlFlags = fcntl(_fds[0],F_GETFL);
    _fcntlFlags = fcntl(_fds[0],F_GETFL);
    _type = TCP_ACCEPT;
    
    JTRACE("check fcntlFlags")(isSetsockopt[0])(_fds[0])(check_fcntlFlags)( _fcntlFlags)(_fcntlFlags & O_ASYNC)(_type);
  }
  // sawada original:end

  if ((_fcntlFlags & O_ASYNC) != 0) {
    if (really_verbose) {
      JTRACE("Removing O_ASYNC flag during checkpoint.") (_fds[0]) (id());
    }
    errno = 0;
    JASSERT(fcntl(_fds[0],F_SETFL,_fcntlFlags & ~O_ASYNC) == 0)
      (JASSERT_ERRNO) (_fds[0]) (id());
  }

  if (dmtcp_no_coordinator()) {
    markExternalConnect();
  }

  switch (_type) {
    case TCP_ERROR:
      // Treat TCP_ERROR as a regular socket for draining purposes. There still
      // might be some stale data on it.
    case TCP_CONNECT:
    case TCP_ACCEPT:
      JTRACE("Will drain socket") (_hasLock) (_fds[0]) (_id) (_remotePeerId);
      KernelBufferDrainer::instance().beginDrainOf(_fds[0], _id);
      break;
    case TCP_LISTEN:
      JTRACE("addListenSocket");
      KernelBufferDrainer::instance().addListenSocket(_fds[0]);
      break;
    case TCP_BIND:
      JWARNING(_type != TCP_BIND) (_fds[0])
        .Text("If there are pending connections on this socket,\n"
              " they won't be checkpointed because"
              " it is not yet in a listen state.");
      break;
    case TCP_EXTERNAL_CONNECT:
      JTRACE("Socket to External Process, won't be drained") (_fds[0]);
      break;
  }
}

void dmtcp::TcpConnection::doSendHandshakes(const ConnectionIdentifier& coordId)
{
  switch (_type) {
    case TCP_CONNECT:
    case TCP_ACCEPT:
      JTRACE("Type TCP_ACCEPT:Sending handshake ...") (id()) (_fds[0]);//id()の中にconnectionIDも含まれている
      sendHandshake(_fds[0], coordId);//coordIdのconnectionIDは-1,
      break;
    case TCP_EXTERNAL_CONNECT:
      JTRACE("Socket to External Process, skipping handshake send") (_fds[0]);
      break;
  }
}

void dmtcp::TcpConnection::doRecvHandshakes(const ConnectionIdentifier& coordId)
{
  switch (_type) {
    case TCP_CONNECT:
    case TCP_ACCEPT:
      recvHandshake(_fds[0], coordId);
      JTRACE("Received handshake.") (id()) (_remotePeerId) (_fds[0]);
      break;
    case TCP_EXTERNAL_CONNECT:
      JTRACE("Socket to External Process, skipping handshake recv") (_fds[0]);
      break;
  }
}

void dmtcp::TcpConnection::refill(bool isRestart)
{
  if ((_fcntlFlags & O_ASYNC) != 0) {
    JTRACE("Re-adding O_ASYNC flag.") (_fds[0]) (id());
    restoreSocketOptions(_fds);
  } else if (isRestart && _sockDomain != AF_INET6 &&
             _type != TCP_EXTERNAL_CONNECT) {
    restoreSocketOptions(_fds);
  }
}

void dmtcp::TcpConnection::postRestart()
{
  
  JTRACE("socket postRestart()")(id())(id().hostid())(_fds[0]);
  int fd;
 
  // sawada
  char deamonProcess1[100]="DMTCP:mpirun";
  char deamonProcess2[100]="DMTCP:orted";
  // 

  JASSERT(_fds.size() > 0);
  switch (_type) {
    case TCP_PREEXISTING:
    case TCP_INVALID:
    case TCP_EXTERNAL_CONNECT:
      JTRACE("Creating dead socket.") (_fds[0]) (_fds.size());
      Util::dupFds(_makeDeadSocket(), _fds);
      break;

    case TCP_ERROR:
      // Disconnected socket. Need to refill the drained data
      {
        const vector<char>& buffer =
          KernelBufferDrainer::instance().getDrainedData(_id);
        Util::dupFds(_makeDeadSocket(&buffer[0], buffer.size()), _fds);
      }
      break;

    case TCP_CREATED:
    case TCP_BIND:
    case TCP_LISTEN:
      
      // Sometimes _sockType contains SOCK_CLOEXEC/SOCK_NONBLOCK flags.
      JWARNING((_sockDomain == AF_INET || _sockDomain == AF_UNIX ||
                _sockDomain == AF_INET6) && (_sockType & 077) == SOCK_STREAM)
        (id()) (_sockDomain) (_sockType) (_sockProtocol)
        .Text("Socket type not yet [fully] supported.");

      
      if (really_verbose) {
        JTRACE("Restoring socket.") (id()) (_fds[0]);
      }

      fd = _real_socket(_sockDomain, _sockType, _sockProtocol);
      JASSERT(fd != -1) (JASSERT_ERRNO);
      //udsのとき...sockDomain() = 1
      //tcpのとき...sockDomain() = 2
      JTRACE("Before Util::dupFds(fd, _fds)")(_sockDomain)(fd)(_fds[0]);
      Util::dupFds(fd, _fds);
      

      if (_type == TCP_CREATED) break;

      if (_sockDomain == AF_UNIX &&
          _bindAddrlen > sizeof(_bindAddr.ss_family)) {
        struct sockaddr_un *uaddr = (sockaddr_un*) &_bindAddr;
        if (uaddr->sun_path[0] != '\0') {
          JTRACE("Unlinking stale unix domain socket.") (uaddr->sun_path);
          JWARNING(unlink(uaddr->sun_path) == 0) (uaddr->sun_path);
        }
      }

      
      /*
       * During restart, some socket options must be restored(using
       * setsockopt) before the socket is used(bind etc.), otherwise we might
       * not be able to restore them at all. One such option is set in the
       * following way for IPV6 family:
       * setsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY,...)
       * This fix works for now. A better approach would be to restore the
       * socket options in the order in which they are set by the user
       * program.  This fix solves a bug that caused Open MPI to fail to
       * restart under DMTCP.
       *                               --Kapil
       */

      if (_sockDomain == AF_INET6) {
        JTRACE("Restoring some socket options before binding.");
        typedef map< int64_t,
                     map< int64_t, jalib::JBuffer > >::iterator levelIterator;
        typedef map< int64_t, jalib::JBuffer >::iterator optionIterator;

        for (levelIterator lvl = _sockOptions.begin();
             lvl!=_sockOptions.end(); ++lvl) {
          if (lvl->first == IPPROTO_IPV6) {
            for (optionIterator opt = lvl->second.begin();
                 opt!=lvl->second.end(); ++opt) {
              if (opt->first == IPV6_V6ONLY) {
                if (really_verbose) {
                  JTRACE("Restoring socket option.")
                    (_fds[0]) (opt->first) (opt->second.size());
                }
                int ret = _real_setsockopt(_fds[0], lvl->first, opt->first,
                                           opt->second.buffer(),
                                           opt->second.size());
                JASSERT(ret == 0) (JASSERT_ERRNO) (_fds[0]) (lvl->first)
                  (opt->first) (opt->second.buffer()) (opt->second.size())
                  .Text("Restoring setsockopt failed.");
              }
            }
          }
        }
      }

      if (really_verbose) {
        JTRACE("Binding socket.") (id());
      }
      errno = 0;
      // case TCP_LISTENのどこでもエラー
      //sawada original:start
      //struct sockaddr_in *temp = (sockaddr_in*) &_bindAddr;
      // JTRACE("check port of missing bind")(temp->sin_port);
      //sawada original:end

      //sawada orignal:start
      listenfd = _fds[0];
      //sawada original:end
      JWARNING(_real_bind(_fds[0], (sockaddr*) &_bindAddr,_bindAddrlen) == 0)
        (JASSERT_ERRNO) (id()) .Text("Bind failed.");
      
      if (_type == TCP_BIND) break;

      if (really_verbose) {
        JTRACE("Listening socket.") (id());
      }
      errno = 0;
      JTRACE("socket:postRestart:event is TCP_LISTEN")(_fds[0])(id())(_listenBacklog)(_type);
      JWARNING(_real_listen(_fds[0], _listenBacklog) == 0)
        (JASSERT_ERRNO) (id()) (_listenBacklog) .Text("listen failed.");
      if (_type == TCP_LISTEN) break;

      break;

    case TCP_ACCEPT:
      JTRACE("socket:postRestart:event is TCP_ACCEPT")(_remotePeerId);
      JASSERT(!_remotePeerId.isNull()) (id()) (_remotePeerId) (_fds[0])
        .Text("Can't restore a TCP_ACCEPT socket with null acceptRemoteId.\n"
              "  Perhaps handshake went wrong?");

      
      /*sawada original:start*/
      if(strcmp(deamonProcess1,procname)==0){
        JTRACE("this is parent proc(orterun)");
        if(id().hostid()!=_remotePeerId.hostid()){
          fdsAndHostid[_remotePeerId.hostid()]=_fds[0];
          for(map<uint64_t,int32_t>::iterator pair=fdsAndHostid.begin();pair!=fdsAndHostid.end();pair++){
	    JTRACE("parent proc connect another parent proc")(pair->first)(pair->second)(_remotePeerId.hostid());
          }
        }
      }
      else if(strcmp(deamonProcess2,procname)==0){
	JTRACE("this is parent proc(orted)");
	if(id().hostid()!=_remotePeerId.hostid()){
          fdsAndHostid[_remotePeerId.hostid()]=_fds[0];
          for(map<uint64_t,int32_t>::iterator pair=fdsAndHostid.begin();pair!=fdsAndHostid.end();pair++){
	    JTRACE("parent proc connect another parent proc")(pair->first)(pair->second)(_remotePeerId.hostid());
          }
        }

      }
      /*sawada original:end*/
      JTRACE("registerIncoming") (id()) (_remotePeerId) (_fds[0])(id().hostid())(_remotePeerId.hostid())(_sockDomain);
      
      ConnectionRewirer::instance().registerIncoming(id(), this, _sockDomain);
      //通信の効率化のためには通信相手が同一ノードか否かを判断する必要があるため、受信側/送信側双方のホストIDを記録する
      ConnectionRewirer::instance().registerConInfo(id(),_remotePeerId,_sockDomain);//sawada add
      break;

    case TCP_CONNECT:
      //int size = sizeof(_remotePeerId.pid()); // pid_t型:4byte
      //protocol = 0... 通常それぞれの ソケットは、与えられたプロトコルファミリーの種類ごとに一つのプロトコルのみを サポートする。 その場合は protocol に 0 
      // _sockType = SOCK_STREAM
      JTRACE("socket:postRestart:event is TCP_CONNECT")(_sockDomain)(_sockType)(SOCK_STREAM)(SOCK_DGRAM)(_sockProtocol)(_remotePeerId);
#ifdef ENABLE_IP6_SUPPORT
      fd = _real_socket(_sockDomain, _sockType, _sockProtocol);
#else
      fd = _real_socket(_sockDomain == AF_INET6 ? AF_INET : _sockDomain,
                        _sockType, _sockProtocol);
#endif
      JASSERT(fd != -1) (JASSERT_ERRNO);
      //fdsに、real_connect時に使用するfdが入る
      if (_sockDomain == 1) _type = 0x12000;//sawada add 
      JTRACE("create & dup sock fd")(fd)(_sockDomain)((_fds[0]))(_bindAddrlen)(conType());
      Util::dupFds(fd, _fds);
      //JTRACE("After Util::dupFds")(_fds);
      if (_bindAddrlen != 0) { // _bindAddrlen = 0 on restart(when Not balancing)
        JWARNING(_real_bind(_fds[0], (sockaddr*)&_bindAddr, _bindAddrlen) != -1)
          (JASSERT_ERRNO);
      }
      JTRACE("registerOutgoing") (id()) (_remotePeerId) (_fds[0]);
      ConnectionRewirer::instance().registerOutgoing(_remotePeerId, this);
      break;
      //    case TCP_EXTERNAL_CONNECT:
      //      int sockFd = _real_socket(_sockDomain, _sockType, _sockProtocol);
      //      JASSERT(sockFd >= 0);
      //      JASSERT(_real_dup2(sockFd, _fds[0]) == _fds[0]);
      //      JWARNING(0 == _real_connect(sockFd,(sockaddr*) &_connectAddr,
      //                                  _connectAddrlen))
      //        (_fds[0]) (JASSERT_ERRNO)
      //        .Text("Unable to connect to external process");
      //      break;
  }
  // sawada original :start for debug print
  if(listenfd != 0){
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    getsockname(listenfd,(struct sockaddr *)&addr, &addrlen);
    JTRACE(" getscokname success")(listenfd)(ntohs(addr.sin_port));
    listenfd = 0;
  }
  // sawada original :end
}

void dmtcp::TcpConnection::sendHandshake(int remotefd,
                                         const ConnectionIdentifier& coordId)
{
  jalib::JSocket remote(remotefd);
  ConnMsg msg(ConnMsg::HANDSHAKE);
  msg.from = id();
  msg.coordId = coordId;
  remote << msg; 
}

void dmtcp::TcpConnection::recvHandshake(int remotefd,
                                         const ConnectionIdentifier& coordId)
{
  jalib::JSocket remote(remotefd);
  ConnMsg msg;
  msg.poison();
  remote >> msg;

  msg.assertValid(ConnMsg::HANDSHAKE);
  JASSERT(msg.coordId == coordId) (msg.coordId) (coordId)
    .Text("Peer has a different dmtcp_coordinator than us!\n"
          "  It must be the same.");

  if (_remotePeerId.isNull()) {
    //first time
    
    _remotePeerId = msg.from;
    JTRACE("first time recvHandshake")(msg.from)(_remotePeerId.pid())(_remotePeerId.hostid())(_remotePeerId.conId());
    JASSERT(!_remotePeerId.isNull())
      .Text("Read handshake with invalid 'from' field.");
  } else {
    //next time
    /*default*/
    // JASSERT(_remotePeerId == msg.from)
    //   (_remotePeerId) (msg.from)
    //   .Text("Read handshake with a different 'from' field"
    //         " than a previous handshake.");

    //sawad org:s 
    if(myIsRestart == true && _remotePeerId != msg.from){
      JTRACE("maybe connection partner is changed by process relocation per process basis")(_remotePeerId)(msg.from);
      _remotePeerId = msg.from;
      JASSERT(!_remotePeerId.isNull())
      .Text("Read handshake with invalid 'from' field.");
    }
    //sawada org:e 
  }
}

void dmtcp::TcpConnection::serializeSubClass(jalib::JBinarySerializer& o)
{
  JSERIALIZE_ASSERT_POINT("dmtcp::TcpConnection");
  o & _listenBacklog & _bindAddrlen & _bindAddr & _remotePeerId;
  SocketConnection::serialize(o);
}

/*****************************************************************************
 * RawSocket Connection
 *****************************************************************************/
/*onSocket*/
dmtcp::RawSocketConnection::RawSocketConnection(int domain, int type,
                                                int protocol)
  : Connection(RAW)
    , SocketConnection(domain, type, protocol)
{
  JASSERT(type == -1 ||(type & SOCK_RAW));
  JASSERT(domain == -1 || domain == AF_NETLINK) (domain)
    .Text("Only Netlink raw socket supported");
  JTRACE("Creating Raw socket.") (id()) (domain) (type) (protocol);
}

void dmtcp::RawSocketConnection::drain()
{
  JASSERT(_fds.size() > 0) (id());

  if ((_fcntlFlags & O_ASYNC) != 0) {
    if (really_verbose) {
      JTRACE("Removing O_ASYNC flag during checkpoint.") (_fds[0]) (id());
    }
    errno = 0;
    JASSERT(fcntl(_fds[0], F_SETFL, _fcntlFlags & ~O_ASYNC) == 0)
      (JASSERT_ERRNO) (_fds[0]) (id());
  }
}

void dmtcp::RawSocketConnection::refill(bool isRestart)
{
  if ((_fcntlFlags & O_ASYNC) != 0) {
    JTRACE("Re-adding O_ASYNC flag.") (_fds[0]) (id());
    restoreSocketOptions(_fds);
  } else if (isRestart) {
    restoreSocketOptions(_fds);
  }
}

void dmtcp::RawSocketConnection::postRestart()
{
  JASSERT(_fds.size() > 0);
  if (really_verbose) {
    JTRACE("Restoring socket.") (id()) (_fds[0]);
  }

  int sockfd = _real_socket(_sockDomain,_sockType,_sockProtocol);
  JASSERT(sockfd != -1);
  Util::dupFds(sockfd, _fds);
}

void dmtcp::RawSocketConnection::serializeSubClass(jalib::JBinarySerializer& o)
{
  JSERIALIZE_ASSERT_POINT("dmtcp::RawSocketConnection");
  SocketConnection::serialize(o);
}
