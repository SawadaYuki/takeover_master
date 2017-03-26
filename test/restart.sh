#!/bin/sh
dmtcp_restart --host joker.is.utsunomiya-u.ac.jp --port 7779 --interval 0  /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_cg.C.4_22fa2f4d8b89e2f-48000-58d7626b.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_cg.C.4_22fa2f4d8b89e2f-49000-58d7626b.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_orterun_22fa2f4d8b89e2f-40000-58d7626a.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_cg.C.4_3442c2f7b6cfb13a-46000-58d7626b.dmtcp /home/fss5/sawada/デスクトップ/takeover_sawada/test/ckpt_cg.C.4_3442c2f7b6cfb13a-47000-58d7626b.dmtcp &

#最後に起動したバックグラウンドプロセスのプロセスIDを格納
pid = $!
#対象のプロセスの終了するのを待つ
wait $pid
echo "対象pid =  $pid"
#終了ステータスが格納されている
echo "終了ステータス =  $?"
