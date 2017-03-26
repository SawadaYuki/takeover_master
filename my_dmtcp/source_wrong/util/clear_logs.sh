#!/bin/sh
#this script was used to free up space after testing on teracluster
#it also wipes out debugging logs
#it takes no args and expects a file 'hosts' to contain a list of nodes

#index = 0
rm /tmp/dmtcp-sawada@joker.is.utsunomiya-u.ac.jp/*

for X in localhost `cat < hosts`
do
  echo "Clearing $X..."
  # if [$index = 0]; then
  #     rm -f /tmp/dmtcp-sawada@$X/*
  #     index = 1
  # fi 
  
  #ssh $X rm -f /tmp/{jassertlog,dmtcpConTable,mpd2}.\* {/dev/shm,~/san,/tmp}/ckpt\*mtcp /tmp/pts\*  
  ssh $X rm -f /tmp/dmtcp-sawada@$X/*
done
wait
