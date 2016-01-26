/***********************************************
*
*Filename: util.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2016-01-25 13:32:19
# Last Modified: 2016-01-26 10:15:58
************************************************/
/**use select to decide whether the sock is ready for reading or not*/
int canRead(int sockFd);

/**use select to decide whether the sock is ready for writing or not*/
int canWrite(int sockFd);

/** wait for the sock to be ready for reading until timeout, if timeout is NULL, will wait forever*/
int waitForRead(int sockFd, struct timeval *timeout);

/** wait for the sock to be ready for writing until timeout, if timeout is NULL, will wait forever*/
int waitForWrite(int sockFd, struct timeval *timeout);

/** wait for connection close by the other side*/
int waitForClose(int sockFd);

int checkFileExist(char * filePath);

// get file size
int getFileSize(char * filePath);

/** receive from sock */
int recvFromSock(int sockFd, char * buffer, int maxSize, int isBlock);

/** send to  sock */
int sendToSock(int sockFd, char * buffer, int length, int isBlock);

/** comppletely close socket*/
int closeSock(int sockFd);


