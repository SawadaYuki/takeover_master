/****************************************************************************
 *   Copyright (C) 2006-2013 by Jason Ansel, Kapil Arya, and Gene Cooperman *
 *   jansel@csail.mit.edu, kapil@ccs.neu.edu, gene@ccs.neu.edu              *
 *                                                                          *
 *  This file is part of DMTCP.                                             *
 *                                                                          *
 *  DMTCP is free software: you can redistribute it and/or                  *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP is distributed in the hope that it will be useful,                *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#include "lookup_service.h"
#include "../jalib/jassert.h"
#include "../jalib/jsocket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>//sawada
#include <algorithm>//sawada
#include <sys/time.h>//sawad add

using namespace dmtcp;

//sawada org:s 
map<dmtcp::string,unsigned short>  IpAndPort;//デバッグ用
map<dmtcp::string,sockaddr_in*>  IpAndVal;//msg送り主のIPと送信するVal(実態はアドレス情報)
map<dmtcp::string,KeyValue*>  IpAndKey;//msg送り主のIPとアドレス検索用のKey
static bool isLookupInfo=true;//負荷分散するプロセスがあるかどうかのフラグ

extern std::vector<dmtcp::UniquePid> parentUpid;//from dmtcp_coordinator.cpp
extern map<dmtcp::string,int> eachNode;//from dmtcp_coordinator.cpp
extern map<dmtcp::UniquePid,int> sentHostidForParent;
extern std::vector<uint64_t> HostIdofConnect;//from dmtcp_coordinator.cpp 全プロセス共有財産

//sawada org:e 


void dmtcp::LookupService::reset()
{
  //   DMT_DO_REGISTER_NAME_SERVICE_DATA        2     
  //JTRACE("called LookupService::reset");
  MapIterator i;
  for (i = _maps.begin(); i != _maps.end(); i++) {
    KeyValueMap &kvmap = i->second;
    KeyValueMap::iterator it;
    for (it = kvmap.begin(); it != kvmap.end(); it++) {
      KeyValue *k = (KeyValue*)&(it->first);
      KeyValue *v = it->second;
      k->destroy();
      v->destroy();
      delete v;
    }
    kvmap.clear();
  }
  _maps.clear();
}

void dmtcp::LookupService::addKeyValue(string id,
                                       const void *key, size_t keyLen,
                                       const void *val, size_t valLen, UniquePid from)
{
  JTRACE("hello,2015/12/1");
  KeyValueMap &kvmap = _maps[id];// id = Socket

  KeyValue k(key, keyLen);
  KeyValue *v = new KeyValue(val, valLen);
  if (kvmap.find(k) != kvmap.end()) {
    JTRACE("Duplicate key");
  }
  kvmap[k] = v;
  //JTRACE("addKeyvalue,kvmap")(&kvmap[k])(from);
}

void dmtcp::LookupService::query(string id,
                                 const void *key, size_t keyLen,
                                 void **val, size_t *valLen,const char *ip)
{

  KeyValueMap &kvmap = _maps[id];
  KeyValue k(key, keyLen);

  
  if (kvmap.find(k) == kvmap.end()) {
    //sawada original:start
    JTRACE("Lookup Failed(first), Key not found")(ip)(IpAndKey[ip]);
    isLookupInfo = false;

    //再配置先ノードの親プロセスへアクセスするためのアドレス情報を取得するためのキーを用意
    KeyValue *moreKey=IpAndKey[ip];
    //sawada original:end
    JTRACE("second CHECK!");
    if(kvmap.find(*moreKey) == kvmap.end()){
      JTRACE("Lookup Failed(second), Key not found")(ip)(moreKey);
      // default dmtcp:start
      *val = NULL;
      *valLen = 0;
      return;
      // default dmtcp:end
    }
    else{//sawada original:start
      JTRACE("reLookup is success!");
      // default dmtcp:start
      KeyValue *v = kvmap[*moreKey]; //再配置先ノードの親プロセスへアクセスするためのアドレス情報を渡す
      *valLen = v->len();
      *val = new char[v->len()];
      memcpy(*val, v->data(), *valLen);
      // default dmtcp:end

      //moreKey->destroy();
      //delete moreKey;
      return;
    }//sawada original:end
  }
  

  /*
  if (kvmap.find(k) == kvmap.end()) {
    JTRACE("Lookup Failed, Key not found.");
    *val = NULL;
    *valLen = 0;
    return;
  }
  */
  KeyValue *v = kvmap[k];
  *valLen = v->len();
  *val = new char[v->len()];
  memcpy(*val, v->data(), *valLen);
}

