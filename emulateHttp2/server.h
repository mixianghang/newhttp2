/***********************************************
*
*Filename: server.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2016-01-25 12:37:25
# Last Modified: 2016-01-25 14:52:43
************************************************/
#define MAX_CONNECTION_QUEUE 10
#define CANCEL_MSG "cancel"
#define CLOSE_MSG "close"

typedef struct RequestInfo {
  int sockFd;
  char clientIP[32];
  int  clientPort;
} RequestInfo;

/** used by thread to process new request*/
void * processRequest(void * requestInfo);

//create a logfile name 
int createLogfileName(char * resultStr, int maxSize);

