#使用例

$ ruby myruby.rb 3 1 j
   - 作業効率化のために、ほぼほぼ完成版のリスタート用のスクリプトを作成
   - 第一引数はlaunch時に起動していたノード数
   - 第二引数は脱退したノードの数
   - 第三引数はホストノードの頭文字(witch,deep,joker,neutrinoには対応)
     -(※)必要に応じてプログラムを改修してください

***************************(実行後作成してくれるrestartスクリプト例)***************************

#!/bin/sh

      dmtcp_restart --host joker.is.utsunomiya-u.ac.jp --port 7779 --interval 0  /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_orterun_22fa2f4d8b89e2f-40000-58d65abf.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_qn24b_mpi_last_syuron_22fa2f4d8b89e2f-53000-58d65ac0.dmtcp &

      /usr/bin/ssh neutrino.is.utsunomiya-u.ac.jp '/bin/sh -c '\''dmtcp_restart --host joker.is.utsunomiya-u.ac.jp --port 7779 --interval 0  /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_orted_3442c2f7b6cfb13a-49000-58d65ac0.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_qn24b_mpi_last_syuron_3442c2f7b6cfb13a-51000-58d65ac0.dmtcp '\''' & 

*****************************************************************************************

#注意点
##動作検証
- 「3ノードから1ノード脱退」、「2ノードから1ノード脱退」するケースにしか利用できない、というか動作検証していない

##書き換えが必要な箇所
-1. 49行目
	# remote nodeの決定
	remote3 = pegasus
	remote2 = zoro
	remote1 = neutrino


↓
クラスタの構成に応じてremote(1 or 2 or 3)を変更する
remote1 or remote2 or remote3が脱退対象

-2. 191行目 or 194行目

↓
sshでリモートログインするノードを適宜変更する.上記の例ではremote1 = neutrino,remote2 = zoroなので...
 if(num == 1) # remote1
 	sshcommand="/usr/bin/ssh neutrino.is.utsunomiya-u.ac.jp '/bin/sh -c '\\''"
 end
 if(num == 2) # remote2
	sshcommand="/usr/bin/ssh zoro.is.utsunomiya-u.ac.jp '/bin/sh -c '\\''"

$ ruby myruby.rb 3 1 j
  => (上記は3ノードのクラスタからzoroが脱退し、jokerとneutirnoでリスタートする場合の記述となっている)

*****************************************************************************************
