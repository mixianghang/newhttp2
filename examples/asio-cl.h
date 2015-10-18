#include <iostream>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

//#include <nghttp2/asio_http2_client.h>
#include <nghttp2/asio_client_session_manager.h>
#include <pthread.h>

using boost::asio::ip::tcp;

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::client;
using namespace std;

typedef struct responseInfo{
  struct timeval responseStart;
  struct timeval end;
  struct timeval requestStart;
  struct timeval connectStart;
  int    len = 0;
  char   output[256];
  char   logFile[256]; 
} timeinfo;

typedef struct RequestInfo {
  std::string uri;
  struct timeval requestStart;
  struct timeval responseStart;
  struct timeval responseEnd;
  char logFile[256];
  int  responseLen;
} requestInfo;

typedef struct SessionInfo {
  int sessId;
  session * sess;
  struct timeval sessionStart;// time when session start
  struct timeval connected;
  struct timeval sessionEnd;// time when session end
  requestInfo * requestList[256];
  int requestNum;// how many requests to sent  every round trip
  int roundNum; // how many round trip
  int leftRoundNum;
} sessionInfo;


int submitRequest(session * sess, int sessId, std::string  uri, timeinfo & timeinfo);
int submitRequest(sessionInfo * sessInfo, requestInfo * reqInfo);

int initSession(session * sess, int sessId, std::string uri, std::string get1, std::string get2, timeinfo& get1Info, timeinfo & get2Info);

int initSession(session * sess, int sessId, std::string uri, std::string get1, timeinfo& get1Info);

int initSession(sessionInfo * sessInfo);

int isFileExist(char * filePath);

int writeToFile(char * filePath, char * buffer, int len);


void * ioservice(void * ioservice);


int initSession(session * sess, int sessId, std::string uri, std::string get1, std::string get2, timeinfo& get1Info, timeinfo & get2Info) {
    sess->on_connect([sess,sessId, uri,get1, get2,&get1Info, &get2Info](tcp::resolver::iterator endpoint_it) {
	  gettimeofday(&(get1Info.requestStart), NULL);
	  gettimeofday(&(get2Info.requestStart), NULL);
	  cout << "session " << sessId << "is connected" << std::endl;
	  submitRequest(sess, sessId, get1, get1Info);
	  submitRequest(sess, sessId, get2, get2Info);
    });
	sess->on_error([sessId](const boost::system::error_code &ec) {
	  std::cerr << "session " << sessId << "error: " << ec.message() << std::endl;
	} );
	return 0;
}

