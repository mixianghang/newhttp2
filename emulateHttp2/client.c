/***********************************************
*
*Filename: client.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-11-24 19:08:50
# Last Modified: 2016-01-24 10:48:01
************************************************/
#include "client.h"

int main(int argc, char * argv[]) {
  //check comand line paremeters
  if (argc < 9) {
	  printf("usage: client ip port requestfilename requestNum cancelOrStop(1 is cancel, 0 is stop) netinterface filterExpression serverbandstatus\n");
	  return 1;
  }
  // check cancel parameter
  int isCancel = 0;
  char *prefix = argv[8];
  if (strcmp(argv[5], "1") == 0) {
	isCancel = 1;
  }
  //init logFile
  char logFile[FILE_NAME_SIZE] = {0};
  if (initLogFile(logFile, sizeof logFile -1, isCancel, prefix) != 0) {
	fprintf(stderr, "init logfile failed\n");
	return 1;
  }
  printf("isCancel:%d\n", isCancel);
  printf("log file name is %s\n", logFile);
  struct ClientSockInfo sockInfo;
  if (initClientSockInfo(&sockInfo, argv[1], argv[2]) == 0) {
	printf("init client sockinfo successfully\n");
  } else {
	fprintf(stderr, "init client sockinfo failed\n");
	return 1;
  }
  char * requestFileName = argv[3];
  int requestNum = atoi(argv[4]);
  //init sniffPanel
  struct SniffPanel sniffPanel;
  sniffPanel.device = argv[6];
  sniffPanel.filterExpression = argv[7];
  sniffPanel.liveReadTimeOut = 2000;//default 2000 ms
  sniffPanel.payloadFilePath = NULL;
  sniffPanel.payloadFile = NULL;
  sniffPanel.cbAfterSniff = &cbAfterSniff;
  sniffPanel.cbWhenError  = &cbForSniffError;
  sniffPanel.cbLog        = &cbForSniffLog;
  sniffPanel.afterSniffArgs = (void *)logFile;
  sniffPanel.errorArgs      = NULL;
  sniffPanel.isStopped      = 1;
  if (initSniff(&sniffPanel) != 0) {
	fprintf(stderr, "init packet sniff error\n");
	return 1;
  } else {
	printf("finish initializing packet sniff\n");
  }
  pthread_t sniffThread;
  int i = 0;
  while (i < requestNum) {
	if (!sniffPanel.isStopped) {
	  printf("we need to stop the last sniff before starting new round\n");
	  if (stopSniff(&sniffPanel) != 0) {
		fprintf(stderr, "stop sniff failed\n");
		continue;
	  } else {
		printf("finish stopping the last sniff before starting new round\n");
	  }
	}
	i++;
    printf("round %d\n", i);
	//create connection to server
	//if (i == 1 || !isCancel) {
	//  if (createNewConn(&sockInfo) != 0) {
	//	fprintf(stderr, "create new connection failed \n");
	//	return 1;
	//  } else {
	//	printf("connect to server %s:%d successfully from port %d\n", sockInfo.serverIpStr, sockInfo.serverPort, sockInfo.clientPort);
	//  }
	//}
	if (createNewConn(&sockInfo) != 0) {
      i--;
	  fprintf(stderr, "create new connection failed \n");
      continue;
	} else {
	  printf("connect to server %s:%d successfully from port %d\n", sockInfo.serverIpStr, sockInfo.serverPort, sockInfo.clientPort);
	}

	//configure packet sniff and start thread
	sniffPanel.packetNum      = 0;
	sniffPanel.payloadSize    = 0;
	sniffPanel.isStopped      = 0;
	pthread_create(&sniffThread, NULL, sniffThreadFunc, (void *)&sniffPanel);

	printf("finish starting thread for sniff\n");
	//send request
	if (sendRequest(&sockInfo, requestFileName, strlen(requestFileName)) == 0) {
	  printf("send request successfully\n");
	} else {
	  fprintf(stderr, "send request failed\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	}

	//receive response within random time
	int randomSecs = generateRandom(RANDOM_RANGE);
	randomSecs += 10;
	sockInfo.recvedBytes = 0;
	printf("start to recv within %d seconds\n", randomSecs);
	if (recvWithInRandom(&sockInfo, randomSecs) == 0) {
	  printf("recv %d bytes in %d seconds\n", sockInfo.recvedBytes, randomSecs);
	} else {
	  fprintf(stderr, "recv failed in round %d with random %d\n", i, randomSecs);
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	}

	int packetNum = sniffPanel.packetNum;
	int payloadSize = sniffPanel.payloadSize;

	//send cancel or close signal
	const char *signal = isCancel? STOP_MSG : CLOSE_MSG;
	if (sendRequest(&sockInfo, signal, strlen(signal)) == 0) {
	  printf("finish sending signal : %s\n", signal);
	} else {
	  fprintf(stderr, "failed to send signal\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	}

	//decide whether to close sock immediately after sending out cancel signal
	if (!isCancel) {
	  printf("shutdown for close test\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
	}
	//record log
	if (appendLog(logFile, &sockInfo, randomSecs) == 0) {
	  printf("finish appending new log\n");
	} else {
	  fprintf(stderr, "failed to append new log\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	}

	FILE * logFd =  fopen(logFile, "a");
	if (logFd == NULL) {
	  fprintf(stderr, "failed to append new log\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	} else {
	  char logStr[1024] = {0};
	  snprintf(logStr, sizeof logStr - 1, "%d,%d", packetNum, payloadSize);
	  if (fwrite(logStr, sizeof(char), strlen(logStr), logFd) < strlen(logStr)) {
		fprintf(stderr, "failed to append new log\n");
		shutdown(sockInfo.clientSockFd, SHUT_RDWR);
		i--;
		continue;
	  }
	  fclose(logFd);
	}

	//sleep and wait for finishing packets sniff
	int sleepTime = 15;
	printf("start to sleep for %d seconds\n", sleepTime);
	sleep(sleepTime);
	if (stopSniff(&sniffPanel) != 0) {
	  fprintf(stderr, "stop sniff failed\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
      i--;
      continue;
	} else {
	  printf("finish stopping sniff with packets: %d, payloadSize: %d\n", sniffPanel.packetNum, sniffPanel.payloadSize);
	}
	//if is cancel test, close sock after waiting some time 
	if (isCancel) {
	  printf("shutdown connection for cancel test\n");
	  shutdown(sockInfo.clientSockFd, SHUT_RDWR);
	}
	close(sockInfo.clientSockFd);
  }
}

//recv response
int recvWithInRandom(struct ClientSockInfo *sockInfo, int random) {
  char recvBuffer[65536] = {0};
  struct timeval startTime;
  struct timeval curTime;
  gettimeofday(&startTime, NULL);
  unsigned int recvedBytes = 0;
  int maxFd = sockInfo->maxFd;
  int clientSockFd = sockInfo->clientSockFd;
  while (1) {
	gettimeofday(&curTime, NULL);
	unsigned int timeCost = (curTime.tv_sec - startTime.tv_sec) * 1000000 + (curTime.tv_usec - startTime.tv_usec);
	if (timeCost >= random * 1000000) {
	  break;
	}
	// read data and append recvedBytes
	fd_set readFds = sockInfo->originalFds;
	select(maxFd, &readFds, NULL, NULL, NULL);
	if (FD_ISSET(clientSockFd, &readFds)) {
	  int tempNum = 0;
	  memset(recvBuffer, 0, sizeof recvBuffer);
	  tempNum = recv(clientSockFd, recvBuffer, sizeof recvBuffer - 1, 0);
	  if (tempNum <= 0) {
		return 1;
	  }
	  //printf("recved %d bytes in %s\n", tempNum, __func__);
	  recvedBytes += tempNum;
	}
  }
  sockInfo->recvedBytes = recvedBytes;
  return 0;
}
//append log
int appendLog(char *logFile, struct ClientSockInfo *sockInfo, int random) {
  char logStr[1024] = {0};
  // append log
  FILE * fp = fopen(logFile, "a");
  if (!fp) {
	fprintf(stderr, "open file failed: %s in %s\n", logFile, __func__);
	return 1;
  }
  sprintf(logStr, "%d,%d,", random, sockInfo->recvedBytes);
  if (fwrite(logStr, sizeof(char), strlen(logStr), fp) < strlen(logStr)) {
	fprintf(stderr, "append file failed: %s in %s\n", logFile, __func__);
	return 1;
  }
  fclose(fp);
  return 0;
}
//generate random number
int generateRandom(int randomRange) {
  srand(time(NULL));
  return rand() % RANDOM_RANGE + 1;
}
int sendRequest(struct ClientSockInfo *sockInfo, const char * msg, int size) {
  fd_set writeFds = sockInfo->originalFds;
  int maxFd = sockInfo->maxFd;
  int clientSockFd = sockInfo->clientSockFd;
  select(maxFd, NULL, &writeFds, NULL, NULL);
  if (FD_ISSET(clientSockFd, &writeFds) ) {
	//send request file name
	if (send(clientSockFd, msg, size, 0) <= 0) {
	  return 1;
	}
	return 0;
  }
  return 1;
}

/**
*@description creat a logfile name
*@param resultStr char * store the created name
*@param maxSize int the max size of resultStr
*/
int createLogfileName(char * resultStr, int maxSize, int isCancel, const char *prefix) {
  time_t now;
  struct tm * timeInfo;
  time(&now);
  timeInfo = localtime(&now);

  char timeFormat[] = "%Y_%m_%d_%H_%M";
  char timeStr[1024] = {0};
  strftime(timeStr, 1023, timeFormat, timeInfo);
  if (isCancel) {
	snprintf(resultStr, maxSize, "%s_%s_%s.csv", "cancel", prefix,  timeStr);
  } else {
	snprintf(resultStr, maxSize, "%s_%s_%s.csv", "close", prefix, timeStr);
  }
  return 0;
}

int initLogFile(char *logFile, int maxSize, int isCancel, const char *prefix) {
  //initialize logfile
  createLogfileName(logFile, maxSize, isCancel, prefix);
  FILE * fd = fopen(logFile, "w");
  if (!fd) {
	return 1;
  }
  char logStr[1024] = {0};
  char *cancelStr;
  if (isCancel) {
	cancelStr = STOP_MSG;
  } else {
	cancelStr = CLOSE_MSG;
  }
  sprintf(logStr, "timeDuration, bytesRecvedOfApplication, sniffedSizeWhen%s, sniffPacketNumWhen%s, sniffedSize, sniffedPacketNum\n", cancelStr, cancelStr);
  if (fwrite(logStr, sizeof (char), strlen(logStr), fd) <= 0) {
	return 1;
  }
  fclose(fd);
  return 0;
}

int initClientSockInfo(struct ClientSockInfo *sockInfo, char *serverIP, char *serverPort) {
  sockInfo->len_sockaddr_in = sizeof(struct sockaddr_in);
  sockInfo->serverIp = inet_addr(serverIP);
  sockInfo->serverPort = atoi(serverPort);
  sockInfo->serverAddr_in.sin_family      = AF_INET;
  sockInfo->serverAddr_in.sin_port        = htons(sockInfo->serverPort);
  sockInfo->serverAddr_in.sin_addr.s_addr = sockInfo->serverIp;
  memset(sockInfo->serverIpStr, 0, sizeof(sockInfo->serverIpStr));
  inet_ntop(AF_INET, &(sockInfo->serverAddr_in.sin_addr), sockInfo->serverIpStr, sizeof(sockInfo->serverIpStr) - 1);
  return 0;
}

//called after sniff
void cbAfterSniff(const struct SniffPanel *panel, void * userArgs) {
  char * logFilePath = (char *)userArgs;
  char logStr[1024] = {0};
  // append log
  FILE * fp = fopen(logFilePath, "a");
  if (!fp) {
	fprintf(stderr, "open file failed: %s in %s\n", logFilePath, __func__);
	return;
  }
  sprintf(logStr, "%d,%d\n", panel->packetNum, panel->payloadSize);
  if (fwrite(logStr, sizeof(char), strlen(logStr), fp) < strlen(logStr)) {
	fprintf(stderr, "append log failed: %s in %s\n", logFilePath, __func__);
	return;
  }
  fclose(fp);
  return;
}
//called when error happened in sniff
void cbForSniffError(const struct SniffPanel *panel,const int errorCode, char * errorMsg, void * userArgs) {
  fprintf(stderr, "sniff error happened with code %d and msg %s\n", errorCode, errorMsg);
}
//output sniff log
void cbForSniffLog(const char * logMsg) {
 //printf("log from sniff: %s", logMsg);
 //printf("this is %s\n", __func__);
}
/* thread function for packet sniff*/
void * sniffThreadFunc( void * sniffPanel) {
  runSniff((struct SniffPanel *)sniffPanel);
  return NULL;
}

//create new tcp connection
int createNewConn(struct ClientSockInfo * sockInfo) {
  int clientSockFd;
  struct sockaddr_in serverAddr_in = sockInfo->serverAddr_in;
  int len_sockaddr_in = sockInfo->len_sockaddr_in;
  char * serverIpStr = sockInfo->serverIpStr;
  int serverPort = sockInfo->serverPort;
  //sleep and wait for finishing packets sniff
  clientSockFd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(clientSockFd,(struct sockaddr*) &serverAddr_in,(socklen_t)len_sockaddr_in) != 0){
	printf("failed to connect to server %s:%d \n", serverIpStr, serverPort);
	close(clientSockFd);
	return 1;
  }
  struct sockaddr_in localAddr;
  int sockaddr_len = sizeof (struct sockaddr_in);
  memset(&localAddr, 0, sizeof (localAddr));
  getsockname(clientSockFd, (struct sockaddr*)&localAddr, (socklen_t *)&sockaddr_len);
  int clientPort = ntohs(localAddr.sin_port);
  //printf("connect to server %s:%d successfully from port %d\n", serverIpStr, serverPort, clientPort);
  int maxFd = clientSockFd + 1;
  fd_set originalFds;
  FD_ZERO(&originalFds);
  FD_SET(clientSockFd, &originalFds);
  sockInfo->clientSockFd = clientSockFd;
  sockInfo->clientAddr_in = localAddr;
  sockInfo->clientPort = clientPort;
  sockInfo->maxFd      = maxFd;
  sockInfo->originalFds = originalFds;
  return 0;
}
