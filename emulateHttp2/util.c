/***********************************************
*
*Filename: util.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2016-01-25 13:31:40
# Last Modified: 2016-01-25 15:26:23
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
#include "util.h"

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

/**use select to decide whether the sock is ready for reading or not*/
int canRead(int sockFd) {
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sockFd, &set);
  int maxFd = sockFd + 1;
  select(maxFd, &set, NULL, NULL, &timeout);
  if (FD_ISSET(sockFd, &set)) {
	return 1;
  } else {
	return 0;
  }
}
/**use select to decide whether the sock is ready for writing or not*/
int canWrite(int sockFd) {
  struct timeval timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 0;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sockFd, &set);
  int maxFd = sockFd + 1;
  select(maxFd, NULL, &set, NULL, &timeout);
  if (FD_ISSET(sockFd, &set)) {
	return 1;
  } else {
	return 0;
  }
}
/** wait for the sock to be ready for reading until timeout, if timeout is NULL, will wait forever*/
int waitForRead(int sockFd, struct timeval *timeout) {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sockFd, &set);
  int maxFd = sockFd + 1;
  select(maxFd, &set, NULL, NULL, timeout);
  if (FD_ISSET(sockFd, &set)) {
	return 1;
  } else {
	return 0;
  }
}
/** wait for the sock to be ready for writing until timeout, if timeout is NULL, will wait forever*/
int waitForWrite(int sockFd, struct timeval *timeout) {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sockFd, &set);
  int maxFd = sockFd + 1;
  select(maxFd, NULL, &set, NULL, timeout);
  if (FD_ISSET(sockFd, &set)) {
	return 1;
  } else {
	return 0;
  }
}


/** wait for connection close by the other side*/
int waitForClose(int sockFd) {
  char buffer[1024] = {0};
  int sleepNum = 0;
  int sleepTime = 5;
  int status = -2;
  while (1) {
	status = recv(sockFd, buffer, sizeof buffer - 1, 0);
	if (status ==  0) {
	  return 1;
	} else {
	  sleep(sleepTime);
	  sleepNum += 1;
	  if (sleepNum >= 100) {//wait for no more than 500 seconds, which is completely enough
		return 0;
	  }
	}
	memset(buffer, 0, sizeof buffer);
  }
  return 0;
}

/** receive from sock */
int recvFromSock(int sockFd, char * buffer, int maxSize, int isBlock) {
  if (isBlock) {
	if (waitForRead(sockFd, NULL) != 1) {
	  return -1;
	}
  } else {
	if (canRead(sockFd) != 1) {
	  return -1;
	}
  }
  int recvLen = recv(sockFd, buffer, maxSize, 0);
  return recvLen;
}

/** send to  sock */
int sendToSock(int sockFd, char * buffer, int length, int isBlock) {
  if (isBlock) {
	if (waitForWrite(sockFd, NULL) != 1) {
	  return -1;
	}
  } else {
	if (canWrite(sockFd) != 1) {
	  return -1;
	}
  }
  int temp = 0;
  int sentSum = 0;
  while ( sentSum < length ) {
	temp = send(sockFd, buffer + sentSum, length - sentSum, 0);
	if (temp > 0) {
	  sentSum += temp;
	}
	continue;
  }
  if (sentSum == length) {
	return 1;
  }
  return -1;
}