int submitRequest (sessionInfo * sessInfo, requestInfo * reqInfo) {
  boost::system::error_code ec;
  session * sess = sessInfo->sess;
  int sessId = sessInfo->sessId;
  auto req = sess->submit(ec, "GET", reqInfo->uri);
  gettimeofday(&(reqInfo->requestStart), NULL);
  if (ec) {
	std::cerr << "session" << sessId << "error" << ec.message() << std::endl;
	return -1;
  }
  req->on_response([sessInfo,reqInfo](const response &res) {
	gettimeofday(&(reqInfo->responseStart), NULL);
	res.on_data([reqInfo](const uint8_t *data, std::size_t len) {
	  reqInfo->responseLen += len;
	});
  });

// we don't need to shutdown the session when a stream starts to shutdown
  req->on_close([sessInfo, reqInfo](uint32_t error_code) { //sess->shutdown(); 
	auto sess = sessInfo->sess;
	int  sessId = sessInfo->sessId;
	int currentRound = sessInfo->roundNum - sessInfo->leftRoundNum + 1;
	auto uri = reqInfo->uri;
	char * logFile = reqInfo->logFile;
	uint32_t timeForConnect;
	uint32_t timeForRequestAndResponse;
	uint32_t timeForResponse;
	uint32_t timeForRequest;
	uint32_t dataLen;
	printf("the %d round of request %s in session %d \n", currentRound, uri.c_str(), sessId);
	gettimeofday(&(reqInfo->responseEnd), NULL);
	timeForResponse           = (reqInfo->responseEnd.tv_sec - reqInfo->responseStart.tv_sec) * 1000000 + 
	reqInfo->responseEnd.tv_usec - reqInfo->responseStart.tv_usec;

	timeForRequestAndResponse = (reqInfo->responseEnd.tv_sec - reqInfo->requestStart.tv_sec) * 1000000 + 
	reqInfo->responseEnd.tv_usec - reqInfo->requestStart.tv_usec;

	timeForRequest            = timeForRequestAndResponse - timeForResponse;
	timeForConnect            = (sessInfo->connected.tv_sec - sessInfo->sessionStart.tv_sec) * 1000000 +
	sessInfo->connected.tv_usec - sessInfo->sessionStart.tv_usec;

	dataLen = reqInfo->responseLen;

	if (isFileExist(logFile)) {
	  FILE * fp = fopen(logFile, "a+");
	  if (fp != NULL) {
		//uri, dataLen, sessId, timeForConnect timeForRequest, timeForResponse, timeForRequestAndResponse
		fprintf(fp, "%s,%d,%d,%d,%d,%d,%d\r\n", uri.c_str(), dataLen, sessId, timeForConnect, timeForRequest, timeForResponse, timeForRequestAndResponse);
		fclose(fp);
	  } else {
		printf("open file %s failed\n", logFile);
	  }
	} else {
	  printf("log file %s doesn't exist\n", logFile);
	}

	printf("timeForConnect of %s is %d useconds\n", uri.c_str(), timeForConnect);
	printf("timeForRequestAndResponse %s is %d useconds\n", uri.c_str(), timeForRequestAndResponse);
	printf("timeForRequestAndResponse %s is %d useconds\n", uri.c_str(), timeForResponse);
	printf("responseLen of %s is %d\n", uri.c_str(), dataLen);

	if (sess->getStreamNum() == 0) { // finish processing all the rounds
	  (sessInfo->leftRoundNum)--;
	  if (sessInfo->leftRoundNum == 0) {
		  sess->shutdown();
		  gettimeofday(&(sessInfo->sessionEnd), NULL);
		  int timeSec = sessInfo->sessionEnd.tv_sec - sessInfo->sessionStart.tv_sec;
		  int timeUsec = sessInfo->sessionEnd.tv_usec - sessInfo->sessionStart.tv_usec;
		  printf("shutdown session %d running for %d rounds of %d requests\n with time cose %ds %d usec", sessId, sessInfo->roundNum, sessInfo->requestNum, timeSec, timeUsec);
	  } else {
		printf("\n\nstart round %d of session %d\n", sessInfo->roundNum - sessInfo->leftRoundNum + 1, sessId);
		int i = 0;
		while (i < sessInfo->requestNum) {
		  sessInfo->requestList[i]->responseLen = 0;
		  submitRequest(sessInfo, sessInfo->requestList[i]);
		  i++;
		}
	  }
	}
	});
	return 0;
}

