/***********************************************
*
*Filename: asio_client_session_manager.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-22 07:00:58
*Last Modified: 2015-09-22 07:00:58
************************************************/
#ifndef ASIO_CLIENT_SESSION_MANAGER_H
#define ASIO_CLIENT_SESSION_MANAGER_H

#include <boost/array.hpp>

#include <nghttp2/asio_http2_client.h>


#define MAX_SESSION_NUM 2

namespace nghttp2 {
namespace asio_http2 {
namespace client {

class session_manager {
public :
  session_manager();
  ~session_manager();
  session_manager(const session_manager & ) =delete;
  session_manager & operator=(const session_manager &) =delete;

  /**
  *start new session
  */
  int32_t startNewSession(boost::asio::io_service &io_service, const std::string host, const std::string service);

  /**
  *start new session
  */
  int32_t startNewSession(boost::asio::io_service &io_service, boost::asio::ssl::context &tls_service, const std::string host, const std::string service);

  /**
  *delete existing session
  */
  int32_t deleteSession(int32_t sessionIndex);

  /**
  *get session by session id
  */
  session * getSession(int32_t sessionId);
  
  /**
  *@description get random sessionId
  */
  int32_t getRandomSessionId(int32_t random);

private :
  int32_t _sessionIndex;
  std::map<int32_t, std::unique_ptr<session>> _sessions; 
};//class session_manager

} //namspace client
} // namespace asio
} // namespace nghttp2

#endif//asio_client_session_manager
