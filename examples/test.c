#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

int main(int argc, char * argv[]) {
  int runNum;
  char command[512];
  struct timeval startTime;
  struct timeval endTime;
  FILE * logFile1;
  FILE * logFile2;
  char firstRow[] = "url,dataLen,connId,timeForConnect,timeForRequest,timeForResponse,timeForRequestAndResponse\n";
  if (argc < 5) {
	printf("Usage : ./test runNum command logfile1 logfile2\n");
	return 1;
  }

  // rewrite log file to store new data
  logFile1 = fopen(argv[3], "w+");
  logFile2 = fopen(argv[4], "w+");
  if (logFile1 == NULL) {
	printf("open file %s failed \n", argv[3]);
  } else {
	fprintf(logFile1, "%s", firstRow);
	fclose(logFile1);
  }
  if (logFile2 == NULL) {
	printf("open file %s failed \n", argv[4]);
  } else {
	fprintf(logFile2, "%s", firstRow);
	fclose(logFile2);
  }
  memset(command, 0, sizeof command);
  sprintf(command, "%s %s %s", argv[2], argv[3], argv[4]);
  printf("%s\n", command);
  runNum = atoi(argv[1]);
  gettimeofday(&startTime, NULL);
  int i;
  for (i = 0; i< runNum; i++) {
	system(command);
  }

  gettimeofday(&endTime, NULL);
  int timeCostInSec = endTime.tv_sec - startTime.tv_sec;
  int timeCostInUsec = endTime.tv_usec - startTime.tv_usec;
  if ( timeCostInUsec < 0) {
	timeCostInSec--;
	timeCostInUsec += 1000000;
  }
  printf("cost %ds %dus to run command %s for %d times\n", timeCostInSec, timeCostInUsec, argv[2], runNum); 
  return 0;
}
