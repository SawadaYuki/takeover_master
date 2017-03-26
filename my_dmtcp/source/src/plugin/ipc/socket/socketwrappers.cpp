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

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/un.h>
#include <arpa/inet.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "socketconnection.h"
#include "socketconnlist.h"
#include "socketwrappers.h"
#include "../jalib/jassert.h"
#include "../jalib/jfilesystem.h"

#include "kernelbufferdrainer.h"//sawada

// from connectionrewirer.cpp
// 新しいfdを用意するかどうかのflag
extern int ortedNewfd_is;
extern int orterunNewfd_is;

using namespace dmtcp;
int ret;//元

/* getaddrinfo() (and possibly getnameinfo()) open socket to external services
 * (DNS etc.) to resolve hostname/address etc. In doing so, the call to
 * socket() is captured by our wrappers, however, the call to close() is
 * internal and thus not captured by the close wrapper. This results in a stale
 * connection. To avoid this problem, we disable socket processing for the
 * thread calling getaddrinfo().
 */
static __thread bool _doNotProcessSockets = false;

extern "C" int socket(int domain, int type, int protocol)
{
  DMTCP_PLUGIN_DISABLE_CKPT();
  ret = _real_socket(domain, type, protocol);

  // original :start
  
  //restore予定のfdを格納した_drainedData(map変数)のコピー作成
  const map<int , dmtcp::vector<char> >& checkfds = KernelBufferDrainer::instance().getDrainedFds();
  int flag = 1;  

  //restore予定のfdを作った場合、のちにcloseするためにかぶったfdを格納しておくためのvector
  vector<int> usedfds;
  usedfds.push_back(ret);
  int ret_new=ret;
  
  //orterun,ortedのどちらかで新しいfdを用意する(flag:1)場合
  if(ortedNewfd_is == 1 || orterunNewfd_is == 1){
   JTRACE("In if(ortedNewfd_is == 1 || )");
   while(flag == 1){
   
     JTRACE("compare fds(ret_new)")(ortedNewfd_is)(ret_new);
     //作成したfdと_drainedData(map変数)のコピー内のfdの比較
     if(checkfds.find(ret_new) == checkfds.end()){
       //未使用のfdを得たので比較のループ(while)を抜ける
       JTRACE("unuseing fd(ret)")(ret_new);
       flag = 0;
       break;
     }else{//fdがかぶったので新たにソケットfdを取得
       JTRACE("using fd(ret)")(ret_new)(ret);
        ret_new = _real_socket(domain, type, protocol);
        usedfds.push_back(ret_new);
        continue;
     }
  
   }
  
   JTRACE("final fds")(ret_new);
   ret=ret_new;

   int i;
   
   //新しいfdの取得や比較のためにオープンしたfdをクローズ
   for(i=0;i<usedfds.size()-1;i++){
     JTRACE("close fd")(usedfds[i])(i);
     _real_close(usedfds[i]);
   }
  }// if(orted_ 
  
  // original :end
  
  if (ret != -1 && !_doNotProcessSockets) {
    Connection *con;
    JTRACE("socket created") (ret) (domain) (type) (protocol);
    if ((type & 0xff) == SOCK_RAW) {
      JASSERT(domain == AF_NETLINK) (domain) (type)
        .Text("Only Netlink Raw sockets supported");
      con = new RawSocketConnection(domain, type, protocol);
    } else {
      JTRACE("new TcpConnection(domain, type, protocol);");
      con = new TcpConnection(domain, type, protocol);
    }
    JTRACE("Before SocketConnList::instance().add(ret, con);")(ret)(con);
    SocketConnList::instance().add(ret, con);
  }
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}

