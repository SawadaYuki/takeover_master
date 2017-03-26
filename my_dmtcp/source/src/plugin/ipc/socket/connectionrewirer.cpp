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

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dmtcp.h"
#include "protectedfds.h"
#include "util.h"
#include "jsocket.h"

#include "connectionrewirer.h"
#include "socketconnection.h"
#include "socketwrappers.h"

using namespace dmtcp;


// sawada org: start
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <algorithm>
#include <fcntl.h>
#include <sstream>    
#define DEAD_FD "DEAD_FD" //12/15


//function
std::vector<std::string> split(const std::string &str, char sep);
std::vector<int> split_int(const std::string &str, char sep);


//value
std::vector<int> udsconnection;
std::vector<int> NotusingconnectionForOrted;// orterunに１つもプロセスが負荷分散しないとき用
char procname[100];
int blanceproc=0;
int ortedNewfd_is;//ortedが新しい受信用fdを用意するかのflag(1:用意する)
int orterunNewfd_is=0;//orterunが新しい受信用fdを用意するかのflag(1:用意する)
vector<int32_t> newfds;//orterun or ortedが新しく用意したfd逹
vector<int32_t> removefdsOfparentproc;

extern jalib::JServerSocket restoreSocket_parent;
extern int ptsmx; //from connectionlist.cpp
extern int numOfChildProc; //from processinfo.cpp
extern int numofProcess;// from coordinatorapi.cpp
extern std::string sProcessLocation;// from coordinatorapi.cpp
extern std::vector<uint64_t> RecvHostIds;//from coordinator.cpp
extern  map<uint64_t,int32_t> fdsAndHostid;// from socketconnection.cpp (_remotePeerId.hostid(),受信用fd)
extern  int balanceproc20151118;//form coordinatorapi.cpp

static int numofBlanceProc = 0;
static int64_t ConOfLeftOrted;
static int64_t hostidOfleftNode;
static uint64_t blanceproc_hostid=0;
static struct in_addr ipAddr; 
static std::vector< ConnectionIdentifier > changedConId;
static std::vector< ConnectionIdentifier > changedConId2;
static int numOfMissingCon = 0;
static int checkNum = 0;
static int second_check = 0;
static int skipflag = 0;
static int isorterun;
static int parentproc =0;
static char hostip[20];
static int isfindblanceproc = 0;
//sawada org: end




// FIXME: IP6 Support disabled for now. However, we do go through the exercise
// of creating the restore socket and all.
// #define ENABLE_IP6_SUPPORT
static void markSocketNonBlocking(int sockfd)
{
    // Remove O_NONBLOCK flag from listener socket
    int flags = _real_fcntl(sockfd, F_GETFL, NULL);
    JASSERT(flags != -1);
    JASSERT(_real_fcntl(sockfd, F_SETFL,
                        (void*) (long) (flags | O_NONBLOCK)) != -1);
}

static void markSocketBlocking(int sockfd) // => sockfd = PROTECTED_RESTORE_IP4_SOCK_FD(823)
{
    // Remove O_NONBLOCK flag from listener socket
    int flags = _real_fcntl(sockfd, F_GETFL, NULL);
    JASSERT(flags != -1);
    JASSERT(_real_fcntl(sockfd, F_SETFL,
                        (void*) (long) (flags & ~O_NONBLOCK)) != -1);
}

static dmtcp::ConnectionRewirer *theRewirer = NULL;
dmtcp::ConnectionRewirer& dmtcp::ConnectionRewirer::instance()
{
  if (theRewirer == NULL) {
    theRewirer = new ConnectionRewirer();
  }
  return *theRewirer;
}

void dmtcp::ConnectionRewirer::destroy()
{
  dmtcp_close_protected_fd(PROTECTED_RESTORE_IP4_SOCK_FD);
  dmtcp_close_protected_fd(PROTECTED_RESTORE_IP6_SOCK_FD);
  dmtcp_close_protected_fd(PROTECTED_RESTORE_UDS_SOCK_FD);

  // Free up the object.
  delete theRewirer;
  theRewirer = NULL;
}
// sawada org ***********************************************************************
void dmtcp::ConnectionRewirer::BalanceCheckForPendingIncoming(int restoreSockFd,
                                                       ConnectionListT *conList)
{
  JTRACE("SawadacheckForPendingIncoming(),check conList.size")(conList->size())(numofBlanceProc);
  while(numofBlanceProc > 0){
    int fd = _real_accept(restoreSockFd, NULL, NULL);
    if (fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      JTRACE("fd == -1, next return")(fd);
      return;
    }
    JASSERT(fd != -1) (JASSERT_ERRNO) .Text("Accept failed.");
    ConnectionIdentifier id;
    JASSERT(Util::readAll(fd, &id, sizeof id) == sizeof id);
    JTRACE("After Util::readAll")(id)(fd)(restoreSockFd)(conList->size());

    iterator i = conList->find(id);
    if(i == conList->end()){//  blance_proc_ver
        blanceproc_hostid = id.hostid();
        JTRACE("find new_connection by blocking connection")(id)(fd)(blanceproc_hostid);
        //ortedで新たに受信用fdを用意するのでflag:1を立てる
        ortedNewfd_is=1;
        int new_sockfd;

        //新らしいfdの取得(新しいfdを得る具体的な処理はsocketwrapper.cpp)
        new_sockfd=socket(AF_INET,SOCK_STREAM,0);

        vector<int32_t> _dumyFds_orted;
        _dumyFds_orted.push_back(new_sockfd);
	newfds.push_back(new_sockfd);
        JTRACE("under orted,new receive_fds is set")(ortedNewfd_is)(_dumyFds_orted[0])(newfds[0]);
        Util::dupFds(fd,_dumyFds_orted);
    }
    JTRACE("restoring incoming connection") (id);
    numofBlanceProc = numofBlanceProc-1;
  }
  JTRACE("end:SawadacheckForPendingIncoming");
}


//***********************************************************************************************