int submitRequest(session * sess, int sessId, std::string  uri, timeinfo & timeinfo) {
	boost::system::error_code ec;
	auto req = sess->submit(ec, "GET", uri);
	if (ec) {
	  std::cerr << "session " << sessId << "error: " << ec.message() << std::endl;
	  return -1;
	}

	req->on_response([sess,sessId, uri,&timeinfo](const response &res) {
	  gettimeofday(&(timeinfo.responseStart), NULL);
	  //std::cerr << "HTTP/2 " << res.status_code() << std::endl;
	  for (auto &kv : res.header()) {
	    //std::cerr << kv.first << ": " << kv.second.value << "\n";
	  }
	  res.on_data([sess, sessId, uri, &timeinfo](const uint8_t *data, std::size_t len) {
	   // std::cerr.write(reinterpret_cast<const char *>(data), len);
	    //FILE* fd = fopen(timeinfo.output,"a+");
		//if (fd == NULL) {
		//  std::cerr << "wrong file" << std::endl;
		//}
		//if (fd == NULL || fwrite(reinterpret_cast<const char *>(data), sizeof(char), len, fd) < 0) {
		//  std::cerr << "write to file failed" << timeinfo.output << std::endl;
		//} else {
		//  //std::cerr << "write to file" << timeinfo.output << std::endl;
		//}
		//if (fd != NULL) {
		//  fclose(fd);
		//}
		timeinfo.len += len;
	    //std::cerr << "session" << sessId << uri << "data lenght is " << len << std::endl;
	  });
	});

// we don't need to shutdown the session when a stream starts to shutdown
	req->on_close([sess, sessId, uri, &timeinfo](uint32_t error_code) { //sess->shutdown(); 
	  uint32_t timeForConnect;
	  uint32_t timeForRequestAndResponse;
	  uint32_t timeForResponse;
	  uint32_t timeForRequest;
	  uint32_t dataLen;
	  printf("sessId is %d\n", sessId);
	  printf("uri %s will be closed \n", uri.c_str());
	  gettimeofday(&(timeinfo.end), NULL);

	  timeForConnect            = (timeinfo.requestStart.tv_sec - timeinfo.connectStart.tv_sec) * 1000000 + 
	  timeinfo.requestStart.tv_usec - timeinfo.connectStart.tv_usec;

	  timeForResponse           = (timeinfo.end.tv_sec - timeinfo.responseStart.tv_sec) * 1000000 + 
	  timeinfo.end.tv_usec - timeinfo.responseStart.tv_usec;

	  timeForRequestAndResponse = (timeinfo.end.tv_sec - timeinfo.requestStart.tv_sec) * 1000000 + 
	  timeinfo.end.tv_usec - timeinfo.requestStart.tv_usec;

	  timeForRequest            = timeForRequestAndResponse - timeForResponse;

	  dataLen = timeinfo.len;

	  if (isFileExist(timeinfo.logFile)) {
		FILE * fp = fopen(timeinfo.logFile, "a+");
		if (fp != NULL) {
		  //uri, dataLen, sessId, timeForConnect timeForRequest, timeForResponse, timeForRequestAndResponse
		  fprintf(fp, "%s,%d,%d,%d,%d,%d,%d\r\n", uri.c_str(), dataLen, sessId, timeForConnect, timeForRequest, timeForResponse, timeForRequestAndResponse);
		  fclose(fp);
		} else {
		  printf("open file %s failed\n", timeinfo.logFile);
		}
	  } else {
		printf("log file %s doesn't exist\n", timeinfo.logFile);
	  }

	  //printf("time.begin.sec is %d\n",timeinfo.start.tv_sec);
	  //printf("time.end.sec is %d\n",timeinfo.end.tv_sec);
	  //printf("time.begin.usec is %d\n",timeinfo.start.tv_usec);
	  //printf("time.end.usec is %d\n",timeinfo.end.tv_usec);
	  //printf("the time of %s start at %d second %d usec\n", uri.c_str(), 
	  //timeinfo.connectStart.tv_sec, timeinfo.connectStart.tv_usec
	  //);
	  //printf("the time of %s request start at %d second %d usec\n", uri.c_str(), 
	  //timeinfo.requestStart.tv_sec, timeinfo.requestStart.tv_usec
	  //);
	  //printf("the time of %s response end at %d second %d usec\n", uri.c_str(), 
	  //timeinfo.end.tv_sec, timeinfo.end.tv_usec
	  //);
	  printf("the time used for connect %s is %d useconds\n", uri.c_str(), timeForConnect);
	  printf("the time used for request and response %s is %d useconds\n", uri.c_str(), timeForRequestAndResponse);
	  printf("the time used for response %s is %d useconds\n", uri.c_str(), timeForResponse);
	  printf("the lenght of %s is %d\n", uri.c_str(), timeinfo.len);

	  if (sess->getStreamNum() == 0) { // finish processing all the streams
		sess->shutdown();
		printf("shutdown the connection %d\n", sessId);
	  }
	});
	return 0;
}

int initSession(session * sess, int sessId, std::string uri, std::string get1, timeinfo& get1Info) {
    sess->on_connect([sess,sessId, uri,get1,&get1Info](tcp::resolver::iterator endpoint_it) {
	  cout << "session " << sessId << "is connected" << std::endl;
	  gettimeofday(&(get1Info.requestStart), NULL);
	  submitRequest(sess, sessId, get1, get1Info);
    });
	sess->on_error([sessId](const boost::system::error_code &ec) {
	  std::cerr << "session " << sessId << "error: " << ec.message() << std::endl;
	} );
	return 0;
}


int initSession (sessionInfo * sessInfo) {
  sessInfo->leftRoundNum = sessInfo->roundNum;
  auto sess = sessInfo->sess;
  sess->on_connect([sessInfo](tcp::resolver::iterator endpoint_it) {
	cout << "session " << sessInfo->sessId << "is connected" << std::endl;
	gettimeofday(&(sessInfo->connected), NULL);
	int i = 0;
	while (i < sessInfo->requestNum) {
	  submitRequest(sessInfo, sessInfo->requestList[i]);
	  i++;
	}
  });
  sess->on_error([sessInfo](const boost::system::error_code &ec) {
	std::cerr << "session " << sessInfo->sessId << "error: " << ec.message() << std::endl;
  } );
  return 0;
}

void * ioservice(void * ioservice) {
  boost::asio::io_service * service = static_cast<boost::asio::io_service *>(ioservice);
  service->run();
}

int isFileExist(char * filePath) {
  struct stat buffer;
  return stat(filePath, &buffer) == 0;
}

int writeToFile(char * filePath, char * buffer, int len) {
}