void dmtcp::LookupService::registerData(const DmtcpMessage& msg,
                                        const void *data)
{

  JASSERT (msg.keyLen > 0 && msg.valLen > 0 &&
           msg.keyLen + msg.valLen == msg.extraBytes)
    (msg.keyLen) (msg.valLen) (msg.extraBytes);
 
  const void *key = data;

  const void *val = (char *)key + msg.keyLen;
  size_t keyLen = msg.keyLen;
  size_t valLen = msg.valLen;

  /*sawada ori:start for debug*/
  
  sockaddr_in *sn = (sockaddr_in*) val;
  unsigned short port = htons(sn->sin_port);
  char *ip = inet_ntoa(sn->sin_addr);
  
  /*ori:end*/


  //msg._magicBits = "DMTCP_CKPT_V0";
   JTRACE("--------DmtcpMessage:msg----------------------------------")(msg.from)(msg._magicBits)(msg.state)(msg.nsid)(key)(keyLen)(val)(valLen)(ip)(port);
  addKeyValue(msg.nsid, key, keyLen, val, valLen,msg.from);
  // sawad original: start
  int j;
  
  if(valLen == 16){//IPv4
    
    for(j=0;j<parentUpid.size();j++){
      JTRACE("check parent?")(j)(parentUpid[j]);
      if(parentUpid[j] == msg.from){ //msg送信元と記録している親プロセスのUPID群との照合
	//parentUpid.back();
	IpAndPort[ip]=port;
	IpAndVal[ip]=(sockaddr_in*)val; //IPアドレスと親プロセスへアクセスするアドレス情報の対応付け
	sockaddr_in *temp=IpAndVal[ip];

	//-----
	KeyValue *insertkey=new KeyValue(key, keyLen);
	IpAndKey[ip]=insertkey;  //IPアドレスと親プロセスへアクセスするアドレス情報を検出するためのキーの対応付け
	JTRACE("I am parent process,register ip and port")( IpAndPort[ip])(IpAndPort.size())(&IpAndVal[ip])(htons(temp->sin_port))(insertkey)(&insertkey);
	break;
      }
    }
  }
  // sawada original: end
}

void dmtcp::LookupService::respondToQuery(jalib::JSocket& remote,
                                          const DmtcpMessage& msg,
                                          const void *key)
{
  //JTRACE("respondquery top")(key);
  
  //sawada original:start for debug
  map<dmtcp::string,unsigned short>::iterator i;
  const char *ip = inet_ntoa(msg.ipAddr);
  for(i = IpAndPort.begin();i!=IpAndPort.end();++i){
    JTRACE("check IpAndPort on respondTOQuery")(i->first)(i->second)(inet_ntoa(msg.ipAddr))(ip);
   
  }
  JASSERT(strcmp(ip,"0.0.0.0") != 0) .Text("Failed getting sender's IP");
  // original:end
  

  JASSERT (msg.keyLen > 0 && msg.keyLen == msg.extraBytes)
    (msg.keyLen) (msg.extraBytes);
  void *val = NULL;
  size_t valLen = 0;

  query(msg.nsid, key, msg.keyLen, &val, &valLen,ip);

  DmtcpMessage reply (DMT_NAME_SERVICE_QUERY_RESPONSE);
  reply.keyLen = 0;
  reply.valLen = valLen;
  reply.extraBytes = reply.valLen;
   // sawada original :start
  /*
  char *id="b";
  strncpy(reply.nsid, id, 1);
  JTRACE("check reply.nsid")(reply.nsid);
  */
  if(isLookupInfo == false){//query()が失敗したら = 通信相手プロセスのアドレス情報を見つけられなかった = 負荷分散するプロセスがある

    unsigned short paretn_port= IpAndPort[inet_ntoa(msg.ipAddr)];
    sockaddr_in *temp=(sockaddr_in*)IpAndVal[ip];
    JTRACE("Port of distination node will be given ")(paretn_port)(msg.from)(inet_ntoa(msg.ipAddr))(&IpAndVal[ip])(htons(temp->sin_port));
    // 測るまでもない
    // -----------------------------------
    isLookupInfo = true;
    char *id="b"; //"balance"の意
    strncpy(reply.nsid, id, 1); //coordinatorから各プロセスに対して、負荷分散("balance")するプロセスがあることを知らせるため
     // -----------------------------------
    JTRACE("check reply.nsid")(reply.nsid);
  }
  std::vector<dmtcp::UniquePid>::iterator j = find(parentUpid.begin(),parentUpid.end(),msg.from);
  if(j!=parentUpid.end()){//親プロセスに対して
 
    dmtcp::UniquePid tempPid = *j;
    reply.numPeers = eachNode[ip];//親プロセスが起動しているノードで合計何プロセス起動しているかを知らせる

    // debag
    /*
    int l;
    for(l=0;l<HostIdofConnect.size();l++){
       JTRACE("check HostIdofConnect")(HostIdofConnect[l]);
    }
    */
    //

    //このrespondToQueryは複数回呼ばれるため、reply_msgにホストIDを潜り込ませて親プロセスへ知らせる
    if(sentHostidForParent[tempPid] < HostIdofConnect.size()) {
      int index = HostIdofConnect.size() - sentHostidForParent[tempPid];
      JTRACE("prepare hostid")(*j)(tempPid)(sentHostidForParent[tempPid])(index);
      if(tempPid.hostid()!=HostIdofConnect[index-1]) {
	reply.dumyhostid = HostIdofConnect[index-1];
      }else{
	reply.dumyhostid = 0;
      }
      //HostIdofConnect.pop_back();// HostIdofConnectは共有財産だから駄目
      int tempcount = sentHostidForParent[tempPid];
      tempcount++;
      sentHostidForParent[tempPid] = tempcount;
      JTRACE("Add msg to hostid")(sentHostidForParent[tempPid])(reply.dumyhostid)(HostIdofConnect[index-1]);
    }
    
  }
  //original :end
  
  remote << reply;

  //JTRACE("called respondToQuery")(&key)(msg.from)(msg.virtualPid)(msg.type)(reply.type);
 

  if (valLen > 0) {
    
    remote.writeAll((char*)val, valLen);
    // original start for debug
    sockaddr_in *sn = (sockaddr_in*) val;
    unsigned short port = htons(sn->sin_port);
    char *ip = inet_ntoa(sn->sin_addr);
    JTRACE("next:write val")(ip)(port);
    
   
    //end
  }else{
    JTRACE("Fail to remote.write.All")(valLen)(msg.from);
  }
  delete [] (char*)val;
}