//default checkForPendingIncoming()にも対応
void dmtcp::ConnectionRewirer::checkForPendingIncoming(int restoreSockFd,
                                                       ConnectionListT *conList)
{
  // original :start
  bool compareFlag = false;
  // 負荷分散プロセスにおいて、なくても困らない通信(死んだortedからの接続)
  if(second_check == 1 && blanceproc == 1){
    JTRACE("second checkpending,and this is blance proc");
    //プロトコルチェンジによって生じたUDSコネクションをacceptし損ねた場合
    if(numOfMissingCon > 0){
      compareFlag = true;
      JTRACE("necessary retry accpet chenged connection")(compareFlag);
    }else if(numOfMissingCon == 0){//従来通りにスキップ
      skipflag = 1;
      return;
    }
  }
  
  // original :end
  
  // default:start
  while (conList->size() > 0) {
    JTRACE("_real_accept(fd,~):fd = restoreSockFd")(restoreSockFd);
    int fd = _real_accept(restoreSockFd, NULL, NULL);
    if (fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      JTRACE("fd == -1, next return")(fd);
      return;
    }
    JASSERT(fd != -1) (JASSERT_ERRNO) .Text("Accept failed.");
    ConnectionIdentifier id;
    JASSERT(Util::readAll(fd, &id, sizeof id) == sizeof id);
    int64_t fcntlFlags = fcntl(fd,F_GETFL);//sawada
    JTRACE("After Util::readAll")(id)(fd)(restoreSockFd)(conList->size())(fcntlFlags);
    
    
    iterator i = conList->find(id);
    //default:end
    
    // org:s
    if(i == conList->end()){//  blance_proc_ver
      
      isfindblanceproc = 1;
      numofBlanceProc = numofBlanceProc -1;
      JTRACE("find new_connection,next dupFds")(id)(fd)(isfindblanceproc)(numofBlanceProc);
      
      if(isorterun == 1){ // blance proc is  under orterun
	
        vector<int32_t> _dumyFds;
        int32_t receivefds;
        int new_sockfdForOrterun;
      
        JTRACE("check blanceproc_hostid")(id.hostid());

	//ortedとの通信のために用意したfdのうち、負荷分散プロセスの元々の親プロセスだったortedのfdの判定
	if(fdsAndHostid.find(id.hostid())==fdsAndHostid.end()){
	  JTRACE("No key");
	}else{
	 
	  receivefds=fdsAndHostid[id.hostid()];
	  JTRACE("find key")(receivefds);

	  //orterunで新たに受信用fdを用意するのでflag:1を立てる
	  orterunNewfd_is = 1; //socketwrapper,connection.cppに通知
        
	  //使用しないfdをudesconnectionに記録し、のちのソケットのrefill,restore処理をスキップするために使用する
	  std::vector<int>::iterator isexistfd = find(udsconnection.begin(),udsconnection.end(),receivefds);
	  //脱退nodeのプロセスが2プロセス以上あった場合、複数回usdconnectionに同じfdをpush_back and _real_closeしてしまうので対策
	  if(isexistfd !=  udsconnection.end()){
	    JTRACE("receivefds is already exist! Not push back to udsconnection")(receivefds);
	  }else{

	    udsconnection.push_back(receivefds);
	    _real_close(receivefds);
	    JTRACE("Push back to udsconnection,and close")(receivefds);
	    //add 12/15:start
	    // char *buf;
	    // char *strenv;
	    // sprintf(buf,"%d",receivefds);
	    // setenv(DEAD_FD, buf, 1);
	    // if ((strenv = getenv(DEAD_FD)) != NULL) {
	    //   JTRACE("setenv success")(strenv);
	    // }else{
	    //   JTRACE("failed to setenv");
	    // }
	    //add 12/15:end
	  }
	  
	  //新らしいfdの取得(新しいfdを得る具体的な処理はsocketwrapper.cpp)
	  new_sockfdForOrterun=socket(AF_INET,SOCK_STREAM,0);
	  newfds.push_back(new_sockfdForOrterun);
	  
	  //念のため、flagのreset
	  //orterunNewfd_is = 0; 
	  JTRACE("orterun: new fd for receive")(new_sockfdForOrterun)(newfds[0]);
	}
      
	//新しく用意したfdをdup
	_dumyFds.push_back(new_sockfdForOrterun);

	Util::dupFds(fd,_dumyFds);
      }// if(isorterun == 1)
      else if(isorterun == 0){ //blancd proc is under orted
        //ortedで新たに受信用fdを用意するのでflag:1を立てる
        ortedNewfd_is=1;
        int new_sockfd;
        //新らしいfdの取得(新しいfdを得る具体的な処理はsocketwrapper.cpp)
        new_sockfd=socket(AF_INET,SOCK_STREAM,0);
	
         
	//念のため、flagのreset
	//ortedNewfd_is=0;

        vector<int32_t> _dumyFds_orted;
        _dumyFds_orted.push_back(new_sockfd);
	newfds.push_back(new_sockfd);
        //_dumyFds_orted.push_back(24);
        JTRACE("under orted,new receive_fds is set")(ortedNewfd_is)(_dumyFds_orted[0])(newfds[0]);
        Util::dupFds(fd,_dumyFds_orted);
       }
      JTRACE("restoring incoming connection") (id);
    } // org:e
    else{ // nomal_proc_ver (default function)
      JTRACE("Before dupFds_nomal_ver")(fd)((i->second)->getFds());
      Util::dupFds(fd, (i->second)->getFds());
      if(compareFlag == true){
	std::vector< ConnectionIdentifier >::iterator isMatchConId = find(changedConId.begin(),changedConId.end(),id); 
	if(isMatchConId != changedConId.end()){//Match
	  checkNum++;
	  JTRACE("restoring changed incoming connection")(id)(checkNum);
	  conList->erase(i);
	  if(checkNum == numOfMissingCon) break;
	}
      }else{
	JTRACE("restoring default incoming connection")(id);
	conList->erase(i);
      }
    }
    
  }// while
}

