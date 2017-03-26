フォルダ/ファイル構成
※アクセスできないファイルがあったら、適宜ルート権限で変更してください
-takeover_sawada
	- readme.txt　			<--(this file)

	- my_dmtcp/
	  - readme_my_dmtcp.txt		<--私が変更を加えたDMTCPの特徴・注意点
	  - source/			<--変更を加えたDMTCP
	  - about_modified_source/	<--具体的に変更を加えたソースファイルの解説
	
	- about-mpiP/			<--MPIアプリケーションの静的解析について

	- about_ipc_bench/		<--TCP/Unix Domai Socket等々の計測プログラム
	  - ipc-bench-master/
	    -unix_thr-2017-01-05.c	<--若干の修正が入ったUDS計測プログラム
	    -他	    
					    
	- application/
	  - readme_application.txt	<--実行全体の流れ
	  - NPB3.3.1/
	  - n-queen/
	
	- related_work/			<--関連研究

        - syuron/			<--修士論文

	- briefing_paper_pptx/		<--諸々の発表資料

	- test/				<--テスト実行環境
	  - filedelete.rb		<--ckptファイル削除スクリプト
	  - myruby.rb			<--restartスクリプト作成を簡略化/効率化するためのお助けスクリプト
	  - myruby_readme.txt		<--myruby.rbを使用するときの注意点
	  - dl2.sh 			<--テスト/実行作業効率化用のお助けスクリプト
	  - inspect_proc.sh             <--DMTCPがdefaultで用意している便利スクリプト.PIDを引数にとり、プロセスが開いているFD,またその種類についてデバッグ表示してくれる

	- blockus/			<--blokusに関するソースなどは渡したが、このフォルダ内にあるファイルに置き換えて実行してみて(正常に動作すればこっちの方がいい)
