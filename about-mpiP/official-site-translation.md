#http://mpip.sourceforge.net/
##Introduction

mpiPは、MPIアプリケーション用の軽量プロファイリングライブラリです。 MPI関数に関する統計情報のみを収集するため、mpiPはトレースツールよりもかなり少ないオーバーヘッドとデータを生成します。

mpiPによってキャプチャされたすべての情報はタスクローカルです。
すべてのタスクの結果を1つの出力ファイルにマージするために、通常は**実験(experiment)**終了時のレポート生成時にのみ通信を使用します。

mpiPによるパフォーマンス解析の詳細については、**Vetter, J.S. and M.O. McCracken, "Statistical Scalability Analysis of Communication Operations in Distributed Applications," Proc. ACM SIGPLAN Symp. on Principles and Practice of Parallel Programming (PPOPP), 2001. **の論文を読め


##Using mpiP

mpiPの使用は非常に簡単です。 **MPIプロファイリングレイヤーを介してMPI情報を収集し、mpiPはリンク時ライブラリだからです。**

mpiPライブラリがリンク行のMPIライブラリの前に表示されていることを確認してください。
ライブラリ（-lbfd -liberty）は、シンボル情報のデコードをサポートしています。 それらはGNU binutilsの一部です。
>-L${mpiP_root}/lib -lmpiP -lm -lbfd -liberty -lunwind

##Configuring and Building mpiP
###Configuring mpiP
- --enable-collective-report-default
>デフォルトでは、mpiPはすべてのプロセスデータを1つのプロセスで集約**(aggregate)**し、レポートを生成します。
この機能を有効にすると、mpiPレポートの生成時に、報告されている個々のコールサイトについてのみコールサイトデータを集約するようになります。
これにより、多数のMPI呼び出しを行う大規模なアプリケーションのメモリ要件が大幅に削減されます。
レポート生成の動作を変更するには、ランタイムフラグ-lと-rを参照してください。

- --disable-libunwind
>スタックトレースを生成するためにlibunwindを使用しないでください。
>>現在、libunwindはIA64-Linuxおよびx86-Linuxプラットフォーム上では便利ですが、**Intelコンパイラで提供されるlibunwind.aと競合する可能性があります。**

- --with-wtime
>Use MPI_Wtime for timing
>>デフォルトのプラットフォームタイマーではなく、タイミングのためにMPI_Wtime呼び出しを使用します。

##Run-time Configuration of mpiP
mpiPには、ユーザーが環境変数MPIPを使用して設定できるいくつかの設定可能なパラメーターがあります。
MPIPの設定は、コマンドラインパラメータのように "-t 10 -k 2"となります。
さらに、 "-t10、-k2"のようにカンマを使用して複数のパラメータを区切ることができます。

>-g :Enable mpiP debug mode. 
>-k n :Sets callsite stack traceback depth to <n>.
>-p :Point-to-point histogram reporting on message size and communicator used.
>-t x :Set print **threshold(しきい値)** for report, where <x> is the MPI percentage of time for each callsite.
>-y :Collective histogram reporting on message size and communicator used.

たとえば、**callsite stackの歩行深度(walking depth)を2**に設定し、**レポートの印刷しきい値を10％**に設定するには、次の例のように、環境内でmpiP文字列を定義するだけです。
>$ setenv MPIP "-t 10.0 -k 2" (csh)


##mpiP Output
**Note that MPIP does not capture information about ALL MPI calls. **
MPI_Comm_sizeのようなローカル呼び出しは、**混乱(perturbation)**とmpiP出力を減らすためにプロファイリングライブラリ測定から除外されます。

| column | Description |
|--------|--------|
|   App%     |Ratio of time for this call to the overall application time for each task.|
|  MPI%      |Ratio of time for this call to the overall MPI time for each task.|

>--
>@--MPI Time (seconds) --
>--
>Task    AppTime    MPITime    MPI%
   0         10   0.000243    0.00
   1         10         10   99.92
   2         10         10   99.92
>  3         10         10   99.92
>  \*         40         30   74.94
>--

**Apptime**は、MPI_Initの終了からMPI_Finalizeの開始までのウォールクロック時間です。
**MPI_Time**はApptime内に含まれるすべてのMPI呼び出しのウォールクロック時間です。
**MPI％**はこのMPI_TimeとApptimeの比率を示します。 アスタリスク（*）は、アプリケーション全体の集約行です。