void dmtcp::ConnectionRewirer::doReconnect()
{
  // int num = dmtcp::ProcessInfo::instance().getNumOfChildProc(); // -> dmtcp::ProcessInfo’ has not been declared
  // if(parentproc == 1) ptsmx = 2;
  // else if(parentproc == 0) ptsmx = 0;
  // numofBlanceProc = (numofProcess -1)- ptsmx;//sawada add
  
  //親プロセスだけnumofProcess !=0
  numofBlanceProc = (numofProcess -1) - numOfChildProc;
  JTRACE("called:dmtcp::ConnectionRewirer::doReconnect()")(numofProcess)(numofBlanceProc)(numOfChildProc);
  JASSERT(numofProcess>numofBlanceProc).Text("Bad num of BlanceProc");
  
  //自分自身のhostidを除いたhostidが入っている
  JTRACE("check RecvHostIds")(RecvHostIds.size())(fdsAndHostid.size());

  // debag print:start  
  uint count;
  for(count=0;count<RecvHostIds.size();count++){
    JTRACE("debag each hostid of RecvHostIds")(RecvHostIds[count])(count);
  }
  
  map<uint64_t,int32_t>::iterator debag;
  for (debag = fdsAndHostid.begin(); debag != fdsAndHostid.end(); debag++) {
    uint64_t debaghostid = debag->first;
    int32_t debagfd = debag->second;
    JTRACE("debag fdsAndHostid")(debaghostid)(debagfd);
  }
  // debag:end

  //fdsAndHostidからclearするconnection/fdの抽出
  //orterun用
  //orterunのfdsAndHostidには脱退したorted用receive fdしか残らない
  if(isorterun == 1 && parentproc == 1){
    uint count;
    for(count=0;count<RecvHostIds.size();count++){
       JTRACE("each of RecvHostIds")(count)(RecvHostIds[count]);
       int32_t checkfd1;
      
       if(fdsAndHostid.find(RecvHostIds[count]) != fdsAndHostid.end()){// 理論上100%ある
	 checkfd1 = fdsAndHostid[(RecvHostIds[count])];
	 JTRACE("before fdsAndHostid.erase")(fdsAndHostid.size());
	 fdsAndHostid.erase((RecvHostIds[count]));
	 JTRACE("sender is alive. receiver side'fd = checkfd1")(checkfd1)(fdsAndHostid.size());
       }else{
	 JTRACE("error　!!!!!!!!!!!!!!!!!!!");
	 exit(1);
       }
     
    }//for( ~
    
    //debag print:start
    map<uint64_t,int32_t>::iterator debag;
    for (debag = fdsAndHostid.begin(); debag != fdsAndHostid.end(); debag++) {
       uint64_t debaghostid = debag->first;
       int32_t debagfd = debag->second;
       JTRACE("debag fdsAndHostid:this is shotdown fd coming from left orted")(debaghostid)(debagfd);
    }
    //debag:end

  }//if(isorterun == 1

  iterator i;
  for (i = _pendingOutgoing.begin(); i != _pendingOutgoing.end(); i++) {
    const ConnectionIdentifier& id = i->first;
    Connection *con = i->second;
    struct RemoteAddr& remoteAddr = _remoteInfo[id];
    int fd = con->getFds()[0];
    errno = 0;
    /*--debug用--*/
    sockaddr_in *sn = (sockaddr_in*) &remoteAddr.addr;
    unsigned int port = htons(sn->sin_port);
    char *ip = inet_ntoa(sn->sin_addr);
    char *port_string=(char*)&port;
    int port_len=strlen(port_string);
    /*----*/
    // sawada original :start
    JTRACE("check 2016_1_15")(isorterun)(blanceproc)(hostidOfleftNode)(id.hostid());
    
    if(ConOfLeftOrted == id.conId()){
      JTRACE("[orted --> orted]:sender ")(id)(con->getFds()[0])(ConOfLeftOrted);
      NotusingconnectionForOrted.push_back(con->getFds()[0]);
      continue;
    }
    int64_t fcntlFlags = fcntl(fd,F_GETFL);//sawada
    JTRACE("Before _real_connect(nomal_ver):")(id)(fd)(sn->sin_family)(port)(ip)(port_len)(fcntlFlags)(con->hasLock());
    JASSERT(_real_connect(fd, (sockaddr*) &remoteAddr.addr, remoteAddr.len)
            == 0)
      (id)(errno) (JASSERT_ERRNO) .Text("failed to restore connection");
    
    // original :end

    JASSERT(Util::writeAll(fd, &id, sizeof id));
    JTRACE("real_connect success! and writeAll is done 2015/12/2")(fd)(id);
    std::vector< ConnectionIdentifier >::iterator isMatchConId = find(changedConId2.begin(),changedConId2.end(),id); 
    if(isMatchConId != changedConId.end()){//Match
      //JTRACE("changed outgoing connection")(id);
      //char msg[6] = "hello";
      //JASSERT(Util::writeAll(fd, &msg, sizeof msg));
    }

    checkForPendingIncoming(PROTECTED_RESTORE_IP4_SOCK_FD,
                            &_pendingIP4Incoming);
    checkForPendingIncoming(PROTECTED_RESTORE_IP6_SOCK_FD,
                            &_pendingIP6Incoming);
    checkForPendingIncoming(PROTECTED_RESTORE_UDS_SOCK_FD,
                            &_pendingUDSIncoming);

    JTRACE("After checkpendingIncoming");
  }
  _pendingOutgoing.clear();
  _remoteInfo.clear();
  JTRACE("Before if (_pendingIP4Incoming.size() > 0)")(_pendingIP4Incoming.size())(_pendingUDSIncoming.size())(numofBlanceProc);
  int clearFlag=0;

  // sawada original :start
  if (_pendingIP4Incoming.size() > 0) {
    // Add O_NONBLOCK flag to the listener sockets.
    JTRACE("if(_pendingIP4Incoming.size() > 0) -> do!!!!!!!!!!!!!!!!!!!")(_pendingIP4Incoming.size());
    
    
    JTRACE("check:before debag")(fdsAndHostid.size());
    vector<int32_t> shotdownfds;
    
    //case1:ortedはdefaultで空,but -> when orted[sender] --> orted[receive],sender is not alive,I am receiver.
    //case2:when orted[sender] --> orted[receive],sender is alive
    //case3:orterun or 元々通信していたorted[sender]が脱退
    if(fdsAndHostid.size()!=0){
      int iscase2 = false;
      uint count;
      for(count=0;count<RecvHostIds.size();count++){
	JTRACE("each of RecvHostIds")(count)(RecvHostIds[count]);
      
	if(fdsAndHostid.find(RecvHostIds[count]) != fdsAndHostid.end()){//case2
	  JTRACE("orted[sender] --> orted[receive]:sender is alive")(fdsAndHostid[(RecvHostIds[count])]);
	  iscase2 = true;//blockingへ移行する必要がある
	}
      }
      if(iscase2 == false){
	map<uint64_t,int32_t>::iterator checkfdsAndHostid = fdsAndHostid.begin();
	// orterun,ortedともにclearするfdをshotdownfdsに記録
	while(checkfdsAndHostid != fdsAndHostid.end()){//case1,case3
	  uint64_t checkhostid = checkfdsAndHostid->first;
	  int32_t leftfd = checkfdsAndHostid->second;
	  shotdownfds.push_back(leftfd);
	  JTRACE("sender is not alive!! Do clear this fd")(checkhostid)(leftfd)(shotdownfds.back());
	  checkfdsAndHostid++;//これを忘れていてずっとloopから抜けれなかった笑
	}
      }//if(iscase2 == false)

    }//if(fdsAndHostid ~
    
    iterator j = _pendingIP4Incoming.begin();
    //bool isonlyMustFd = false;
	
    // while :_pendingIP4Incomingから復元しないfdを取り除く
    // left connectionについてそれぞれチェックしていく
    while(j != _pendingIP4Incoming.end()) {
     // [要blocking connection] or [clear connection(通信相手が脱退)] 
     const ConnectionIdentifier& left_id = j->first;
     Connection *left_con = j->second;
     vector<int32_t>  judgeFds=left_con->getFds();//loopのたびにjudgeを宣言しているので要素は毎回1個
     
     JTRACE("left connectionID")(left_id)(left_con->getFds())(judgeFds[0]);
     
     if((judgeFds.empty() == false) && (parentproc == 1)){//parentプロセス用 orturn and orted
       clearFlag=1;
       JTRACE("In if((judgeFds.empty() == false) ~");
       vector<int32_t>::iterator clearfd = find(shotdownfds.begin(),shotdownfds.end(),judgeFds[0]);
       if(clearfd != shotdownfds.end()) {// mapに残っていたfd(judgeFds)はclearすべきfdだった
        iterator temp = j;
	 j++;
	 _pendingIP4Incoming.erase(temp);//mapからそのconnecitonをremove
         
        NotusingconnectionForOrted.push_back(judgeFds[0]);//restoreしないfdに登録
        removefdsOfparentproc = left_con->getFds();
	JTRACE("confirm not using fd")(removefdsOfparentproc[0])(NotusingconnectionForOrted[0]);
        
        }else{
          if (_pendingIP4Incoming.size() > 0) ++j;
          if (_pendingIP4Incoming.size() == 0) JTRACE("_pendingIP4Incoming.size() == 0");
        }

     }// if((judgeFds~
     else{// child proc ver...clear connection(通信相手が脱退)はありえない.要blocking connection
       JTRACE("On child proc,next in: if(clearFlag!=1){ ")(clearFlag);
       break;
     }
     JTRACE("next left connection in _pendingIP4Incoming");
    }// while(j != _pendingIP4 ~
    
    bool checksum = false;
    JTRACE("we remove fd to clear.we has only fd with blocking ")(_pendingIP4Incoming.size());
    if(clearFlag!=1 || _pendingIP4Incoming.size()>0){//子プロセスとparentプロセス用 
      JTRACE("if childproc  or orterun proc do this,it is successful!,next is blocking")(parentproc);
      //child procは問答無用でbloking || 使わないfdをclearしたparent procは残りのfdはblockingでしっかりcatch
      markSocketBlocking(PROTECTED_RESTORE_IP4_SOCK_FD);

      JTRACE("After markSocketBlocking");

      checkForPendingIncoming(PROTECTED_RESTORE_IP4_SOCK_FD,
                            &_pendingIP4Incoming);
      JTRACE("After second's checkForPendingIncoming");

      if(parentproc == 1 && numofBlanceProc > 0){//負荷分散プロセスと接続し損ねた場合:ここだと_pending>0のときじゃないとBalanceCheckForPendingIncoming()をやってくれない
        //orterunにおいて、orted用receive fdだけが残った場合,if(clear~ に到達する前に削除され
        //_pending=0となりBalanceCheckForPendingIncoming()してくれない
	//↑case 0122
      	markSocketBlocking(PROTECTED_RESTORE_IP4_SOCK_FD);
      	BalanceCheckForPendingIncoming(PROTECTED_RESTORE_IP4_SOCK_FD,
                            &_pendingIP4Incoming);
      	checksum = true;
      	JTRACE("After BalanceCheckForPendingIncoming")(checksum);
	//_real_close(PROTECTED_RESTORE_IP4_SOCK_FD);
	//JTRACE("After _real_close");
      }

      //_real_close(PROTECTED_RESTORE_IP4_SOCK_FD);
      //JTRACE("After _real_close");

    }// if(clearFlag!=1 ~
    if(parentproc == 1 && numofBlanceProc > 0 && _pendingIP4Incoming.size() == 0 && checksum == false){//For case 0122
      markSocketBlocking(PROTECTED_RESTORE_IP4_SOCK_FD);
      BalanceCheckForPendingIncoming(PROTECTED_RESTORE_IP4_SOCK_FD,
                            &_pendingIP4Incoming);
      
    }
     _real_close(PROTECTED_RESTORE_IP4_SOCK_FD);
     JTRACE("After _real_close");
  }//  if (_pending~
  else if(_pendingIP4Incoming.size() == 0 && parentproc == 1 && numofBlanceProc > 0){
	markSocketBlocking(PROTECTED_RESTORE_IP4_SOCK_FD);
	BalanceCheckForPendingIncoming(PROTECTED_RESTORE_IP4_SOCK_FD,
                            &_pendingIP4Incoming);
	JTRACE("After BalanceCheckForPendingIncoming");
	_real_close(PROTECTED_RESTORE_IP4_SOCK_FD);
        JTRACE("After _real_close");
  }

  
  
  second_check = 1;
  // sawada original:end

  if (_pendingIP6Incoming.size() > 0) {
    // Add O_NONBLOCK flag to the listener sockets.
    markSocketBlocking(PROTECTED_RESTORE_IP6_SOCK_FD);
    checkForPendingIncoming(PROTECTED_RESTORE_IP6_SOCK_FD,
                            &_pendingIP6Incoming);
    _real_close(PROTECTED_RESTORE_IP6_SOCK_FD);
  }
  if (_pendingUDSIncoming.size() > 0) {
    // Add O_NONBLOCK flag to the listener sockets.

    // original: start
    // 親プロセスからよく分らないconnectionで接続するときの受信側fdをudsconnection[]に記録するだけ
    JTRACE("_pendingUDSIncoming.size() > 0 ")(_pendingUDSIncoming.size());
    iterator k;
    int x = 0;
    
    for (k = _pendingUDSIncoming.begin(); k!= _pendingUDSIncoming.end(); k++) {
      const ConnectionIdentifier& id_uds = k->first;
      Connection *con_uds = k->second;
      int fd_uds = con_uds->getFds()[0];//2015/12/15 [0]?
      
      //プロトコルチェンジしたコネクションが取そこねたのか?
      std::vector< ConnectionIdentifier >::iterator isMatchConId = find(changedConId.begin(),changedConId.end(),id_uds); 
      if(isMatchConId != changedConId.end()){//取り損ねたコネクションの数を記録
	numOfMissingCon++;
      }else{
	udsconnection.push_back(fd_uds);
      }
      JTRACE("check UDS connection")(id_uds)(fd_uds)(udsconnection[x])(x)(changedConId.size())(numOfMissingCon);
      x++;
    }
    // original: end

    markSocketBlocking(PROTECTED_RESTORE_UDS_SOCK_FD);
    checkForPendingIncoming(PROTECTED_RESTORE_UDS_SOCK_FD,
                            &_pendingUDSIncoming);
    
    JTRACE("After second's checkpendingIncoming,next is real_close" )(PROTECTED_RESTORE_UDS_SOCK_FD)(skipflag);
    _real_close(PROTECTED_RESTORE_UDS_SOCK_FD);
  }
  //JTRACE("Closed restore sockets");
}