extern "C" int connect(int sockfd, const struct sockaddr *serv_addr,
                       socklen_t addrlen)
{
  DMTCP_PLUGIN_DISABLE_CKPT(); // The lock is released inside the macro.
  int ret = _real_connect(sockfd,serv_addr,addrlen);

  //no blocking connect... need to hang around until it is writable
  if (ret < 0 && errno == EINPROGRESS)
  {
    fd_set wfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);

    tv.tv_sec = 15;
    tv.tv_usec = 0;

    retval = select(sockfd+1, NULL, &wfds, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
      perror("select()");
    else if (FD_ISSET(sockfd, &wfds))
    {
      int val = -1;
      socklen_t sz = sizeof(val);
      getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&val,&sz);
      if (val==0) ret = 0;
    }
    else
      JTRACE("No data within five seconds.");
  }

  if (ret != -1 && !_doNotProcessSockets) {
    JTRACE("Before TcpConnection *con =");
    TcpConnection *con =
      (TcpConnection*) SocketConnList::instance().getConnection(sockfd);
    if (con == NULL) {
      JTRACE("Connect operation on unsupported socket type.");
    } else {
      con->onConnect(serv_addr, addrlen);

#if HANDSHAKE_ON_CONNECT == 1
      JTRACE("connected, sending 1-way handshake") (sockfd) (con->id());
      con->sendHandshake(sockfd, DmtcpWorker::instance().coordinatorId());
      JTRACE("1-way handshake sent");
#else
      JTRACE("connected") (sockfd) (con->id());
#endif
    }
  }
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}

extern "C" int bind(int sockfd, const struct sockaddr *my_addr,
                     socklen_t addrlen)
{
  DMTCP_PLUGIN_DISABLE_CKPT(); // The lock is released inside the macro.
  int ret = _real_bind(sockfd, my_addr, addrlen);
  if (ret != -1 && !_doNotProcessSockets) {
    JTRACE("Before TcpConnection *con")(sockfd)(ret);
    TcpConnection *con =
      (TcpConnection*) SocketConnList::instance().getConnection(sockfd);
    if (con == NULL) {
      JTRACE("bind operation on unsupported socket type.");
    } else {
      con->onBind((struct sockaddr*) my_addr, addrlen);
      JTRACE("bind") (sockfd) (con->id());
    }
  }
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}

extern "C" int listen(int sockfd, int backlog)
{
  DMTCP_PLUGIN_DISABLE_CKPT(); // The lock is released inside the macro.
  int ret = _real_listen(sockfd, backlog);
  if (ret != -1 && !_doNotProcessSockets) {
    JTRACE("Before TcpConnection *con =")(sockfd)(backlog);
    TcpConnection *con =
      (TcpConnection*) SocketConnList::instance().getConnection(sockfd);
    if (con == NULL) {
      JTRACE("listen operation on unsupported socket type.");
    } else {
      con->onListen(backlog);
      JTRACE("listen") (sockfd) (con->id()) (backlog);
    }
  }
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}

static void process_accept(int ret, int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen)
{
  JASSERT(ret != -1);
  JTRACE("Before TcpConnection *parent");
  TcpConnection *parent =
    (TcpConnection*) SocketConnList::instance().getConnection(sockfd);
  JTRACE("new TcpConnection(*parent, ConnectionIdentifier::Null())");
  TcpConnection* con = new TcpConnection(*parent, ConnectionIdentifier::Null());
  if (con == NULL) {
    JTRACE("accept operation on unsupported socket type.");
    return;
  }
  JTRACE("Before SocketConnList::instance().add(ret, con);")(ret)(con);
  SocketConnList::instance().add(ret, con);

#if HANDSHAKE_ON_CONNECT == 1
  JTRACE("accepted, waiting for 1-way handshake") (sockfd) (con->id());
  con->recvHandshake(ret, DmtcpWorker::instance().coordinatorId());
  JTRACE("1-way handshake received") (con->getRemoteId());
#else
  JTRACE("accepted incoming connection") (sockfd) (con->id());
#endif
}

extern "C" int accept(int sockfd, struct sockaddr *addr,
                      socklen_t *addrlen)
{
  /* FIXME: accept() is a blocking call that can alter the process state(by
   * creating a new socket-fd). This can cause problems if it happens at a time
   * when some other thread is processing inside a fork() or exec() wrapper.
   * For more details, please look at the comment in
   * dmtcp::DmtcpWorker::wrapperExecutionLockLockExcl().
   *
   * Since it's a blocking call, we cannot grab the actual wrapper-execution
   * lock here.
   */
  struct sockaddr_storage tmp_addr;
  socklen_t tmp_len = 0;
  if (addr == NULL || addrlen == NULL) {
    memset(&tmp_addr,0,sizeof(tmp_addr));
    addr = (struct sockaddr*) &tmp_addr;
    addrlen = &tmp_len;
  }
  int ret = _real_accept(sockfd, addr, addrlen);
  JTRACE("After _real_accept")(ret)(sockfd)(_doNotProcessSockets);
  if (ret != -1 && !_doNotProcessSockets) {
    JTRACE("Next:process_accept()");
    process_accept(ret, sockfd, addr, addrlen);
  }
  return ret;
}