>--
@-- Callsites: 2 --
--
 ID Lev File/Address        Line Parent_Funct             MPI_Call
  1   0 9-test-mpip-time.c    52 main                     Barrier
  2   0 9-test-mpip-time.c    61 main                     Barrier
>--

callsiteセクションは、**アプリケーション内のすべてのMPIコールサイトを識別**します。
次の列には、MPI呼び出しのタイプ（MPI_接頭部なし）が表示されます。
このMPI呼び出しを含む関数の名前は、次にファイル名と行番号のあとに続きます。
最後の列には、そのMPIコールサイトのPCまたはプログラムカウンタが表示されます。
**callsite stack walk depthのデフォルト設定は1です。**
他の設定では、単一のコールサイトだけでなく、スタックトレース全体でコールタイトを**列挙する(enumerate)**。

>--
@-- Aggregate Time (top twenty, descending, milliseconds) --
--
>Call                 Site       Time    App%    MPI%     COV
>Barrier                 2      3e+04   75.00  100.00    0.67
>Barrier                 1      0.405    0.00    0.00    0.59
>--

**集計時間セクションは、アプリケーションで最も集約された時間を消費する上位20個のMPIコール対象の概要です。**
Callは、MPI関数のタイプを識別します。
Siteは、コールサイトIDを提供します（**callsite sectionにリスト**されています）。
Timeは、そのコールサイトの集約時間（ミリ秒単位）です。
次の2つの列は、総適用時間と合計MPI時間の合計時間の割合を示しています。

**COV列**は、個々のプロセス時間から計算された変動係数を提示することによって、このコールサイトの個々のプロセスの時間の**変化( variation)**を示します。**値が大きいほど処理時間のばらつきが大きくなります。**

>--
@-- Aggregate Sent Message Size (top twenty, descending, bytes) --
--
>Call                 Site      Count      Total    Avrg      MPI%
>Send                    7        320   1.92e+06   6e+03     99.96
>Bcast                   1         12        336      28      0.02
>--

上記のセクションは集計時間セクションに似ていますが、**合計送信メッセージサイズの上位20個のコールサイト**について報告します。

>--
@-- Callsite Time statistics (all, milliseconds): 8 --
--
>Name              Site Rank  Count      Max     Mean      Min   App%   MPI%
>Barrier              1    0      1    0.107    0.107    0.107   0.00  44.03
>Barrier              1    *      4    0.174    0.137    0.107   0.00   0.00

>Barrier              2    0      1    0.136    0.136    0.136   0.00  55.97
>Barrier              2    1      1    1e+04    1e+04    1e+04  99.92 100.00
>Barrier              2    2      1    1e+04    1e+04    1e+04  99.92 100.00
>Barrier              2    3      1    1e+04    1e+04    1e+04  99.92 100.00
>Barrier              2    *      4    1e+04  7.5e+03    0.136  74.94 100.00
>--

最後のセクションは、すべてのタスクの各コールサイトの統計情報リストと、それに続く集計行です（ランク列のアスタリスクで示されます）。
最初のセクションはoperation timeの後にメッセージサイズのセクションが続きます。

**MPIP%が10％未満の行をprintしないようにMPIPを構成/設定しているに注意してください。 
すべての集約行は構成設定に関係なく印刷されます。**

各コールの集約結果は、同じ測定の意味を持ちます
;ただし、統計はすべてのタスクにわたって収集され、集計アプリケーションおよびMPI時間と比較されます。
**送信されるメッセージサイズのセクションも同様の形式です。**

>--
@-- Callsite Message Sent statistics (all, sent bytes) --
--
Name              Site Rank   Count       Max      Mean       Min       Sum
Send                 5    0      80      6000      6000      6000   4.8e+05
Send                 5    1      80      6000      6000      6000   4.8e+05
Send                 5    2      80      6000      6000      6000   4.8e+05
Send                 5    3      80      6000      6000      6000   4.8e+05
Send                 5    *     320      6000      6000      6000   1.92e+06
>--
>>Count :Number of times this call was executed.
>>Maximum :sent message size in bytes for one call.
>>Mean  :Arithmetic mean of the sent message sizes in bytes for one call.
>>>1回の呼び出しで送信されたメッセージサイズのバイト単位の算術平均。
>>Sum 	:Total of all message sizes for this operation and callsite.

**MPI I / Oレポートセクションの形式は、送信されたメッセージサイズセクションに非常に似ています。**