void dmtcp::ConnectionRewirer::openRestoreSocket(bool hasIPv4Sock,
                                                 bool hasIPv6Sock,
                                                 bool hasUNIXSock)
{
  memset(&_ip4RestoreAddr, 0, sizeof(_ip4RestoreAddr));
  memset(&_ip6RestoreAddr, 0, sizeof(_ip6RestoreAddr));
  memset(&_udsRestoreAddr, 0, sizeof(_udsRestoreAddr));

  // Open IP4 Restore Socket
  if (hasIPv4Sock) {
    jalib::JServerSocket restoreSocket(jalib::JSockAddr::ANY, 0);
    JASSERT(restoreSocket.isValid());
    restoreSocket.changeFd(PROTECTED_RESTORE_IP4_SOCK_FD);

    // Setup restore socket for name service
    _ip4RestoreAddr.sin_family = AF_INET;
    dmtcp_get_local_ip_addr(&_ip4RestoreAddr.sin_addr);
    _ip4RestoreAddr.sin_port = htons(restoreSocket.port());
    _ip4RestoreAddrlen = sizeof(_ip4RestoreAddr);

    JTRACE("opened listen socket") (restoreSocket.sockfd())(PROTECTED_RESTORE_IP4_SOCK_FD)
      (inet_ntoa(_ip4RestoreAddr.sin_addr)) (ntohs(_ip4RestoreAddr.sin_port));
    markSocketNonBlocking(PROTECTED_RESTORE_IP4_SOCK_FD);
   
    // original:start

    JTRACE("procname")(procname);
    char deamonProcess1[100]="DMTCP:mpirun";
    char deamonProcess2[100]="DMTCP:orted";
    
    // A :start 
    char *ip=inet_ntoa(_ip4RestoreAddr.sin_addr);
    
    char temp_ip[20]="160.12.172.5";
    if(strcmp(ip,temp_ip)==0){
      strcpy(hostip,ip);
      JTRACE("I am under joker")(hostip);
      isorterun=1;
      if(strcmp(deamonProcess1,procname)==0){
	JTRACE("I am host deamon");
	parentproc = 1;
      }
      
    }else{
      JTRACE("I am under remote node ");
      isorterun=0;
      if(strcmp(deamonProcess2,procname)==0){
	JTRACE("I am remote deamon");
	parentproc = 1;
      }
    }
    //A :end
    
    // original: end
  }

  // Open IP6 Restore Socket
  if (hasIPv6Sock) {
    int ip6fd = _real_socket(AF_INET6, SOCK_STREAM, 0);
    JASSERT(ip6fd != -1) (JASSERT_ERRNO);

    _ip6RestoreAddr.sin6_family = AF_INET6;
    _ip6RestoreAddr.sin6_port = 0;
    _ip6RestoreAddr.sin6_addr = in6addr_any;
    _ip6RestoreAddrlen = sizeof(_ip6RestoreAddr);
    JASSERT(_real_bind(ip6fd, (struct sockaddr*) &_ip6RestoreAddr,
                       _ip6RestoreAddrlen) == 0)
      (JASSERT_ERRNO);
    JASSERT(getsockname(ip6fd, (struct sockaddr*)&_ip6RestoreAddr,
                        &_ip6RestoreAddrlen) == 0)
      (JASSERT_ERRNO);
    JASSERT(_real_listen(ip6fd, 32) == 0) (JASSERT_ERRNO);
    Util::changeFd(ip6fd, PROTECTED_RESTORE_IP6_SOCK_FD);

    JTRACE("opened ip6 listen socket") (PROTECTED_RESTORE_IP6_SOCK_FD);
    markSocketNonBlocking(PROTECTED_RESTORE_IP6_SOCK_FD);
  }

  // Open UDS Restore Socket
  if (hasUNIXSock) {
    dmtcp::ostringstream o;
    o << dmtcp_get_uniquepid_str() << "_" << dmtcp_get_coordinator_timestamp();
    dmtcp::string str = o.str();
    int udsfd = _real_socket(AF_UNIX, SOCK_STREAM, 0);
    
    JASSERT(udsfd != -1);
    memset(&_udsRestoreAddr, 0, sizeof(struct sockaddr_un));
    _udsRestoreAddr.sun_family = AF_UNIX;
    strncpy(&_udsRestoreAddr.sun_path[1], str.c_str(), str.length());
    _udsRestoreAddrlen = sizeof(sa_family_t) + str.length() + 1;
    JASSERT(_real_bind(udsfd, (struct sockaddr*) &_udsRestoreAddr,
                       _udsRestoreAddrlen) == 0)
      (JASSERT_ERRNO);
    JASSERT(_real_listen(udsfd, 32) == 0) (JASSERT_ERRNO);
    Util::changeFd(udsfd, PROTECTED_RESTORE_UDS_SOCK_FD);

    JTRACE("opened UDS listen socket")
      (udsfd)(PROTECTED_RESTORE_UDS_SOCK_FD) (&_udsRestoreAddr.sun_path[1]);
    markSocketNonBlocking(PROTECTED_RESTORE_UDS_SOCK_FD);
  }
}

