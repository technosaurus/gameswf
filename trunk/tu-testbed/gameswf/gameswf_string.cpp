#include <string>
#include "gameswf_string.h"
#include "gameswf_log.h"
using namespace gameswf;
using namespace std;

BaseString::BaseString()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);  
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
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

std::string
BaseString::toString(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);

  return "";                    // FIXME: this is just to shut up G++ for now
}

String::String(const char *str) : BaseString(str)
{
  //  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

String::~String()
{
  _name = "";
  _value = "";
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

std::string
String::charAt(short index)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

short
String::charCodeAt(short index)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return -1;                    // FIXME: this is just to shut up G++ for now
}

std::string
String::concat(std::string str1, std::string str2)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::fromCharCode(short code)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

short
String::indexOf(std::string str, int short)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return -1;                    // FIXME: this is just to shut up G++ for now
}

short
String::indexOf(std::string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return -1;                    // FIXME: this is just to shut up G++ for now
}

std::string
String::lastIndexOf(std::string, int index)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::lastIndexOf(std::string)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::slice(int start) 
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::slice(int start, int end)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::split(std::string str)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::split(std::string, int limit)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::substr(int start)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::substr(int start, int end)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::substring(int start)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::substring(int start, int end)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string 
String::toLowerCase()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

std::string
String::toUpperCase()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return "";                    // FIXME: this is just to shut up G++ for now
}

int
String::length()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return -1;                    // FIXME: this is just to shut up G++ for now
}

void
String::test(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
}



void
string_lastIndexOf(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    tu_string filespec = env->bottom(first_arg).to_string();
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
    //    xsockobj.connect(filespec, 8000);
}

void
string_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  //  log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
  tu_string filespec = env->bottom(first_arg).to_string();
  String *str = new String(filespec.c_str());
  //  double value = env->bottom(first_arg - 1).to_number();
  //  log_msg("%s: New string name is:  %s = %g\n", __PRETTY_FUNCTION__, filespec.c_str(), value);
  // log_msg("%s: New string name is:  %s\n", __PRETTY_FUNCTION__, filespec.c_str());
  result->set(str);
}
