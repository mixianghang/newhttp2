/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2015 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <cstdio>
#include <ctime>
#include <pthread.h>

//#include <nghttp2/asio_http2_client.h>
#include <nghttp2/asio_client_session_manager.h>

using boost::asio::ip::tcp;

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::client;
using namespace std;


int initSession(session * sess, int sessId, std::string uri);
int submitRequest(session * sess, const std::string uri, int sessId);
void * getRequest(void *);

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: asio-cl URI" << std::endl;
      return 1;
    }
    boost::system::error_code ec;
    boost::asio::io_service io_service;

    std::string uri = argv[1];
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
    auto sessId1 = scheme == "https" ? sessionManager.startNewSession(io_service, tls_ctx, host, service)
                                  : sessionManager.startNewSession(io_service, host, service);
								  

	if (sessId < 0) {
	  std::cerr << "error: create new session error" << std::endl;
	  return 1;
	}
	if (sessId1 < 0) {
	  std::cerr << "er or: create new session1 error" << std::endl;
	  return 1;
	}

	auto sess = sessionManager.getSession(sessId);
	auto sess1 = sessionManager.getSession(sessId1);
	initSession(sess, sessId, uri);
	initSession(sess1, sessId1, uri);
	pthread_t readthread;
	pthread_create(&readthread, NULL, getRequest, (void*)(&sessionManager));
    io_service.run();
	pthread_join(readthread,NULL);
  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

int initSession(session * sess, int sessId, std::string uri) {
    sess->on_connect([sess,sessId, uri](tcp::resolver::iterator endpoint_it) {
	  cout << "session " << sessId << "is connected" << std::endl;
	  cout << "it is time to input request url"  << std::endl;
	  return 0;
	  submitRequest(sess, uri, sessId);
	  std::string tempUri = "http://www.nghttp2.org/httpbin";
	  submitRequest(sess, tempUri, sessId);
    });
	sess->on_error([sessId](const boost::system::error_code &ec) {
	  std::cerr << "session " << sessId << "error: " << ec.message() << std::endl;
	} );
	return 0;
}

int submitRequest(session * sess, std::string  uri, int sessId) {
	boost::system::error_code ec;
	auto req = sess->submit(ec, "GET", uri);
	if (ec) {
	  std::cerr << "session " << sessId << "error: " << ec.message() << std::endl;
	  return -1;
	}

	req->on_response([sess, uri, sessId](const response &res) {
	  std::cerr << "HTTP/2 " << res.status_code() << std::endl;
	  for (auto &kv : res.header()) {
	    //std::cerr << kv.first << ": " << kv.second.value << "\n";
	  }
	  std::cerr << std::endl;

	  res.on_data([sess, uri, sessId](const uint8_t *data, std::size_t len) {
	    std::cerr.write(reinterpret_cast<const char *>(data), len);
		std::cerr << std::endl;
	    std::cerr << "session" << sessId << uri << "data lenght is " << len << std::endl;
	  });
	});

// we don't need to shutdown the session when a stream starts to shutdown
	req->on_close([sess, uri](uint32_t error_code) { //sess->shutdown(); 
	  printf("uri %s will be closed \n", uri.c_str());
	});
	return 0;
}

/**
*get some 
*/
void * getRequest(void * sessionManager) {
  session_manager * manager = (session_manager *) sessionManager;
  while(1) {
	char uri[256] = {0};
	cin.getline(uri, 256);
	if (strcmp(uri, "stop") == 0) {
	  break;
	}
	cout << uri << std::endl;
	std::string scheme, host, service;
    boost::system::error_code ec;
	if (host_service_from_uri(ec, scheme, host, service, uri)) {
	  std::cout << "error: bad URI: " << ec.message() << std::endl;
	  break;
	}
	time_t random = time(0);
	int32_t sessId = manager -> getRandomSessionId(random);
	session * sess = manager -> getSession(sessId);
	if (sess->isConnected()) {
	  submitRequest(sess,uri,sessId);
	} else {
	  std::cerr << "connect failed" << std::endl;
	}
  }
}