void
dmtcp::ConnectionRewirer::registerIncoming(const ConnectionIdentifier& local,
                                           Connection* con,
                                           int domain)
{
  JASSERT(domain == AF_INET || domain == AF_INET6 || domain == AF_UNIX)
    (domain) .Text("Unsupported domain.");

  if (domain == AF_INET) {
    _pendingIP4Incoming[local] = con;
  } else if (domain == AF_INET6) {
#ifdef ENABLE_IP6_SUPPORT
    _pendingIP6Incoming[local] = con;
#else
    _pendingIP4Incoming[local] = con;
#endif
  } else if (domain == AF_UNIX) {
    _pendingUDSIncoming[local] = con;
  } else {
    JASSERT(false) .Text("Not implemented");
  }

  JTRACE("announcing pending incoming") (local);
}

void
dmtcp::ConnectionRewirer::registerOutgoing(const ConnectionIdentifier& remote,
                                           Connection* con)
{
  _pendingOutgoing[remote] = con;
  JTRACE("announcing pending outgoing") (remote);
}

void
dmtcp::ConnectionRewirer::registerConInfo(const ConnectionIdentifier& local,
					  const ConnectionIdentifier& remote,int domain)
{
  _endpointConnectionInfo[local] = remote;
  JTRACE("announcing incoming & outgoing") (domain)(local)(remote)(local.pid())(remote.pid());
}


