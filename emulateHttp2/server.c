/***********************************************
*
*Filename: server.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-11-26 11:04:10
# Last Modified: 2016-01-25 22:32:29
************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>

#include "server.h"
#include "util.h"

int main(int argc, char *argv[]) {
  //check args num
  if (argc < 2) {
	printf("Usage : server port\n");
	return 1;
  }

  //create socket and bind to port
  int listenSockFd, listenPort;
  int acceptedSockFd;
  struct sockaddr_in serverAddr_in;
  int len_sockaddr_in = sizeof(struct sockaddr_in);

  listenSockFd = socket(AF_INET,SOCK_STREAM,0);//create socket 
  listenPort = atoi(argv[1]);
  printf("the listen port is %d \n", listenPort);
  memset((char*) &serverAddr_in, 0, sizeof(serverAddr_in));
  serverAddr_in.sin_family         = AF_INET;
  serverAddr_in.sin_addr.s_addr    = INADDR_ANY;
  serverAddr_in.sin_port           = htons(listenPort);
  // set address reuse option
  int reuseOn = 1;
  if (setsockopt(listenSockFd, SOL_SOCKET, SO_REUSEADDR, &reuseOn, sizeof reuseOn) < 0) {
	printf("set reuse option failed\n");
	return 1;
  }

  if (bind(listenSockFd, (struct sockaddr *)&serverAddr_in, len_sockaddr_in) < 0) {
	  printf("bind to port failed \n");
	  return 1;
  }

  listen(listenSockFd,MAX_CONNECTION_QUEUE);
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof (struct sockaddr_in);
  while (1) {
	memset(&clientAddr, 0, sizeof clientAddr);
	acceptedSockFd = accept(listenSockFd, (struct sockaddr *)&clientAddr, &addrLen);
	if (acceptedSockFd >= 0) {
	  RequestInfo * info = (RequestInfo *) malloc(sizeof(RequestInfo));
	  info->sockFd = acceptedSockFd;
	  info->clientPort = ntohs(clientAddr.sin_port);
	  memset(info->clientIP, 0, sizeof info->clientIP);
	  inet_ntop(AF_INET, &(clientAddr.sin_addr), info->clientIP, 19);
	  pthread_t thread;
	  pthread_create(&thread, NULL, processRequest, (void *)info);
	  //struct timeval tv;
	  //tv.tv_sec = 30;  /* 30 Secs Timeout */
	  //setsockopt(acceptedSockFd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
	} else {
	  printf("failed to accept new request\n");
	  continue;
	}
  }
  return 0;
}

/**
*@description creat a logfile name
*@param resultStr char * store the created name
*@param maxSize int the max size of resultStr
*/
int createLogfileName(char * resultStr, int maxSize) {
  time_t now;
  struct tm * timeInfo;
  time(&now);
  timeInfo = localtime(&now);

  char timeFormat[] = "%Y_%m_%d_%H_%M.csv";
  char timeStr[1024] = {0};
  strftime(resultStr, maxSize, timeFormat, timeInfo);
  return 0;
}

/** used by thread to process new request*/
void * processRequest(void * param) {
  RequestInfo *requestInfo = (RequestInfo *) param;
  int  sockFd = requestInfo->sockFd;
  char *clientIP  = requestInfo->clientIP;
  int  clientPort = requestInfo->clientPort;

  printf("start to serve request from %s:%d\n", clientIP, clientPort);
  //receive request file information
  char requestFile[1024] = {0};
  int recvStatus = recvFromSock(sockFd, requestFile, sizeof requestFile - 1, 1);
  if (recvStatus < 0) {
	printf("recv request info failed\n");
	shutdown(sockFd, SHUT_RDWR);
	close(sockFd);
	free(requestInfo);
	return NULL;
  } else if (recvStatus == 0) {
	printf("client has closed this connection\n");
	shutdown(sockFd, SHUT_RDWR);
	close(sockFd);
	free(requestInfo);
	return NULL;
  }
  printf("requestInfo: %s\n", requestFile);

  // check File existence and open file
  if (!checkFileExist(requestFile)) {
	printf("file doesn't exist: %s\n", requestFile);
	shutdown(sockFd, SHUT_RDWR);
	close(sockFd);
	free(requestInfo);
	return NULL;
  }
  FILE * fd = fopen(requestFile, "r");
  if (!fd) {
	printf("open request file failed\n");
	goto cleanAndQuit;
  }

  // send requested file
  char buffer[1024];
  unsigned int requestFileSize = getFileSize(requestFile);
  unsigned int readSize = 0;
  unsigned int sentSize = 0;
  printf("start to send to client\n");
  while (readSize < requestFileSize) {
	unsigned int tempReadLen = 0;
	memset(buffer, 0, sizeof buffer);
	if ((tempReadLen = fread(buffer, sizeof(char), sizeof buffer -1, fd)) < 0) {
	  printf("read file failed %d\n", tempReadLen);
	  goto cleanAndQuit;
	}
	readSize += tempReadLen;

	//send to socket
    if (sendToSock(sockFd, buffer, tempReadLen, 1) != 1) {
	  goto cleanAndQuit;
	}
	sentSize += tempReadLen;
	//printf("send to client with len %d, sumLen, %d\n", sumSentLen, sentSize);

	if (canRead(sockFd) != 1) {
	  continue;
	}
	printf("start to receive cancel/close msg\n");
	memset(buffer, 0, sizeof buffer);
    int recvLen = recvFromSock(sockFd, buffer, sizeof buffer - 1, 1);
	if (recvLen <= 0) {// the other side has close the sockfd or some error happens
	  goto cleanAndQuit;
	}
	printf("sent %d KB data\n", sentSize / 1024);
	printf("receive cancel/close msg: %s\n", buffer);
	if (strcmp(buffer, CANCEL_MSG) == 0) {
	  printf("wait for close after receiving cancel msg\n");
	  if (waitForClose(sockFd) == 1) {
		printf("client close connection, quit right now\n");
	  } else {
		printf("waitForClose Failed, quit right now\n");
	  }
	} else {
	  printf("close after receiving close msg\n");
	}
	cleanAndQuit:
	shutdown(sockFd, SHUT_RDWR);
	close(sockFd);
	if (requestInfo) {
	  free(requestInfo);
	}
	if (fd) {
	  fclose(fd);
	}
	return NULL;
  }
}
