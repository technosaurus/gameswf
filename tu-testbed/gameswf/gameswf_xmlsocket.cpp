// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/select.h>

// FIXME
using namespace std;

#include "gameswf_log.h"
#include "gameswf_xmlsocket.h"
#include "gameswf_timers.h"

#ifdef HAVE_LIBXML

namespace gameswf
{

const int INBUF = 7000;

XMLSocket::XMLSocket()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  _data = false;
  _xmldata = false;
  _closed = false;
  _connect = false;
  _processing = false;
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
  log_msg("%s: to host %s at port %d\n", __PRETTY_FUNCTION__, host, port);
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
  
  fcntl(_sockfd, F_SETFL, O_NONBLOCK);

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
  //printf("%s: \n", __PRETTY_FUNCTION__);
  anydata(_sockfd, data);
}

bool XMLSocket::processingData()
{
  //printf("%s: processing flags is is %d\n", __PRETTY_FUNCTION__, _processing);
  return _processing;
}

void XMLSocket::processing(bool x)
{
  //printf("%s: set processing flag to %d\n", __PRETTY_FUNCTION__, x);
  _processing = x;
}

bool
XMLSocket::anydata(int fd, tu_string &data)
{
  fd_set                fdset;
  struct timeval        tval;
  int                   ret = 0, i;
  char                  buf[INBUF];
  int                   retries = 10;
  char                  *ptr;
  int                   pos;
  bool                  ifdata = false;
  static tu_string             remainder;

  //log_msg("%s: \n", __PRETTY_FUNCTION__);

  if (fd <= 0) {
    return false;
  }
  while (retries-- > 0) {
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    
    tval.tv_sec = 0;
    tval.tv_usec = 10;
    
    ret = ::select(fd+1, &fdset, NULL, NULL, &tval);
    
    // If interupted by a system call, try again
    if (ret == -1 && errno == EINTR) {
      log_msg("The socket for fd #%d was interupted by a system call!\n",
              fd);
      continue;
    }
    if (ret == -1) {
      log_msg("The socket for fd #%d never was available!\n", fd);
      return false;
    }
    if (ret == 0) {
      //log_msg("There is no data in the socket for fd #%d!\n", fd);
      if (ifdata) {
        remainder = ptr;
      }
      return ifdata;
    }
    if (ret > 0) {
      //log_msg("There is data in the socket for fd #%d!\n", fd);        
      //break;
    }
    processing(true);
    memset(buf, 0, INBUF);
    ret = ::read(_sockfd, buf, INBUF);
    //log_msg("%s: read %d bytes\n", __FUNCTION__, ret);
    //log_msg("%s: read (%d,%d) %s\n", __FUNCTION__, buf[0], buf[1], buf);
    ptr = buf;
    pos = 1;
    while (pos > 0) {
      tu_string msg;
      //ptr = strchr('\0', buf);
      if (remainder.size() > 0) {
        //printf("%s: The remainder is: \"%s\"\n", __FUNCTION__, remainder.c_str());
        //printf("%s: The rest of the message is: \"%s\"\n", __FUNCTION__, ptr);
        msg = remainder;
        msg += buf;
        //ptr =
        //printf("%s: The whole message for %d is: \"%s\"\n", __FUNCTION__, _messages.size(), msg.c_str());
        remainder.resize(0);
      } else {
        msg = ptr;
        //        strcpy(msg, ptr);
      }
      //if (msg[strlen(msg)-2] == '>')
      //if (((msg[0] == '<') && (msg[1] == '?')) && ((msg[msg.size()-2] == '>') && (msg[msg.size()-3] == 'T'))) {
      // if (strchr(ptr, '\n')) {
      if (ptr[strlen(ptr)-1] == '\n') {
        ifdata = true;
#if 0
        printf("%s: Adding message %d: \"%s\"\n", __FUNCTION__, _messages.size(), msg.c_str());
        printf("%s: message (%d) last three bytes are %d, %d, %d: \n", __FUNCTION__,
               strlen(ptr),
               ptr[strlen(ptr)-2],
               ptr[strlen(ptr)-1],
               ptr[strlen(ptr)]);
#endif
        _messages.push_back(msg);
        pos = strlen(ptr);
        ptr += pos + 1;
        //remainder.resize(0);
      } else {
        remainder = ptr;
        if (remainder.size() > 0) {
          //printf("%s: Adding remainder: \"%s\"\n", __FUNCTION__, remainder.c_str());
        } else {
          remainder = "";
        }
        break;
      }
    }
  }
  
  data = buf;

  return true;
}

bool
XMLSocket::send(tu_string str)
{
  //log_msg("%s: \n", __PRETTY_FUNCTION__);
  str += tu_string( "\0", 1);
  int ret = write(_sockfd, str.c_str(), str.size());

  //log_msg("%s: sent %d bytes, data was %s\n", __FUNCTION__, ret, str.c_str());
  if (ret == str.size()) {
    return true;
  } else {
    return false;
  }
}