void dmtcp::ConnectionRewirer::registerNSData()
{
  
  //sawada org :start
  
 
  JTRACE("called ConnectionRewirer::registerNSData(PidIpSet *procLocation)")(parentproc)(_pendingIP4Incoming.size())(_pendingOutgoing.size())(_pendingUDSIncoming.size());
  if(sProcessLocation.size() > 0 &&  parentproc == 0 ) changeConnection(&sProcessLocation);
  
  //sawada org :end
  
  registerNSData((void*)&_ip4RestoreAddr, _ip4RestoreAddrlen,
                 &_pendingIP4Incoming);
  JTRACE("After registerNSData() IPv4");
  registerNSData((void*)&_ip6RestoreAddr, _ip6RestoreAddrlen,
                 &_pendingIP6Incoming);
  registerNSData((void*)&_udsRestoreAddr, _udsRestoreAddrlen,
                 &_pendingUDSIncoming);
  JTRACE("After registerNSData() UDS");
}

// sawada original
void dmtcp::ConnectionRewirer::changeConnection(std::string *sProcLocation)
{
  
  iterator j;
  iterator k;
  iterator l;
  int i;
  int index = 0;
  int procNum;
  int outgoingSize =  _pendingOutgoing.size();
  int infoNum;
  
  printConList();
  dmtcp_get_local_ip_addr(&ipAddr);
  // conType...tcp,udsに関わらず「65536」(connection.hで定義)
  // const std::string tempStr = sProcessLocation;  
  std::vector<std::string> eachInfo = split(sProcessLocation,'/');
  //std::vector<std::string> eachInfo = dmtcp::Util::split(sProcessLocation,'/');
  JASSERT(eachInfo.size() == 3)(eachInfo.size()) .Text("we must have 3 info.PID and IP,isParent");
  
  
  

  infoNum = eachInfo.size();
  for(i=0;i<infoNum;i++){
    JTRACE("split:level 1")(eachInfo[i])(*sProcLocation);
  }
  
  std::vector<int> pidArray = split_int(eachInfo[0],'+');
  std::vector<std::string> ipArray = split(eachInfo[1],'+');
  std::vector<int> isParentArray = split_int(eachInfo[2],'+');
  // std::vector<int> pidArray = dmtcp::Util::split_int(eachInfo[0],'+');
  // std::vector<std::string> ipArray = dmtcp::Util::split(eachInfo[1],'+');
  // std::vector<int> isParentArray = dmtcp::Util::split_int(eachInfo[2],'+');

  JASSERT(pidArray.size() == ipArray.size() &&  ipArray.size() == isParentArray.size())(pidArray.size())(ipArray.size())(isParentArray.size()).Text("wrong num of ecah 3 info(PID and IP,isParent)");

  procNum = pidArray.size();
  for(i = 0;i<procNum;i++){
    JTRACE("split:level 2")(pidArray[i])(ipArray[i])(isParentArray[i])(procNum);
  }  
  
  
  //受けてのコネクションの変更(tcp_ver)
  j = _pendingIP4Incoming.begin();
  
  while( j != _pendingIP4Incoming.end()) {
    const ConnectionIdentifier& id = j->first;
    Connection *con = j->second;
    vector<int32_t> fds = con->getFds();
    const ConnectionIdentifier& remoteCon = _endpointConnectionInfo[id];//接続要求元(remtoe proc)

    for(i = 0;i<procNum;i++){
      if(pidArray[i] == remoteCon.pid() ){
	index = i;
	break;
      }else{
	index = -1;
      }
    }      
    
    //remote procのいるIPと、このプロセスがいるIPの比較
    int result;
    if(index != -1) result = strcmp(ipArray[index].c_str(),inet_ntoa(ipAddr));
    
    JTRACE("check _pendingIP4Incoming")(id)(id.pid())(fds[0])(remoteCon)(remoteCon.pid())(_pendingIP4Incoming.size())(result)(index);
    //TCPコネクションだが、実は同じノードだった場合
    if(result == 0 )  {
      //_real_close(fd); //fdを閉じてからsocket()したからと言って、小さい順にならってfdと同じ数字で新しくfdが作られる分けではない
      //accept用に取っといてあるfdの数字で作られてしまう

      _real_close(fds[0]);
      int32_t new_fd = _real_socket(AF_UNIX, SOCK_STREAM, 0);
      Util::dupFds(new_fd, fds);//dup2の後、第一引数のfdを閉じるところまでやっていくれる
      con->setConType(Connection::TCP);
      registerIncoming(id,con,AF_UNIX);
      iterator temp = j;
      ++j;
      _pendingIP4Incoming.erase(temp);
      changedConId.push_back(id);
      JTRACE("default fd & new fd:IPv4Incoming")(fds[0])(new_fd)(changedConId.back());
      JTRACE("Location of remote proc ")(index)(ipArray[index].c_str())(inet_ntoa(ipAddr));
    }else{
      ++j;
     
    }
    
    
    
  }
  JTRACE("end changing accept:TCP -> UDS");

  k = _pendingUDSIncoming.begin();
  
  //受けてのコネクションの変更(uds_ver)
  while(k != _pendingUDSIncoming.end()){
    const ConnectionIdentifier& id = k->first;
    Connection *con = k->second;
    //int fd = con->getFds()[0];
    vector<int32_t> fds = con->getFds();
    const ConnectionIdentifier& remoteCon = _endpointConnectionInfo[id];
    
    for(i = 0;i<procNum;i++){
      if(pidArray[i] == remoteCon.pid() ){
	index = i;
	break;
      }else{
	index = -1;
      }
    }   
    
    //remote procのいるIPと、このプロセスがいるIPの比較
    //int result = strcmp(procLocation->ipSet[index],inet_ntoa(ipAddr));
    int result;
    if(index != -1) result = strcmp(ipArray[index].c_str(),inet_ntoa(ipAddr));
    

    //JTRACE("check _pendingUDSIncoming")(id)(id.pid())(fds[0])(remoteCon)(remoteCon.pid())(result);
    
    //udsコネクションだが、実は違うノードだった場合
    if( (result != 0 && isParentArray[index] != 1)  && index != -1 ){
      JTRACE("change UDS -> TCP!!!!!!!!");
      _real_close(fds[0]);
      int32_t new_fd = _real_socket(AF_INET, SOCK_STREAM, 0);
      Util::dupFds(new_fd, fds);//dup2の後、第一引数のfdを閉じるところまでやっていくれる
      con->setConType(Connection::TCP);
      registerIncoming(id,con,AF_INET);
      iterator temp = k;
      ++k;
      _pendingUDSIncoming.erase(temp);
      JTRACE("default fd & new fd:UDSIncoming")(fds[0])(new_fd);
      JTRACE("Location of remote proc ")(index)(ipArray[index].c_str())(inet_ntoa(ipAddr))(result);
    }else{
      k++;
    }
    
  }
 JTRACE("end changing accept:UDS -> TCP");

 //接続要求側のコネクションの変更
  for (l = _pendingOutgoing.begin(); l != _pendingOutgoing.end(); l++) {
    const ConnectionIdentifier& id = l->first;
    Connection *con = l->second;
    vector<int32_t> fds = con->getFds();
    
    for(i = 0;i<procNum;i++){
      if(pidArray[i] == id.pid() ){
	index = i;
	break;
      }else{
	index = -1;
      }
    }    

    //接続要求(connect)を出すプロセスのIPと、このプロセスがいるIPの比較
    int result;   
    if(index != -1) result = strcmp(ipArray[index].c_str(),inet_ntoa(ipAddr));

    // JTRACE("check _pendingOutgoing")(id)(id.pid())(fds[0])(con->conType())(index);

    //Connection::UDS = 73728 ...sockDomain = unix domain
    //Connection::TCP = 65536...default


    //接続先のプロセスのIPが同一ノード内だった場合 かつ ドメインがunix domain socketではない(=tcp)の場合 
    // かつ 接続要求先が親プロセスでない(注意:子から親プロセスへのtcpコネクションが引っかかってしまう)
    if( (result == 0  && con->conType() != Connection::UDS) && (isParentArray[index] != 1 && index != -1))    
    {
      _real_close(fds[0]);
      int32_t new_fd = _real_socket(AF_UNIX, SOCK_STREAM, 0);
      Util::dupFds(new_fd, fds);//dupp2の後、第一引数のfdを閉じるところまでやっていくれる
      con->setConType(Connection::TCP);
      changedConId2.push_back(id);
      JTRACE("default fd & new fd:Outgoing TCP -> UDS")(fds[0])(new_fd);
      JTRACE("Location of remote proc ")(index)(ipArray[index].c_str())(inet_ntoa(ipAddr))(result);
    }
    //接続要求先プロセスが別のノード　かつ　ドメインがUDSの場合:もともとノード内通信だったものがノード間通信となった
    else if( (result !=0 && con->conType() == Connection::UDS) && (isParentArray[index] != 1 && index != -1) ){
      _real_close(fds[0]);
      int32_t new_fd = _real_socket(AF_INET, SOCK_STREAM, 0);
      Util::dupFds(new_fd, fds);
      con->setConType(Connection::TCP);
      changedConId2.push_back(id);
      JTRACE("default fd & new fd:Outgoing UDS -> TCP")(fds[0])(new_fd);
      JTRACE("Location of remote proc ")(index)(ipArray[index].c_str())(inet_ntoa(ipAddr))(result);
    }
    
  }
  JTRACE("end changing connet:TCP -> UDS or UDS -> TCP")(outgoingSize)(_pendingOutgoing.size());
  
  printConList();

}

