/***********************************************
*
*Filename: asio-cl-2.cc
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-28 14:48:25
*Last Modified: 2015-09-28 14:48:25
************************************************/

#include "asio-cl.h"

int main(int argc, char *argv[]) {
  try {
    if (argc < 10) {
      std::cerr << "Usage: asio-cl-2 URI get1 get2 logfile1 logfile2 roundNum logDir mediumLen1 mediumLen2" << std::endl;
      return 1;
    }
    boost::system::error_code ec;
    boost::asio::io_service io_service1;
    boost::asio::io_service io_service2;
	requestInfo reqInfo1, reqInfo2;
	sessionInfo sessInfo1, sessInfo2;
	reqInfo1.mediumLen = atoi(argv[8])/2;
	reqInfo2.mediumLen = atoi(argv[9])/2;
	memset(reqInfo1.logFile, 0, sizeof (reqInfo1.logFile));
	memset(reqInfo2.logFile, 0, sizeof (reqInfo2.logFile));
	sprintf(reqInfo1.logFile, "%s/two_%s_%s.csv", argv[7], argv[2], argv[4]);
	sprintf(reqInfo2.logFile, "%s/two_%s_%s.csv", argv[7], argv[3], argv[5]);
	FILE * logFile1;
	FILE * logFile2;
	char firstRow[] = "url,dataLen,connId,timeForConnect,timeForRequest,timeForResponse,timeForRequestAndResponse\n";
	// rewrite log file to store new data
	logFile1 = fopen(reqInfo1.logFile, "w+");
	logFile2 = fopen(reqInfo2.logFile, "w+");
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
    std::string uri = argv[1];
	reqInfo1.uri = uri + "/" + argv[2];
	reqInfo2.uri = uri + "/" + argv[3];
	reqInfo1.responseLen = 0;
	reqInfo2.responseLen = 0;
    std::string scheme, host, service;
    if (host_service_from_uri(ec, scheme, host, service, uri)) {
      std::cerr << "error: bad URI: " << ec.message() << std::endl;
      return 1;
    }

    boost::asio::ssl::context tls_ctx(boost::asio::ssl::context::sslv23);
    tls_ctx.set_default_verify_paths();
    // disabled to make development easier...
    // tls_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    configure_tls_context(ec, tls_ctx);

	session_manager sessionManager{};
    auto sessId1 = scheme == "https" ? sessionManager.startNewSession(io_service1, tls_ctx, host, service)
                                  : sessionManager.startNewSession(io_service1, host, service);
    auto sessId2 = scheme == "https" ? sessionManager.startNewSession(io_service1, tls_ctx, host, service)
                                  : sessionManager.startNewSession(io_service2, host, service);
								  

	if (sessId1 < 0) {
	  std::cerr << "error: create new session error" << std::endl;
	  return 1;
	}
	if (sessId2 < 0) {
	  std::cerr << "error: create new session error" << std::endl;
	  return 1;
	}

	auto sess1 = sessionManager.getSession(sessId1);
	auto sess2 = sessionManager.getSession(sessId2);

	// initial start time of those two sessions
	gettimeofday(&(sessInfo1.sessionStart), NULL);
	gettimeofday(&(sessInfo2.sessionStart), NULL);

	// initialize sessInfo1
	sessInfo1.sessId = sessId1;
	sessInfo1.sess   = sess1;
	sessInfo1.roundNum = atoi(argv[6]);
	sessInfo1.leftRoundNum = sessInfo1.roundNum;
	sessInfo1.requestNum = 1;
	sessInfo1.isConcurrent = 1;

	// initialize sessInfo2
	sessInfo2.sessId = sessId2;
	sessInfo2.sess   = sess2;
	sessInfo2.roundNum = atoi(argv[6]);
	sessInfo2.leftRoundNum = sessInfo2.roundNum;
	sessInfo2.requestNum = 1;
	sessInfo2.isConcurrent = 1;

	// set requestList
	sessInfo1.requestList[0] = &reqInfo1;
	sessInfo2.requestList[0] = &reqInfo2;
	initSession(&sessInfo1);
	initSession(&sessInfo2);
	pthread_t thread1, thread2;
	pthread_create(&thread1,NULL,ioservice,(void *)&io_service1);
	pthread_create(&thread1,NULL,ioservice,(void *)&io_service2);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
