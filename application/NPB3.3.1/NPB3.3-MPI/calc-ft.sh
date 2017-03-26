#!/bin/sh

command=$*
echo "command ="  $command

result_file=TMP.txt
touch "$result_file"

for i in  1 2 3 4 5 6 7 8 9 10
do 
    echo "$i : exec commad"
    `$command >> $result_file`
done


`mv "$result_file" ./FT/ `


