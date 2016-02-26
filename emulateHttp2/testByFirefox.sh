#!/bin/bash
#cancel download in browser
cancelDownload(){
  wndId=$(xdotool search --name "Mozilla Firefox")
  xdotool windowactivate --sync $wndId

  xdotool windowsize --sync $wndId 1000 500

  xdotool windowmove --sync $wndId 200 200
  xdotool mousemove --sync 1060 275
  xdotool click  1
  sleep 2
  xdotool mousemove_relative  --sync -- -10 70
  sleep 1
  xdotool click 1
}

startSendRequest() {
  requestUrl=$1
  echo $1
  wndId=$(xdotool search --name "Mozilla Firefox")
  if [ "$wndId" == "" ];then
	firefox >/dev/null 2>&1 &
	sleep 4
    wndId=$(xdotool search --name "Mozilla Firefox")
	echo $wndId
  fi

  xdotool windowactivate --sync $wndId
  #xdotool windowfocus --sync $wndId
  xdotool windowsize --sync $wndId 1000 500
  xdotool windowmove --sync $wndId 200 200
#why relative mouse move doesn't work?
  #xdotool mousemove --window $wndId --sync  200 75
  xdotool mousemove --sync  400 275
  xdotool click 1
  sleep 2
  xdotool type $requestUrl
  xdotool key Return
}

killBrowser() {
  wndId=$(xdotool search --name "Mozilla Firefox")
  if [ "$wndId" == "" ];then
	return 
  fi
  xdotool windowactivate --sync $wndId
  xdotool key ctrl+shift+w
  sleep 3
  #xdotool windowsize --sync $wndId 1000 500
  #xdotool windowmove --sync $wndId 200 200
  ##xdotool mousemove --window $wndId --sync  200 75
  #xdotool mousemove --sync --window $wndId  15 15
  #xdotool click 1
  #sleep 2
}
#test code
#i=0
#sum=10
#while [ $i -le $sum ]
#do 
#  startSendRequest "https://http2_test:6003/250mb.tar.gz"
#  sleep 10
#  cancelDownload
#  sleep 5
#  killBrowser
#  sleep 2
#  ((i++))
#done
#exit 0
requestAndSniff() {
	if [ $# -lt 2 ];then
	  echo "Usage requestUrl logFile\n"
	fi
	sudo ./packetSniff $device "$filterExpression" payloadFile $2 >logSniff &
	sniffId=$!
	echo "processId of sniff is $sniffId"
	startSendRequest $1
	sleep 15
	cancelDownload
	sudo kill -SIGCONT $sniffId
	sleep 10
	sudo kill -SIGINT $sniffId
	killBrowser
}
curDir=$(pwd)
serverName="nginx"
if [ $# -ge 1 ];then
  serverName=$1
fi

ip="156.56.83.24"
portH2=6003
portH1=6004
urlH1="https://http1_test:6004"
urlH2="https://http2_test:6003"
fileName="250mb.tar.gz"
num=10
device="eth0"
filterExpression="src host $ip"
resultDir=$curDir/testWithFirefoxBrowser_${serverName}_$(date +"%Y%m%d_%H%M%S")
mkdir -p $resultDir
#delays=("50ms" "100ms" "250ms" "500ms")
delays=("100ms" "500ms")
#bdlimits=("100mbit" "50mbit" "10mbit" "5mbit")
bdlimits=("100mbit" "10mbit")
#bufferSize=(1048576 2097152 4194304 8388608 16777216)
bufferSize=(8388608)

defaultSize=524288 
for defaultSize in ${bufferSize[@]}
do
  echo $defaultSize
  minSize=$((defaultSize / 2))
  maxSize=$((defaultSize * 2))
  echo "change tcp buffer to $minSize, $defaultSize, $maxSize"
  $curDir/interaction.exp "sudo ./tuneTcp.sh $minSize $defaultSize $maxSize;sudo sysctl -p;sudo sysctl net.ipv4.tcp_wmem"
  if [ $? -ne 0 ];then
	echo "set tcp buffer failed"
	exit 1
  fi
  date=$( date +"%Y%m%d-%H%M")

  delay="0ms"
  bdlimit="100mbit"
  for delay in ${delays[@]}
  do
	  for bdlimit in ${bdlimits[@]}
	  do
		  echo "bandwidth limitation: $bdlimit, delay: $delay"
		  $curDir/interaction.exp "sudo ./setBdLimit.sh $bdlimit $portH2 $delay"
		  i=0
		  while [ $i -lt $num ]
		  do
			requestAndSniff $urlH2/$fileName $resultDir/sniff_H2_${bdlimit}_${delay}_${defaultSize}
			((i++))
		  done

		  $curDir/interaction.exp "sudo ./setBdLimit.sh $bdlimit $portH1 $delay"
		  i=0
		  while [ $i -lt $num ]
		  do
			requestAndSniff $urlH1/$fileName $resultDir/sniff_H1_${bdlimit}_${delay}_${defaultSize}
			((i++))
		  done
	  done
  done

done

