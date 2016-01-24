#!/bin/bash
if [ $# -lt 7 ]
then
  echo "Usage : ip port filename requestNum deviceName filterExpression resultDir "
  exit 1
fi
date=$( date +"%Y%m%d-%H%M")
curDir=$(pwd)
resultDir=$7
ip=$1
port=$2
fileName=$3
num=$4
device=$5
filterExpression=$6

mkdir -p $curDir/${resultDir}_$date
resultDir=$curDir/${resultDir}_$date
echo "store result in $resultDir"
cd $resultDir
echo $(pwd)

delays=("0ms"  "50ms" "100ms" "200ms" "500ms")
bdlimits=("100mbit" "50mbit" "10mbit" "5mbit" "2mbit")
delay="0ms"
bdlimit="100mbit"
for delay in ${delays[@]}
do
    for bdlimit in ${bdlimits[@]}
    do
        echo "bandwidth limitation: $bdlimit, delay: $delay"
        logPrefix=${bdlimit}_${delay}
        $curDir/interaction.exp "sudo ./setBdLimit.sh $bdlimit $port $delay"
        echo "start to execute: ./client $ip $port $fileName $num 0 $device '$filterExpression' $logPrefix"
        $curDir/client $ip $port $fileName $num 0 $device "$filterExpression" $logPrefix
        echo "start to execute: ./client $ip $port $fileName $num 1 $device '$filterExpression' $logPrefix"
        $curDir/client $ip $port $fileName $num 1 $device "$filterExpression" $logPrefix
    done
done

