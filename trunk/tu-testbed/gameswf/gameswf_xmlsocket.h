#ifndef __XMLSOCKET_H__
#define __XMLSOCKET_H__

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

#include <string>

class XMLSocket {
 public:
  XMLSocket();
  ~XMLSocket();
  
  bool connect(const char *host, int port);
  bool send(tu_string str);
  void close();

  bool anydata(tu_string &data);
  bool connected() { return _connect; };
  bool fdclosed() { return _closed; }
  bool xmlmsg() { return _xmldata; }
  
  
  // Event Handlers
  void onClose(std::string);
  void onConnect(std::string);
  void onData(std::string);
  void onXML(std::string);
 private:
  tu_string     _host;
  short         _port;
  int           _sockfd;
  bool          _data;
  bool          _xmldata;
  bool          _closed;
  bool          _connect;
};


struct xmlsocket_as_object : public gameswf::as_object
{
  XMLSocket obj;
};

void
xmlsocket_connect(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

void
xmlsocket_send(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);


void
xmlsocket_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

void
xmlsocket_close(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

// These are the event handlers called for this object
void xmlsocket_event_ondata(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

void xmlsocket_event_close(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env);

void xmlsocket_event_connect(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env);

void xmlsocket_event_xml(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env);


// __XMLSOCKETSOCKET_H__
#endif
