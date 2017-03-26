#できること
- プロセス単位で負荷分散/再配置が可能
- 通信の効率化(TCP <==> Unix Domain Socket)が可能

********************************************************************************

#注意点

- 縮退しか対応していない
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

********************************************************************************

#最新の進捗状況

- restart後もチェックポイントを取得できるよう改良を進めていたが、結果達成できていない
  - 原因:唯一エラー / warnnigを吐いたのは、再配置したプロセスのsession ID(_sid)やプロセスグループID(_gid)が古いという点.(2016年11月のwikiに掲載.processinfo.cpp内のrestoreProcessGroupInfo()を色々いじっていた)
  - 解決方法(予測):プロセスのfork時(dmtcp_restart.cpp)に、再配置したプロセスも含めて共通のセッションとし、セッションIDを統一する.加えて、チェックポイント時に記録したプロセスID,セッションIDとの帳尻を合わせる処理が必要かと思う.

********************************************************************************

#今後の展望
##MPIとDMTCPについて

- 様々な課題から、以前から話しているように「プロセス間の親子関係が原因のエラーが多い」

↓ゆえに

- 一番シンプルかつ簡単な方法は「orterunはあってもいいが、プロセスの移動が発生するリモートノードでは子プロセスのみであってほしい」
- ortedがなければプロセスの再配置(縮退と伸長の両方)が用意に実装できそう
- 「 restart後のチェックポイントデータの取得実現」のように、プロセスのセッションIDやプロセスIDが原因となるエラーも解消できそう

↓

- 試しに、MPIで上記を実現んできるような枠組み/機能がないか調べてみるのも良いかも

##プロジェクト全体
- 通信帯域が潤沢な場合でもノード間通信がネックなのは変わらないので、この改善手段が必要かも
- 我々のライバルである「分散型モバイルクラスタ」と異なり、本クラスタ(並列型モバイルクラスタ)にしかできないこと・絶対的な有利性を見つけるとぐっと良くなる or 今の並列型モバイルクラスタを改めて見直す

********************************************************************************


