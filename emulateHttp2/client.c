/***********************************************
*
*Filename: client.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-11-24 19:08:50
# Last Modified: 2015-11-26 10:51:42
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

#define RANDOM_RANGE 30
#define STOP_MSG "stop"

int main(int argc, char * argv[]) {
  int clientSockFd;
  struct sockaddr_in serverAddr_in;
  int len_sockaddr_in = 0;
  int serverIp;
  int serverPort;
  char serverIpStr[20];
  char logStr[1024];
  if (argc < 6) {
	  printf("usage: client ip port requestfilename logfilename  requestNum\n");
	  return 1;
  }

  //initialize logfile
  char * logFile = argv[4];
  FILE * fd = fopen(logFile, "w");
  if (!fd) {
	printf("open file failed: %s\n", argv[4]);
	return 1;
  }
  memset(logStr, 0, sizeof logStr);
  sprintf(logStr, "timeDuration, bytesRecvedWhenStop\n");
  if (fwrite(logStr, 1, strlen(logStr), fd) <= 0) {
	printf("write to log file failed\n");
	return 1;
  }
  fclose(fd);

  len_sockaddr_in = sizeof( struct sockaddr_in);
  serverIp = inet_addr(argv[1]);
  serverPort = atoi(argv[2]);
  serverAddr_in.sin_family      = AF_INET;
  serverAddr_in.sin_port        = htons(serverPort);
  serverAddr_in.sin_addr.s_addr = serverIp;
  memset(serverIpStr, 0, sizeof(serverIpStr));
  inet_ntop(AF_INET, &(serverAddr_in.sin_addr), serverIpStr, 19);
  int requestNum = atoi(argv[5]);
  int i = 0;
  while (i < requestNum) {
	i++;
	clientSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(clientSockFd,(struct sockaddr*) &serverAddr_in,(socklen_t)len_sockaddr_in) != 0){
	  printf("failed to connect to server %s:%d \n", serverIpStr, serverPort);
	  shutdown(clientSockFd, SHUT_RDWR);
	  close(clientSockFd);
	  return 1;
	} else {
	  printf("connect to server %s:%d successfully\n", serverIpStr, serverPort);
	}
	int maxFd = clientSockFd + 1;
	fd_set originalFds;
	fd_set readFds;
	fd_set writeFds;
	FD_ZERO(&originalFds);
	FD_SET(clientSockFd, &originalFds);
	writeFds = originalFds;
	readFds  = originalFds;
	select(maxFd, NULL, &writeFds, NULL, NULL);
	if (FD_ISSET(clientSockFd, &writeFds) ) {
	  //send request file name
	  if (send(clientSockFd, argv[3], sizeof (argv[3]), 0) > 0) {
		// generate a random number
		srand(time(NULL));
		int random = rand() % RANDOM_RANGE + 1;
		struct timeval startTime;
		struct timeval curTime;
		gettimeofday(&startTime, NULL);
		unsigned int recvedBytes = 0;
		while (1) {
		  gettimeofday(&curTime, NULL);
		  unsigned int timeCost = (curTime.tv_sec - startTime.tv_sec) * 1000000 + (curTime.tv_usec - startTime.tv_usec);
		  if (timeCost >= random * 1000000) {
			FILE * fp = fopen(argv[4], "a+");
			if (!fp) {
			  printf("open log file failed\n");
			  return 1;
			}
			memset(logStr, 0, sizeof logStr);
			sprintf(logStr, "%d,%u\n", random, recvedBytes);
			if (fwrite(logStr, 1, strlen(logStr), fp) <= 0) {
			  printf("write to log file failed \n");
			  return 1;
			}
			fclose(fp);
			writeFds = originalFds;
			readFds  = originalFds;
			select(maxFd, NULL, &writeFds, NULL, NULL);
			if (FD_ISSET(clientSockFd, &writeFds)) {// send stop msg to server, record log, shutdown and close socket

			  if (send(clientSockFd, STOP_MSG, strlen(STOP_MSG), 0) <= 0) {
				printf("send stop msg failed in round %d, random %d\n", i, random);
				return 1;
			  }

			  // append log
			  FILE * fp = fopen(logFile, "a");
			  if (!fp) {
				printf("open log failed in round %d, random %d\n", i, random);
				return 1;
			  }
			  sprintf(logStr, "%d, %d\n", random, recvedBytes);
			  if (fwrite(logStr, sizeof(char), strlen(logStr), fp) < strlen(logStr)) {
				printf("append log failed in round %d, random %d\n", i, random);
				return 1;
			  }
			}

			//close socket
			close(clientSockFd);
			break;
		  }

		  // read data and append recvedBytes
		  readFds = originalFds;
		  select(maxFd, &readFds, NULL, NULL, NULL);
		  if (FD_ISSET(clientSockFd, &readFds)) {
			char recvBuffer[65536];
			int tempNum = 0;
			tempNum = recv(clientSockFd, recvBuffer, sizeof recvBuffer, 0);
			if (tempNum <= 0) {
			  printf("recv bytes failed in round %d random %d\n", i, random);
			  return 1;
			}
			recvedBytes += tempNum;
		  }
		}
	  } else {
		printf("send request failed in round %d\n", i);
		return 1;
	  }
	}
  }
}
