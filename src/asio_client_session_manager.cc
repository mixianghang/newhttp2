/***********************************************
*
*Filename: asio_client_session_manager.cc
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-22 11:34:50
*Last Modified: 2015-09-22 11:34:50
************************************************/
#include <nghttp2/asio_client_session_manager.h>
#include <cstdio>
#include "template.h"
#include "nghttp2_config.h"
namespace nghttp2 {
namespace asio_http2 {
namespace client {

/**
*constructor
*/
session_manager::session_manager() : _sessionIndex(0) {
}

/**
*destructor
*/
session_manager::~session_manager() {
}

/**
*start new session
*/
int32_t session_manager::startNewSession(boost::asio::io_service &io_service, const std::string host, const std::string service) {
  if (_sessions.size() >= MAX_SESSION_NUM) { // check current session num
	return -1;
  }
  std::unique_ptr<session> currentSession = make_unique<session>(io_service, host, service);
  auto result = _sessions.emplace(++(this->_sessionIndex), std::move(currentSession));
  if (!(result.second)) {
	return -1;
  }
  return this->_sessionIndex;
}

/**
*start new session
*/
int32_t session_manager::startNewSession(boost::asio::io_service &io_service, boost::asio::ssl::context &tls_service, const std::string host, const std::string service) {
  if (_sessions.size() >= MAX_SESSION_NUM) {
	return -1;
  }
  std::unique_ptr<session> currentSession = make_unique<session>(io_service, tls_service, host, service);
  auto result = _sessions.emplace(++(this->_sessionIndex), std::move(currentSession));
  if (!(result.second)) {
	return -1;
  }
  return this->_sessionIndex;
}

/**
*delete existing session
*/
int32_t session_manager::deleteSession(int32_t sessionIndex) {
  auto result = _sessions.erase(sessionIndex);
  if (result != 1) {
	return -1;
  }
  return sessionIndex;
}
/**
*get session by session id
*/
session * session_manager::getSession(int32_t sessionId) {
  return (_sessions[sessionId]).get();
}

/**
*get random sessionId
*/
int32_t session_manager::getRandomSessionId(int32_t random) {
  int size = _sessions.size();
  return (random % size + 1);
}

} // client namespace
} // asio namespace
} // http2 namespace