// Callbacks

void
XMLSocket::onClose(tu_string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onConnect(tu_string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onData(tu_string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
XMLSocket::onXML(tu_string str)
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
  const array<with_stack_entry> with_stack;

  if (!first) {
    result->set(true);
    return;
  }
  
  log_msg("%s: nargs=%d\n", __PRETTY_FUNCTION__, nargs);
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*) this_ptr;
  assert(ptr);
  const tu_string host = env->bottom(first_arg).to_string();
  double port = env->bottom(first_arg-1).to_number();
  ret = ptr->obj.connect(host, static_cast<int>(port));

#if 0
  // Push result onto stack for onConnect
  if (ret) {
    env->push(as_value(true));
  }
  else {
    env->push(as_value(false));
  }
#endif
  env->push(as_value(true));
  if (this_ptr->get_member("onConnect", &method)) {
    //    log_msg("FIXME: Found onConnect!\n");
    as_c_function_ptr	func = method.to_c_function();
    first = false;
    //env->set_variable("success", true, 0);

    if (func) {
      // It's a C function.  Call it.
      log_msg("Calling C function for onConnect\n");
      (*func)(&val, this_ptr, env, 0, 0);
    }
    else if (as_as_function* as_func = method.to_as_function()) {
      // It's an ActionScript function.  Call it.
      log_msg("Calling ActionScript function for onConnect\n");
      (*as_func)(&val, this_ptr, env, 2, 2);
    } else {
      log_error("error in call_method(): method is not a function\n");
    }    
  } else {
    ptr->set_event_handler(event_id::SOCK_CONNECT, (as_c_function_ptr)&xmlsocket_event_connect);
  }

#if 1
  movie*	mov = env->get_target()->get_root_movie();
  Timer *timer = new Timer;
  as_c_function_ptr ondata_handler =
    (as_c_function_ptr)&xmlsocket_event_ondata;
  timer->setInterval(ondata_handler, 50, ptr, env);
  timer->setObject(ptr);
  mov->add_interval_timer(timer);
#endif

  env->pop();
  
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


void
xmlsocket_event_ondata(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  //log_msg("%s: nargs is %d\n", __PRETTY_FUNCTION__, nargs);
    
  as_value	method;
  as_value	val;
  array<tu_string> msgs;
  tu_string     tmp;
  int           i;
  
  xmlsocket_as_object*	ptr = (xmlsocket_as_object*) (as_object*)this_ptr;
  assert(ptr);
  if (ptr->obj.processingData()) {
    log_msg("Still processing data!\n");
    result->set(false);
    return;
  }
    
  if (ptr->obj.anydata(tmp)) {
    if (this_ptr->get_member("onData", &method)) {
      as_c_function_ptr	func = method.to_c_function();
      as_as_function* as_func = method.to_as_function();
      //log_msg("Got data from socket!!\n%s", data.c_str());
      log_msg("Got %d messages from XMLsocket\n", ptr->obj.messagesCount());
      for (i=0; i<ptr->obj.messagesCount(); i++) {
        tu_string data = ptr->obj[i];
        // 
        if (((data[0] != '<') && ((data[1] != 'R') || (data[1] != '?')))
            || (data[data.size()-2] != '>')){
          printf("%s: bad message %d: \"%s\"\n", __FUNCTION__, i, data.c_str());
          //printf("%s: previous message was message: \"%s\"\n", __FUNCTION__, ptr->obj[i-1]);
          //delete data;
          continue;
        }

        //log_msg("%s: Got message %s: ", data.c_str());
        env->push(as_value(data));
        if (func) {
          // It's a C function.  Call it.
          //log_msg("Calling C function for onData\n");
          (*func)(&val, this_ptr, env, 1, 0);
        } else if (as_func) {
          // It's an ActionScript function.  Call it.
          //log_msg("Calling ActionScript function for onData, processing msg %d\n", i);
          (*as_func)(&val, this_ptr, env, 1, 0);
        } else {
          log_error("error in call_method(): method is not a function\n");
        }
        env->pop();
        //printf("%s: Removing message %d: \"%s\"\n", __FUNCTION__, i, data.c_str());
        //ptr->obj.messageRemove(i);
      }
      //printf("%s: Removing messages\n", __FUNCTION__);
      ptr->obj.messagesClear();
      ptr->obj.processing(false);
      //msgs->clear();
    } else {
      log_msg("FIXME: Couldn't find onData!\n");
    }
  }

  //  env->pop();

  //result->set(&data);
  result->set(true);
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
          //log_msg("Calling C function for onConnect\n");
          (*func)(&val, this_ptr, env, 0, 0);
      }
      else if (as_as_function* as_func = method.to_as_function())
        {
          // It's an ActionScript function.  Call it.
          //log_msg("Calling ActionScript function for onConnect\n");
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

void *
main_read_thread(void *arg)
{
  int                 sockfd;

  sockfd = *(int *)arg;
  
}

} // end of gameswf namespace

// HAVE_LIBXML
#endif