void dmtcp::ConnectionRewirer::registerNSData(void            *addr,
                                              socklen_t        addrLen,
                                              ConnectionListT *conList)
{
  JTRACE("called registerNSData(addr,addrlen,conlist)");
  iterator i;
  JASSERT(theRewirer != NULL);
  for (i = conList->begin(); i != conList->end(); ++i) {
    const ConnectionIdentifier& id = i->first;
    dmtcp_send_key_val_pair_to_coordinator("Socket",
                                           (const void *)&id,
                                           (uint32_t) sizeof(id),
                                           addr,
                                           (uint32_t) addrLen);
    
    sockaddr_in *sn = (sockaddr_in*) addr;
    unsigned short port = htons(sn->sin_port);
    char *ip = inet_ntoa(sn->sin_addr);
    JTRACE("Send NS information:")(id)(sn->sin_family)(port)(ip);
    
  }
  //debugPrint();
  JTRACE("end registerNSData(addr,addrlen,conlist)");
  
}

void dmtcp::ConnectionRewirer::sendQueries()
{
  iterator i;
  for (i = _pendingOutgoing.begin(); i != _pendingOutgoing.end(); ++i) {
    const ConnectionIdentifier& id = i->first;
    struct RemoteAddr remote;
    uint32_t len = sizeof(remote.addr);

    //struct in_addr ipAddr;// sawada:off 2016/1/19 -> staticへ
    // ↓これをoffると、coordinator側のlookupが失敗する
    dmtcp_get_local_ip_addr(&ipAddr);//sawada:off at 2016/06/08
    JTRACE("Before dmtcp_send_query_to_coordinator")(inet_ntoa(ipAddr))(ipAddr.s_addr)(sizeof(ipAddr))(sizeof(ipAddr.s_addr))(id.pid())(id.conId());//sawada
    dmtcp_send_query_to_coordinator("Socket",
                                    (const void *)&id, (uint32_t) sizeof(id),
                                    &remote.addr, &len,(const void*)&ipAddr);
    remote.len = len;
    
    sockaddr_in *sn = (sockaddr_in*) &remote.addr;
    unsigned short port = htons(sn->sin_port);
    char *ip = inet_ntoa(sn->sin_addr);
    JTRACE("Send Queries. Get remote from coordinator:")(id)(sn->sin_family)(port)(ip)(balanceproc20151118);
    if(balanceproc20151118 == 1){//left node's child proc or orted[sender]->orted[receiver]'s orted
      blanceproc = 1;
      hostidOfleftNode = id.hostid();
      JTRACE("set:blanceproc = 1")(blanceproc)(hostidOfleftNode);
      // if(isorterun == 1){//On host node, left node's child proc is.
      // 	 FILE *file;
      //    file=fopen("info_balanceproc.txt","w");
      // 	 //脱退したnode内ortedへのconnectionのhostid(つまり脱退nodeのhostid)をファイルに書き込む
      //    int writebyte = fprintf(file,"%llu\n",hostidOfleftNode);    
      //    fclose(file); 
      // 	 JTRACE("under host node,write hostid")(hostidOfleftNode)(writebyte);
      // }
    }
    if(isorterun == 0 && parentproc == 1 &&  balanceproc20151118==1){
      blanceproc = 0;
      balanceproc20151118 = 0;
      ConOfLeftOrted = id.conId(); 
      JTRACE("[orted --> orted]:sender ")(id)(ConOfLeftOrted);
    
    }
    
    _remoteInfo[id] = remote;
  }
}


