// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/select.h>

using namespace std;

#include "gameswf_log.h"
#include "gameswf_xmlsocket.h"
using namespace gameswf;

XMLSocket::XMLSocket()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  _data = false;
  _xmldata = false;
  _closed = false;
  _connect = false;
  _port = 0;
  _sockfd = 0;
}

XMLSocket::~XMLSocket()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

bool
XMLSocket::connect(const char *host, int port)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  struct sockaddr_in  sock_in;
  fd_set              fdset;
  struct timeval      tval;
  int                 ret;
  int                 retries;
  char                thishostname[MAXHOSTNAMELEN];
  struct protoent     *proto;

  if (port < 1024) {
    log_error("Can't connect to priviledged port #%d!\n", port);
    _connect = false;
    return false;
  }
  
  memset(&sock_in, 0, sizeof(struct sockaddr_in));
  memset(&thishostname, 0, MAXHOSTNAMELEN);
  if (strlen(host) == 0) {
    if (gethostname(thishostname, MAXHOSTNAMELEN) == 0) {
      log_msg("The hostname for this machine is %s.\n", thishostname);
    } else {
      log_msg("Couldn't get the hostname for this machine!\n");
      return false;
    }   
  }
  const struct hostent *hent = ::gethostbyname(host);
  if (hent > 0) {
    ::memcpy(&sock_in.sin_addr, hent->h_addr, hent->h_length);
  }
  sock_in.sin_family = AF_INET;
  sock_in.sin_port = ntohs(static_cast<short>(port));

#if 0
    char ascip[32];
    inet_ntop(AF_INET, &sock_in.sin_addr.s_addr, ascip, INET_ADDRSTRLEN);
      log_msg("The IP address for this client socket is %s\n", ascip);
#endif

  proto = ::getprotobyname("TCP");

  _sockfd = ::socket(PF_INET, SOCK_STREAM, proto->p_proto);
  if (_sockfd < 0)
    {
      log_error("unable to create socket : %s\n", strerror(errno));
      _sockfd = -1;
      return false;
    }

  retries = 2;
  while (retries-- > 0) {
    // We use select to wait for the read file descriptor to be
    // active, which means there is a client waiting to connect.
    FD_ZERO(&fdset);
    FD_SET(_sockfd, &fdset);
    
    // Reset the timeout value, since select modifies it on return. To
    // block, set the timeout to zero.
    tval.tv_sec = 5;
    tval.tv_usec = 0;
    
    ret = ::select(_sockfd+1, &fdset, NULL, NULL, &tval);
    
    // If interupted by a system call, try again
    if (ret == -1 && errno == EINTR)
      {
        log_msg("The connect() socket for fd #%d was interupted by a system call!\n",
                _sockfd);
        continue;
      }
    
    if (ret == -1)
      {
        log_msg("The connect() socket for fd #%d never was available for writing!\n",
                _sockfd);
        ::shutdown(_sockfd, SHUT_RDWR);
        _sockfd = -1;      
        return false;
      }
    if (ret == 0) {
      log_error("The connect() socket for fd #%d timed out waiting to write!\n",
                _sockfd);
      continue;
    }
    if (ret > 0) {
      ret = ::connect(_sockfd, reinterpret_cast<struct sockaddr *>(&sock_in), sizeof(sock_in));
      if (ret == 0) {
        log_msg("\tport %d at IP %s for fd #%d\n", port,
                ::inet_ntoa(sock_in.sin_addr), _sockfd);
        _connect = true;
        return true;
      }
      if (ret == -1) {
        log_msg("The connect() socket for fd #%d never was available for writing!\n",
                _sockfd);
        _sockfd = -1;      
        return false;
      }
    }
  }
  //  ::close(_sockfd);
  //  return false;

  log_msg("\tConnect at port %d on IP %s for fd #%d\n", port,
          ::inet_ntoa(sock_in.sin_addr), _sockfd);
  
  _connect = true;
  return true;
}

void
XMLSocket::close()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  // Since the return code from close() doesn't get used by Shockwave,
  // we don't care either.
  if (_sockfd > 0) {
    ::close(_sockfd);
  }
}

// Return true if there is data in the socket, otherwise return false.
bool
XMLSocket::anydata(tu_string &data)
{
  fd_set                fdset;
  struct timeval        tval;
  int                   ret = 0, i;
  char                  buf[512];

  memset(buf, 0, 512);
  
  // log_msg("%s: \n", __PRETTY_FUNCTION__);

  if (_sockfd <= 0) {
    return false;
  }
  
  
  if (_sockfd > 0) {
    while (ret <= 0) {
      FD_ZERO(&fdset);
      FD_SET(_sockfd, &fdset);
      
      tval.tv_sec = 1;
      tval.tv_usec = 0;
      
      ret = ::select(_sockfd+1, &fdset, NULL, NULL, &tval);
      
      // If interupted by a system call, try again
      if (ret == -1 && errno == EINTR) {
        log_msg("The socket for fd #%d was interupted by a system call!\n",
                _sockfd);
        continue;
      }
      if (ret == -1) {
        log_msg("The socket for fd #%d never was available!\n",
                  _sockfd);
          return false;
      }
      if (ret > 0) {
        log_msg("There is data in the socket for fd #%d!\n",
                  _sockfd);        
        break;
      }
      if (ret == 0) {
        log_msg("There is no data in the socket for fd #%d!\n",
                _sockfd);      
        return false;
      }
    }
    // Read the data from the socket
    //ret = ::read(_sockfd, buf, 512);
    char ch;
    i = 0;
    while (ret = ::read(_sockfd, &ch, 1)) {
      if (i >= 512) {
        break;
      }
      // log_msg("*%c", ch);
      if (ch != '\0') {
        buf[i++] = ch;
      } else {
        ::read(_sockfd, &ch, 1);
        break;
      }
    }

    data = buf;
    return true;
  }
}

