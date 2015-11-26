/***********************************************
*
*Filename: server.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-11-26 11:04:10
# Last Modified: 2015-11-26 15:01:39
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
#include <time.h>

#define MAX_CONNECTION_QUEUE 10

//create a logfile name 
int createLogfileName(char * resultStr, int maxSize);

int checkFileExist(char * filePath);

// get file size
int getFileSize(char * filePath);

int main(int argc, char *argv[]) {
  //check args num
  if (argc < 2) {
	printf("Usage : server port\n");
	return 1;
  }

  // initial logfile
  char logfileName[1024] = {0};
  createLogfileName(logfileName, 1023);
  FILE * logfd = fopen(logfileName, "w");
  if (!logfd) {
	printf("open logfile failed: %s\n", logfileName);
	return 1;
  }
  char logStr[1024] = {0};
  sprintf(logStr, "bytesSentWhenStop, bytesInTCPSendBuffer\n");
  if (fwrite(logStr, sizeof(char), strlen(logStr), logfd) != strlen(logStr)) {
	printf("write log failed\n");
	return 1;
  }
  fclose(logfd);

  //create socket and bind to port
  int listenSockFd, listenPort;
  int acceptedSockFd;

  //sockaddr_in struct
  //sa_family_t    sin_family
  //in_port_t      sin_port
  //struct in_addr sin_addr
  //unsigned char  sin_zero[8]
  struct sockaddr_in serverAddr_in;
  int len_sockaddr_in = sizeof(struct sockaddr_in);
  if (argc < 2) {
	printf("usage: ./server port\n");
	return 1;
  }
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
  while (1) {
	char clientIp[20];
	int clientPort;
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof (struct sockaddr_in);
	acceptedSockFd = accept(listenSockFd, (struct sockaddr *)&clientAddr, &addrLen);
	if (acceptedSockFd >= 0) {
	  clientPort = ntohs(clientAddr.sin_port);
	  memset(clientIp, 0, sizeof clientIp);
	  inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, 19);
	  printf("start to serve request from client %s:%d\n", clientIp, clientPort);
	} else {
	  printf("failed to accept new request\n");
	  close(listenSockFd);
	  return 1;
	}
	fd_set originalFds;
	fd_set readFds;
	fd_set writeFds;
	int maxFd = acceptedSockFd + 1;
	FD_ZERO(&originalFds);
	FD_SET(acceptedSockFd, &originalFds);
	struct timeval zeroTime;
	zeroTime.tv_sec = 0;
	zeroTime.tv_usec = 0;
	readFds = originalFds;
	select(maxFd, &readFds, NULL, NULL, NULL);
	if (!FD_ISSET(acceptedSockFd, &readFds)) {
	  printf("accept request timeout\n");
	  close(acceptedSockFd);
	  continue;
	}
	char requestFile[1024] = {0};
	if (recv(acceptedSockFd, requestFile, sizeof requestFile - 1, 0) <= 0) {
	  printf("recv request info failed\n");
	  return 1;
	}
	printf("requestInfo: %s\n", requestFile);

	// check File existence and open file
	if (!checkFileExist(requestFile)) {
	  printf("file doesn't exist: %s\n", requestFile);
	  close(acceptedSockFd);
	  continue;
	}
	FILE * fd = fopen(requestFile, "r");
	if (!fd) {
	  printf("open request file failed\n");
	  close(acceptedSockFd);
	  continue;
	}

	// send requested file
	char buffer[5012];
	unsigned int requestFileSize = getFileSize(requestFile);
	unsigned int readSize = 0;
	unsigned sentSize = 0;
	while (1) {
	  unsigned int tempReadLen = 0;
	  memset(buffer, 0, sizeof buffer);
	  if ((tempReadLen = fread(buffer, sizeof(char), sizeof buffer -1, fd)) <= 0) {
		printf("read file failed\n");
		close(acceptedSockFd);
		break;
	  }
	  readSize += tempReadLen;

	  //send to socket
	  writeFds = originalFds;
	  select(maxFd, NULL, &writeFds, NULL, NULL);
	  if (FD_ISSET(acceptedSockFd, &writeFds)) {
		int tempSentLen = 0;
		int sumSentLen  = 0;
		while (sumSentLen < tempReadLen) {
		  tempSentLen = send(acceptedSockFd, buffer + sumSentLen, tempReadLen - sumSentLen, 0);
		  if (tempSentLen < 0) {
			printf("send error\n");
			close(acceptedSockFd);
			return 1;
		  }
		  sumSentLen += tempSentLen;
		}
		sentSize += sumSentLen;
		printf("send %d bytes\n", sumSentLen);
	  }

	  // check stop message without blocking
	  readFds = originalFds;
	  select(maxFd, &readFds, NULL, NULL, &zeroTime);
	  if (FD_ISSET(acceptedSockFd, &readFds)) {
		memset(buffer, 0, sizeof buffer);
		if (recv(acceptedSockFd, buffer, sizeof buffer -1, 0) > 0) {
		  printf("recved stop msg: %s\n", buffer);
		  FILE * logfd = fopen(logfileName, "a");
		  if (!logfd) {
			printf("open log file failed\n");
		  } else {
			unsigned int bytesInTcpbuffer = 0;
			memset(logStr, 0, sizeof logStr);
			sprintf(logStr, "%d,%d\n", sentSize, bytesInTcpbuffer);
			if (fwrite(logStr, sizeof(char), strlen(logStr), logfd) <= 0) {
			  printf("append log file failed\n");
			}
		  }
		  printf("close socket and start to wait for another connection\n");
		  close(acceptedSockFd);
		  if (fd) {
			fclose(fd);
		  }
		  if (logfd) {
			fclose(logfd);
		  }
		  break;
		}
	  }
	}
	if (fd) {
	  fclose(fd);
	}
	close(acceptedSockFd);
  }
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

  char timeFormat[] = "%Y_%m_%d_%H_%M.cvs";
  char timeStr[1024] = {0};
  strftime(resultStr, maxSize, timeFormat, timeInfo);
  return 0;
}


int checkFileExist(char * filePath) {
	struct stat statStruct;
	return ( stat(filePath,&statStruct) ) == 0;
}

int getFileSize(char * filePath) {
	struct stat statStruct;
	if (stat(filePath,&statStruct) == 0) { 
		return statStruct.st_size;
	} else {
		return -1;
	}
}