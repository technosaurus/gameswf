// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf_string.h"
#include "gameswf_log.h"

using namespace std;


//#ifndef __FUNCTION__
//#define __FUNCTION__ "__FUNCTION__"
//#endif


namespace gameswf
{  
  String::String(const char* str) : m_value(str)
  {
          //log_msg("%s: \n", __FUNCTION__);
  }
  
  String::String(const tu_string& str) : m_value(str)
  {
    //log_msg("%s: \n", __FUNCTION__);
  }
  
  String::~String()
  {
  }
  
  tu_string
  String::charAt(short index)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  short
  String::charCodeAt(short index)
  {
    log_msg("%s: \n", __FUNCTION__);
    return -1;                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::concat(tu_string str1, tu_string str2)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }

  // Return a string made up of the values of ASCII characters
  tu_string
  String::fromCharCode(short code)
  {
    log_msg("%s: \n", __FUNCTION__);
    m_value = "foobar!";
    return m_value;                    // FIXME: this is just to shut up G++ for now
  }
  
  short
  String::indexOf(tu_string str, int short)
  {
    log_msg("%s: \n", __FUNCTION__);
    return -1;                    // FIXME: this is just to shut up G++ for now
  }
  
  short
  String::indexOf(tu_string str)
  {
    log_msg("%s: \n", __FUNCTION__);
    return -1;                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::lastIndexOf(tu_string, int index)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::lastIndexOf(tu_string)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::slice(int start) 
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::slice(int start, int end)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::split(tu_string str)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::split(tu_string, int limit)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::substr(int start)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::substr(int start, int end)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::substring(int start)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::substring(int start, int end)
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string 
  String::toLowerCase()
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  tu_string
  String::toUpperCase()
  {
    log_msg("%s: \n", __FUNCTION__);
    return "";                    // FIXME: this is just to shut up G++ for now
  }
  
  int
  String::length()
  {
    log_msg("%s: \n", __FUNCTION__);
    return -1;                    // FIXME: this is just to shut up G++ for now
  }
  
  void
  String::test(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    log_msg("%s: args=%d\n", __FUNCTION__, nargs);
  }
  
  
  
  void
  string_lastIndexOf(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    tu_string filespec = env->bottom(first_arg).to_string();
    log_msg("%s: args=%d\n", __FUNCTION__, nargs);
    //    xsockobj.connect(filespec, 8000);
  }
  
  void
  string_from_char_code(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    int i, ch;
    tu_string str;
    
    //log_msg("FIXME: %s: args=%d\n", __FUNCTION__, nargs);

    //tu_string filespec = env->bottom(first_arg).to_string();
    //int ch = env->bottom(first_arg).to_number();

    for (i=0; i<nargs; i++) {
      ch = env->bottom(first_arg).to_number();
      //log_msg("FIXME: char %d is %c\n", ch, ch);
      str += ch;
    }

    result->set(str);
  }

  void
  string_char_code_at(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    log_msg("FIXME: %s: args=%d\n", __FUNCTION__, nargs);
  }

  void
  string_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    as_value method;
    
    //log_msg("%s:\n", __FUNCTION__);

    tu_string_as_object* str = new tu_string_as_object;
    //log_msg("New String object at %p\n", str);

    //env->set_variable("String", str, 0);
    //env->set_member("String", as_value(string_new));
    str->set_member("fromCharCode", &string_from_char_code);
    str->set_member("charCodeAt", &string_char_code_at);
    
    result->set_as_object_interface(str);
  }
  
} // end of gameswf namespace
