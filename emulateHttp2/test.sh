#!/bin/bash
if [ $# -lt 8 ]
then
  echo "Usage : ip port filename requestNum deviceName filterExpression resultDir sleepTime"
  exit 1
fi
curDir=$(pwd)
ip=$1
port=$2
fileName=$3
num=$4
device=$5
filterExpression=$6
resultDir=$7
sleepTime=$8
delays=("0ms"  "50ms" "100ms" "250ms" "500ms")
bdlimits=("100mbit" "50mbit" "10mbit" "5mbit")
bufferSize=(1048576 2097152 4194304 8388608 16777216)

defaultSize=524288 
for defaultSize in ${bufferSize[@]}:
do
  minSize=$((defaultSize / 2))
  maxSize=$((defaultSize * 2))
  echo "change tcp buffer to $minSize, $defaultSize, $maxSize"
  $curDir/interaction.exp "sudo ./tuneTcp.sh $minSize $defaultSize $maxSize;sudo sysctl -p;sudo sysctl net.ipv4.tcp_wmem"
  if [ $? -ne 0 ];then
	echo "set tcp buffer failed"
	exit 1
  fi
  date=$( date +"%Y%m%d-%H%M")
  mkdir -p $curDir/${resultDir}_${defaultSize}_${date}
  resultDir=$curDir/${resultDir}_${defaultSize}_$date
  echo "store result in $resultDir"
  cd $resultDir
  echo $(pwd)

  delay="0ms"
  bdlimit="100mbit"
  for delay in ${delays[@]}
  do
	  for bdlimit in ${bdlimits[@]}
	  do
		  echo "bandwidth limitation: $bdlimit, delay: $delay"
		  logPrefix=${bdlimit}_${delay}
		  $curDir/interaction.exp "sudo ./setBdLimit.sh $bdlimit $port $delay"
		  echo "start to execute: ./client $ip $port $fileName $num 0 $device '$filterExpression' $logPrefix $sleepTime"
		  $curDir/client $ip $port $fileName $num 0 $device "$filterExpression" $logPrefix $sleepTime
		  echo "start to execute: ./client $ip $port $fileName $num 1 $device '$filterExpression' $logPrefix $sleepTime"
		  $curDir/client $ip $port $fileName $num 1 $device "$filterExpression" $logPrefix $sleepTime
	  done
  done

done