bool
XMLSocket::send(tu_string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  int ret = write(_sockfd, str.c_str(), str.size());

  if (ret == str.size()) {
    return true;
  } else {
    return false;
  }
}


// Callbacks

void
XMLSocket::onClose(string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onConnect(string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onData(string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onXML(string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
xmlsocket_connect(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;
  static bool first = true;     // This event handler should only be executed once.
  bool          ret;

  if (!first) {
    result->set(true);
    return;
  }
  
  log_msg("%s:\n", __PRETTY_FUNCTION__);
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*) this_ptr;
  assert(ptr);
  const tu_string host = env->bottom(first_arg).to_string();
  double port = env->bottom(first_arg-1).to_number();
  ret = ptr->obj.connect(host, static_cast<int>(port));

#if 1
  if (ret) {
    env->set_variable("success", true, 0);
  }
  else {
    env->set_variable("success", false, 0);
  }
#endif

#if 1
  if (this_ptr->get_member("onConnect", &method)) {
    //    log_msg("FIXME: Found onConnect!\n");
    as_c_function_ptr	func = method.to_c_function();
    first = false;
    //env->set_variable("success", true, 0);
    env->bottom(0) = true;

    if (func) {
      // It's a C function.  Call it.
      log_msg("Calling C function for onConnect\n");
      (*func)(&val, this_ptr, env, nargs, first_arg);
    }
    else if (as_as_function* as_func = method.to_as_function()) {
      // It's an ActionScript function.  Call it.
      log_msg("Calling ActionScript function for onConnect\n");
      (*as_func)(&val, this_ptr, env, nargs, first_arg);
    } else {
      log_error("error in call_method(): method is not a function\n");
    }    
  } else {
    ptr->set_event_handler(event_id::SOCK_CONNECT, (as_c_function_ptr)&xmlsocket_event_connect);
  }
#else
   ptr->set_event_handler(event_id::SOCK_CONNECT, (as_c_function_ptr)&xmlsocket_event_connect);
#endif
  
  result->set(true);
}


void
xmlsocket_send(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;
  
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*) this_ptr;
  assert(ptr);
  const tu_string object = env->bottom(first_arg).to_string();
  //  log_msg("%s: host=%s, port=%g\n", __PRETTY_FUNCTION__, host, port);
  result->set(ptr->obj.send(object));
}

void
xmlsocket_close(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;
  
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*) this_ptr;
  assert(ptr);
  // Since the return code from close() doesn't get used by Shockwave,
  // we don't care either.
  ptr->obj.close();
}

void
xmlsocket_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  // round argument to nearest int.
  log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
}


// This is the default event handler, and is usually redefined in the SWF script
void
xmlsocket_event_ondata(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: nargs is %d\n", __PRETTY_FUNCTION__, nargs);
    
  as_value	method;
  as_value	val;
  tu_string     data;
  
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*)this_ptr;
  assert(ptr);

  if (ptr->obj.anydata(data)) {
    if (this_ptr->get_member("onData", &method)) {
      log_msg("Got data from socket!!\n%s", data.c_str());
      env->push(as_value(data));
      as_c_function_ptr	func = method.to_c_function();
      if (func) {
        // It's a C function.  Call it.
        log_msg("Calling C function for onData\n");
        (*func)(&val, this_ptr, env, 1, 0);
      } else if (as_as_function* as_func = method.to_as_function()) {
        // It's an ActionScript function.  Call it.
        log_msg("Calling ActionScript function for onData\n");
        (*as_func)(&val, this_ptr, env, 1, 0);
      } else {
        log_error("error in call_method(): method is not a function\n");
      }
      env->pop();
    } else {
      log_msg("FIXME: Couldn't find onData!\n");
    }
  }

  //  env->pop();

  result->set(&data);
}

void
xmlsocket_event_close(as_value* result, as_object_interface* this_ptr, as_environment* env)
{
  
}
void
xmlsocket_event_connect(as_value* result, as_object_interface* this_ptr, as_environment* env)
{
    
  as_value	method;
  as_value	val;
  tu_string     data;
  static bool first = true;     // This event handler should only be executed once.

  if (!first) {
    result->set(true);
    return;
  }
  
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*) this_ptr;
  assert(ptr);

  log_msg("%s: connected = %d\n", __PRETTY_FUNCTION__, ptr->obj.connected());
  if ((ptr->obj.connected()) && (first)) {
    first = false;
    //env->set_variable("success", true, 0);
    //env->bottom(0) = true;

    if (this_ptr->get_member("onConnect", &method)) {
      as_c_function_ptr	func = method.to_c_function();
      if (func)
        {
          // It's a C function.  Call it.
          log_msg("Calling C function for onConnect\n");
          (*func)(&val, this_ptr, env, 0, 0);
      }
      else if (as_as_function* as_func = method.to_as_function())
        {
          // It's an ActionScript function.  Call it.
          log_msg("Calling ActionScript function for onConnect\n");
          (*as_func)(&val, this_ptr, env, 0, 0);
        }
      else
        {
          log_error("error in call_method(): method is not a function\n");
        }    
    } else {
      log_msg("FIXME: Couldn't find onConnect!\n");
    }
  }
  
  result->set(&val); 
}
void
xmlsocket_event_xml(as_value* result, as_object_interface* this_ptr, as_environment* env)
{
  
}
