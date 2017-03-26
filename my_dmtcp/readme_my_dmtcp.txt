#注意点

- 縮退しか対応していない
- 4ノードから1ノード脱退した場合のケース(orted間の通信が発生する場合)にも対応
- 脱退したノードのortedを復元しないため、MPIが正常終了しない
  - ゆえに時間計測ができないため、評価用のアプリケーション側(MPI_finalizeを呼ぶ直前に)で、"dmtcp_command -k"を呼び出し、dmtcp_coordinatorに無理やりプロセスを強制終了するようにして、何とか時間計測を可能としている状況
 - (補足)dmtcpのconfigureオプションで時間計測を可能とするもの(--enable-timing)があるが、リスタートにかかる時間の計測範囲が適切でない(プロセスの再会フェーズに突入する前に計測を終えている)と考えている
 - (補足)現状の計測はdmtcp_coordinator側で行っており、ログファイル(dmtcp_coordinator)内を"realSec"で検索するとリスタート後の実行時間が分かるようになっている

- NPBのプログラムにおいて、時間計測のための"MPI_Reduce()"が原因で正常終了しないことが分かっている
 - 厳密に言うと、MPI_Reduce関数で処理が止まってしまう
 - そこでMPI_Reduce関数をMPI_Allreduce関数に変更している.この変更はプログラム本来の処理に影響は与えず、途中で止まることはない.現在、止まる原因となるMPI_Reduce関数はMPI_Allreduce関数に変更している.

- ホストノードを変える場合はいちいち「src/plugin/ipc/socket/connectionrewirer.cpp内のopenRestoreSocket関数の一部」を変更し,make & make installする必要がある

void dmtcp::ConnectionRewirer::openRestoreSocket(bool hasIPv4Sock,
                                                 bool hasIPv6Sock,
                                                 bool hasUNIXSock)
{
 .
 .
 .
 char temp_ip[20]="160.12.172.3";    <---------------ここをホストノードのIPに変更する必要がある
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

- ソース内では固有名詞("orterun"や"oreted")を使っている部分があり、可用性がない



#
"+":追加の意
"-":概要/結論


#機能1:プロセス単位での負荷分散
##要件1. 親子関係にないプロセスのckptでも復元対象とする
- 修論:3.2.2節
- ソースコード:dmtcp\_restart.cpp:void createProcess(bool createIndependentRootProcesses = false)


        /* sawada */				     
        if(t->_pInfo.sid()!=_pInfo.sid() &&  _pInfo.procname()=="orterun"){
	  JTRACE("comming huka process,next createDependentChildProcess()");
          t->createDependentChildProcess();
          JTRACE("After remote process createDependentChildProcess()");
  
          }
        

        /* sawada */
        
        if(t->_pInfo.sid()!=_pInfo.sid() &&  _pInfo.procname()=="orted"){
	  JTRACE("comming huka process,next createDependentChildProcess()");
          t->createDependentChildProcess();
          JTRACE("After remote process createDependentChildProcess()");
        }

##要件2. 復元に成功する(特にプロセス間通信の再構築)
- 修論:3.2.3節
- ソースコード:dmtcp_restart.cpp:void createProcess(bool createIndependentRootProcesses = false)

#機能2:プロセス間通信の通信方法の切り替え

- src/dmtcp_restart.cpp
 + void createProcess(bool createIndependentRootProcesses = false)
   - 親子関係にないプロセスのckptでも復元対象とし, forkする

- src/coordinatorapi.cpp
  + 変数
  int balanceproc20151118 = 0;  => 負荷分散したプロセスがあるかどうか
  int numofProcess= 0; => リスタート用に復元したプロセスの数
  std::vector<uint64_t> RecvHostIds; => 生き残っているノードのホストID
  std::string sProcessLocation;// s(PID1+PID2+ ... /IP1+IP2+... /IsParent + IsParent +... => プロセスの情報(PID・IPアドレス・親プロセス)

  + void dmtcp::CoordinatorAPI::recvMsgFromCoordinator(dmtcp::DmtcpMessage *msg,
                                                   void **extraData)
    - プロセスの位置情報をcoordinatorからもらう

  else if(msg->type == DMT_DO_REGISTER_NAME_SERVICE_DATA && msg->state == WorkerState::RESTARTING && msg->extraBytes > 0 ){
    msg->assertValid();
    JASSERT(msg->extraBytes > 0);
    uint32_t size = msg->extraBytes;
    void *buf = JALLOC_HELPER_MALLOC(size);
    JTRACE("reading process location")(size);
    _coordinatorSocket.readAll((char*)buf, size);
    
    sProcessLocation = (char*)buf;
    JASSERT(sProcessLocation.size() > 0).Text("empty of process location info");
    JTRACE("read process location")((char*)buf)(sProcessLocation);
   
  }

  + void dmtcp::CoordinatorAPI::connectToCoordOnRestart(CoordinatorMode  mode,
                                                    dmtcp::string progname,
                                                    UniquePid compGroup,
                                                    int np,
                                                    CoordinatorInfo *coordInfo,
                                                    struct in_addr  *localIP)

    - 通信の切り替えオプションがonになったことを確認したら，coordinatorへその旨を知らせる

  if( getenv(ENV_FLAG_CHANGE_DOMAIN) != NULL ){
    if( strcmp(getenv(ENV_FLAG_CHANGE_DOMAIN),"true") == 0 )  {
      JTRACE("Next changeMsgType");
      hello_local.changeMsgType(DMT_RESTART_WORKER_WITH_CHG_DOMAIN);
    }
  } 

  + int dmtcp::CoordinatorAPI::sendQueryToCoordinator(const char *id,
                                                  const void *key,
                                                  uint32_t key_len,
                                                  void *val,
                                                  uint32_t *val_len,const void *myipAddr)

    - どのノードからのmsgか判断するために,ipアドレスをmsgに付加する

    memcpy((void*)&msg.ipAddr,myipAddr,sizeof myipAddr);//sawada

    - coordinatorからリスタート用に復元したプロセスの数を受け取る

    numofProcess = msg.numPeers;

    - クラスタに残っているノードを判断するために,生き残っているノードのホストIDをcoordinatorからもらう

    if(msg.dumyhostid != 0){
    RecvHostIds.push_back(msg.dumyhostid);
    JTRACE("get one hostid of hostids")(msg.dumyhostid)(RecvHostIds.back());
    }else{
	JTRACE("check msg.coordTimeStamp")(msg.dumyhostid);
    }

    - 負荷分散したプロセスがあるかどうかのフラグ情報をもらう
    if(strcmp(msg.nsid,"b") == 0){
    balanceproc20151118 = 1;
    JTRACE("detect:this proc is balance proc")(msg.nsid)(balanceproc20151118);
    }else{
    balanceproc20151118 = 0;
    }


- src/dmtcp_coordinator.cpp ok


- src/dmtcpmessagetypes.cpp ok

- src/lookup_service.cpp　ok

- src/processinfo.cpp △


- src/plugin/ipc/connection.h ok

- src/plugin/ipc/connectionlist.cpp ok

- src/plugin/ipc/connectionrewier.cpp ok

- src/plugin/ipc/socket/kernelbufferdrainer.cpp ok
