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

BaseString::BaseString()
{
  log_msg("%s: \n", __FUNCTION__);  
}

BaseString::BaseString(const char *str)
{
  log_msg("new string \"%s\"\n", str);
  _name = str;
}

BaseString::~BaseString()
{
  _name = "";
  _value = "";
  log_msg("%s: \n", __FUNCTION__);
}

const tu_string& 
BaseString::toString(void *)
{
  log_msg("%s: \n", __FUNCTION__);

  return tu_string("");                    // FIXME: this is just to shut up G++ for now
}

String::String(const char *str) : BaseString(str)
{
  //  log_msg("%s: \n", __FUNCTION__);
}

String::~String()
{
  _name = "";
  _value = "";
  log_msg("%s: \n", __FUNCTION__);
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

tu_string
String::fromCharCode(short code)
{
  log_msg("%s: \n", __FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
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
string_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  //  log_msg("%s: args=%d\n", __FUNCTION__, nargs);
  tu_string filespec = env->bottom(first_arg).to_string();
  String *str = new String(filespec.c_str());
  //  double value = env->bottom(first_arg - 1).to_number();
  //  log_msg("%s: New string name is:  %s = %g\n", __FUNCTION__, filespec.c_str(), value);
  // log_msg("%s: New string name is:  %s\n", __FUNCTION__, filespec.c_str());
  result->set(str);	// tulrich: LEAK -- fixme -- String is not an as_object_interface*
}

} // end of gameswf namespace
