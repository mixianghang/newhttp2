/***********************************************
*
*Filename: client.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-12-28 20:59:31
# Last Modified: 2016-01-25 15:57:29
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
#include <pthread.h>
#include "packetSniff.h"

#define RANDOM_RANGE 10
#define STOP_MSG "cancel"
#define CLOSE_MSG "close"
#define FILE_NAME_SIZE 1024
#define IP_STR_LEN 20

struct ClientSockInfo {
  int clientSockFd;
  int len_sockaddr_in;

  struct sockaddr_in serverAddr_in;
  int serverIp;
  int serverPort;
  char serverIpStr[IP_STR_LEN];

  struct sockaddr_in clientAddr_in;
  int clientPort;
  char clientIpStr[IP_STR_LEN];

  int maxFd;
  fd_set originalFds;

  int recvedBytes;
};

int initialLogFile(int isCancel);

/**
*@description creat a logfile name
*@param resultStr char * store the created name
*@param maxSize int the max size of resultStr
*/
int createLogfileName(char * resultStr, int maxSize, int isCancel, const char *prefix);

int initClientSockInfo(struct ClientSockInfo *sockInfo, char *serverIP, char *serverPort);

void cbAfterSniff(const struct SniffPanel *panel, void * userArgs); 
void cbForSniffError(const struct SniffPanel *panel,const int errorCode, char * errorMsg, void * userArgs); 
void cbForSniffLog(const char * logMsg);

/* thread function for packet sniff*/
void * sniffThreadFunc( void * sniffPanel);

//create new tcp connection
int createNewConn(struct ClientSockInfo * sockInfo);

//send request`
int sendRequest(struct ClientSockInfo *sockInfo, const char * msg, int size);

//generate random number
int generateRandom(int randomRange);
//recv response
int recvWithInRandom(struct ClientSockInfo *sockInfo, int randomSecs);

//append log
int appendLog(char *logFile, struct ClientSockInfo *sockInfo, int randomSecs); 
int initLogFile(char *logFile, int maxSize, int isCancel, const char *prefix);

/** receive until timeout happends **/
int receiveFromSockUtilTimeout(int sockFd, struct timeval *timeout);