//sawada org:start

void dmtcp::ConnectionRewirer::printConList()
{
  ostringstream o;
  o << '\n';
  o << "Pending Incoming:IPv4\n";
  iterator i;
  iterator j;
  iterator k;
  for (i = _pendingIP4Incoming.begin(); i != _pendingIP4Incoming.end(); ++i) {
    Connection *con = i->second;
    o << " " << i->first << " : firstFd=" << con->getFds()[0] << '\n';
  }
  o << "Pending Incoming:UDS\n";
  for (j = _pendingUDSIncoming.begin(); j != _pendingUDSIncoming.end(); ++j) {
    Connection *con = j->second;
    o << " " << j->first << " : firstFd=" << con->getFds()[0] << '\n';
  }
  o << "Pending Outgoing:\n";
  for (k = _pendingOutgoing.begin(); k != _pendingOutgoing.end(); ++k) {
    Connection *con = k->second;
    o << " " << k->first << " : firstFd=" << con->getFds()[0] << '\n';
  }
  JTRACE("check Connection Lists")(o.str());
}

/* ToDo:再利用性があるのでここではなく,utility関数を定義するソースに記述する方がBest*/
std::vector<std::string> split(const std::string &str, char sep)
{
    std::vector<std::string> v;
    std::stringstream ss(str);
    std::string buffer;
    while( std::getline(ss, buffer, sep) ) {
        v.push_back(buffer);
    }
    return v;
}

/* ToDo:再利用性があるのでここではなく,utility関数を定義するソースに記述する方がBest*/
std::vector<int> split_int(const std::string &str, char sep)
{
    std::vector<int> v;
    std::stringstream ss(str);
    std::string buffer;
    while( std::getline(ss, buffer, sep) ) {
      v.push_back( atoi(buffer.c_str()) );
    }
    return v;
}


//sawada org:end

#if 0
void dmtcp::ConnectionRewirer::debugPrint() const
{
#ifdef DEBUG
  ostringstream o;
  o << "Pending Incoming:\n";
  const_iterator i;
  for (i = _pendingIncoming.begin(); i!=_pendingIncoming.end(); ++i) {
    Connection *con = i->second;
    o << i->first << " numFds=" << con->getFds().size()
      << " firstFd=" << con->getFds()[0] << '\n';
  }
  o << "Pending Outgoing:\n";
  for (i = _pendingOutgoing.begin(); i!=_pendingOutgoing.end(); ++i) {
    Connection *con = i->second;
    o << i->first << " numFds=" << con->getFds().size()
      << " firstFd=" << con->getFds()[0] << '\n';
  }
  JNOTE("Pending connections") (o.str());
#endif
}

#endif
