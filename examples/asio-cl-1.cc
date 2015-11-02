/***********************************************
*
*Filename: asio-cl-1.cc
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-28 13:57:09
*Last Modified: 2015-09-28 13:57:09
************************************************/

#include "asio-cl.h"

int main(int argc, char *argv[]) {
  try {
    if (argc < 10) {
      std::cerr << "Usage: asio-cl URI get1 get2 logfile1 logfile2 roundNum logDir mediumLen1 mediumLen2" << std::endl;
      return 1;
    }
    boost::system::error_code ec;
    boost::asio::io_service io_service;

	requestInfo reqInfo1, reqInfo2;
	sessionInfo sessInfo;
	reqInfo1.mediumLen = atoi(argv[8])/2;
	reqInfo2.mediumLen = atoi(argv[9])/2;
	memset(reqInfo1.logFile, 0, sizeof (reqInfo1.logFile));
	memset(reqInfo2.logFile, 0, sizeof (reqInfo2.logFile));
	sprintf(reqInfo1.logFile, "%s/one_%s_%s.csv", argv[7], argv[2], argv[4]);
	sprintf(reqInfo2.logFile, "%s/one_%s_%s.csv", argv[7], argv[3], argv[5]);
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
    auto sessId = scheme == "https" ? sessionManager.startNewSession(io_service, tls_ctx, host, service)
                                  : sessionManager.startNewSession(io_service, host, service);
								  

	if (sessId < 0) {
	  std::cerr << "error: create new session error" << std::endl;
	  return 1;
	}

	auto sess = sessionManager.getSession(sessId);
	sessInfo.sessId = sessId;
	sessInfo.sess   = sess;
	gettimeofday(&(sessInfo.sessionStart), NULL);
	sessInfo.requestNum = 2;
	sessInfo.requestList[0] = &reqInfo2;
	sessInfo.requestList[1] = &reqInfo1;
	sessInfo.roundNum   = atoi(argv[6]);
	sessInfo.isConcurrent = 0;
	sessInfo.currentRequestNum = 0;
	sessInfo.leftRoundNum    = sessInfo.roundNum;
	initSession(&sessInfo);
	// start to run the session
    io_service.run();
  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