extern "C" int accept4(int sockfd, struct sockaddr *addr,
                         socklen_t *addrlen, int flags)
{
  // Look at the comment for accept()
  struct sockaddr_storage tmp_addr;
  socklen_t tmp_len = 0;
  if (addr == NULL || addrlen == NULL) {
    memset(&tmp_addr,0,sizeof(tmp_addr));
    addr = (struct sockaddr*) &tmp_addr;
    addrlen = &tmp_len;
  }
  int ret = _real_accept4(sockfd, addr, addrlen, flags);
  if (ret != -1 && !_doNotProcessSockets) {
    process_accept(ret, sockfd, addr, addrlen);
  }
  return ret;
}

extern "C" int setsockopt(int sockfd, int level, int optname,
                          const void *optval, socklen_t optlen)
{
  int ret = _real_setsockopt(sockfd, level, optname, optval, optlen);
  //JTRACE("check")(SOL_SOCKET);
  JTRACE("setsockopt") (ret) (sockfd)(optval) (level) (optname)(_doNotProcessSockets)(optlen);
  if (ret != -1 && !_doNotProcessSockets) {
    
    //JTRACE("check optname")(SO_DEBUG)(SO_KEEPALIVE)(SO_DONTROUTE)(SO_LINGER)(SO_REUSEADDR)(SO_REUSEPORT)(SO_BROADCAST)(SO_OOBINLINE)(SO_SNDBUF)(SO_RCVBUF);
    TcpConnection *con =
      (TcpConnection*) SocketConnList::instance().getConnection(sockfd);
    if (con == NULL) {
      JTRACE("setsockopt operation on unsupported socket type.");
      return ret;
    }
  }
  return ret;
}

#if 0
extern "C" int getsockopt(int sockfd, int level, int optname,
                          void *optval, socklen_t *optlen)
{
  /* We don't want to acquire the lock here as this is not needed. Also,
   * aquiring the lock here might cause a deadlock when this function is called
   * from within connect(). Here is the deadlock situation:
   * User-thread connect():    acquire lock
   * Ckpt-thread ckpt():       Queued on wr-lock
   * User-thread getsockopt(): block on read lock().
   */
  int ret = _real_getsockopt(sockfd, level, optname, optval, optlen);
  PASSTHROUGH_DMTCP_HELPER(getsockopt,sockfd,level,optname,optval,optlen);
}
#endif

extern "C" int socketpair(int d, int type, int protocol, int sv[2])
{
  DMTCP_PLUGIN_DISABLE_CKPT();

  JASSERT(sv != NULL);
  int rv = _real_socketpair(d,type,protocol,sv);
  if (rv != -1 && !_doNotProcessSockets) {
    JTRACE("socketpair()") (sv[0]) (sv[1]);

    dmtcp::TcpConnection *a, *b;

    a = new dmtcp::TcpConnection(d, type, protocol);
    JTRACE("After a = new dmtcp::TcpConnection(d, type, protocol)")(d)(type)(protocol);
    a->onConnect();
    JTRACE("After a->onConnect()");
    b = new dmtcp::TcpConnection(*a, a->id());
    JTRACE("After b  = new dmtcp::TcpConnection(*a, a->id())");

    JTRACE("Before dmtcp::SocketConnList::instance().add(sv[0], a);")(sv[0])(sv[1])(a)(b);
    dmtcp::SocketConnList::instance().add(sv[0], a);
    dmtcp::SocketConnList::instance().add(sv[1], b);
  }

  DMTCP_PLUGIN_ENABLE_CKPT();

  return rv;
}

extern "C" int getaddrinfo(const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res)
{
  DMTCP_PLUGIN_DISABLE_CKPT();
  // See comment near definition of _doNotProcessSockets;
  _doNotProcessSockets = true;
  int ret = _real_getaddrinfo(node, service, hints, res);
  _doNotProcessSockets = false;
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}

extern "C" int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                           char *host, size_t hostlen,
                           char *serv, size_t servlen, int flags)
{
  DMTCP_PLUGIN_DISABLE_CKPT();
  _doNotProcessSockets = true;
  int ret = _real_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
  _doNotProcessSockets = false;
  DMTCP_PLUGIN_ENABLE_CKPT();
  return ret;
}