>--
@-- Callsite I/O statistics (all, I/O bytes) --
--
Name              Site Rank   Count       Max      Mean       Min       Sum
File_read            1    0      20        64        64        64      1280
File_read            1    1      20        64        64        64      1280
File_read            1    *      40        64        64        64      2560
>--

## Controlling the Scope of mpiP Profiling in your Application
mpiPでは、**MPI_Pcontrol（int level）サブルーチンを使用してプロファイリング測定の範囲(regions)をコードの特定の領域に制限することができます。**0の値はmpiPプロファイリングを無効にし、0以外の値はプロファイリングを有効にします。

最初にMPI_Initでプロファイリングを無効にするには、-o設定オプションを使用します。 mpiPは、起動と停止の間に遭遇したMPIコマンドに関する情報だけを記録します。

実行中にアプリケーションがプロファイリングをアクティブにできる回数に制限はありません。
たとえば、アプリケーションでは、Pcontrolを使用してtimestep 5のみMPIアクティビティを取得できます。
```
for(i=1; i < 10; i++)
{
  switch(i)
  {
    case 5:
      MPI_Pcontrol(1);
      break;
    case 6:
      MPI_Pcontrol(0);
      break;
    default:
      break;
  }
  /* ... compute and communicate for one timestep ... */
}
```
この機能を使用するときは、mpiP環境変数に-oを含めるように設定してください。

##Arbitrary(任意) Report Generation
引数3または4（下の表を参照）でMPI_Pcontrol（）を呼び出すことによって、任意のレポートを生成することもできます。
>0 	Disable profiling.
1 	Enable Profiling.
2 	Reset all callsite data.
3 	Generate verbose report.
>4 	Generate concise report.

最終レポートは、MPI_Finalize中に生成されます。 注：現在のリリースでは、コールサイトIDはレポート間で一貫しません。
レポート間のコールサイトデータの比較は、発信元ロケーションとコールスタックによって行う必要があります。

コードの一部が実行されるたびに個別のレポートを生成したいが、プロファイルデータを蓄積したくない場合は、コードを指定してプロファイルデータを再設定し、プロファイルを作成してからレポートを生成することができます。 例えば：
```
for(i=1; i < 10; i++)
{
  switch(i)
  {
    case 5:
      MPI_Pcontrol(2); // make sure profile data is reset
      MPI_Pcontrol(1); // enable profiling
      break;
    case 6:
      MPI_Pcontrol(3); // generate verbose report
      MPI_Pcontrol(4); // generate concise report
      MPI_Pcontrol(0); // disable profiling
      break;
    default:
      break;
  }
  /* ... compute and communicate for one timestep ... */
}
```

##Caveats(警告)
- mpiPにソースコード変換に問題がある場合は、以下の手法のいくつかを使用して、LLNLシステム上のプログラムカウンタをデコードすることができます。instmap、addr2lineを使うか、アセンブラコード自体を見ることができます。
- ループアンローリングのようなコンパイラ変換は、1つのソースコード行を多くの異なるPCとして表示することがあります。これは、アセンブラを見ることで確認できます。私の経験では、instmapとaddr2lineの両方が、これらの変換されたPCをファイル名と行番号にマッピングすることができます。
	- instmap—an IBM utility
	- addr2line—a gnu tool
	- look at the assembler listing, or with GNU's objdump (-d -S)
	- use Totalview or gdb to translate the PC

- 特定のbinutilsバージョンおよび最近のバージョンのIBMコンパイラーとの非互換性が知られています。 今回のリリースでは、修正はbinutilsに組み込まれていませんでしたが、-bnoobjreorderオプションを使用することは有効な回避策です。
- あるケースでは、64ビットのFortranアプリケーションのソース・ルックアップを持つIBMマシンで問題が発生しました。 不正なコンパイラ設定ファイルが使用され、デバッグ情報とPC値が正しく一致していないようです。 これを解決するには、リンクフラグ-bpT：0x100000000を使用します。
- スタックウォーク最適化アプリケーションの問題：
	- gccでコンパイルされたアプリケーションが誤った親関数を返すことがあります。 ただし、ファイルと行番号の情報が正しいかもしれません。
	- インテル®コンパイラーでコンパイルされたアプリケーションは、親スタック・フレームを識別できない場合があります。
	
- 動的にロードされるオブジェクト内からMPI関数を呼び出す場合は、ライブラリを共有オブジェクトとして再コンパイルする必要があります。
- LinuxおよびAIXシステムで時折否定的な報告値が発生しました。 私たちは引き続きこの問題を調査しますが、この動作がmpiPで発生する可能性があります。
