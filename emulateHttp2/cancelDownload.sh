#!/bin/bash
#cancel download in browser
cancelDownload(){
  wndId=$(xdotool search --name "Google Chrome")
  xdotool windowactivate --sync $wndId

  xdotool windowsize --sync $wndId 500 500

  xdotool windowmove --sync $wndId 250 250
  xdotool mousemove --window $wndId --sync 200 480
  xdotool click  1
  xdotool mousemove_relative  --sync 50 -40
  sleep 1
  xdotool click 1
}

startSendRequest() {
  requestUrl=$1
  wndId=$(xdotool search --name "Google Chrome")
  if [ "$wndId" == "" ];then
	google-chrome &
	sleep 2
    wndId=$(xdotool search --name "Google Chrome")
  fi

  xdotool windowactivate --sync $wndId
  xdotool windowfocus --sync $wndId
  #xdotool click 1
  xdotool type $requestUrl
  xdotool key Return
}

killBrowser() {
  wndId=$(xdotool search --name "Google Chrome")
  if [ "$wndId" == "" ];then
	return 
  fi
  xdotool windowkill $wndId
}

if [ $# -lt 1 ];then
  echo "Usage requestUrl"
  exit 1
fi
startSendRequest $1
sleep 15
killBrowser
#cancelDownload
