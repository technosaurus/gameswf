// gameswf_xml.h			-- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain. Do whatever
// you want with it.

#ifndef __STRING_H__
#define __STRING_H__

#include <iostream>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

namespace gameswf 
{
  
class BaseString
{
public:
	BaseString();
	BaseString(const char *name);
	~BaseString();
	
	std::string toString(void *);
 protected:
	std::string _value;
	std::string _name;
};
	
class String: public BaseString
{
public:
	String(const char *str);
	~String();
	void test(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

	std::string charAt(short index);
	short charCodeAt(short index);
	std::string concat(std::string str1, std::string str2);
	std::string fromCharCode(short code);
	short indexOf(std::string str, int short);
	short indexOf(std::string str);
	std::string lastIndexOf(std::string, int index);
	std::string lastIndexOf(std::string);
	std::string slice(int start);
	std::string slice(int start, int end);

	std::string split(std::string str);
	std::string split(std::string, int limit);
	std::string substr(int start);
	std::string substr(int start, int end);
	std::string substring(int start);
	std::string substring(int start, int end);
	std::string toLowerCase();
	std::string toUpperCase();

	int length();
};

struct string_as_object : public gameswf::as_object
{
	String str;
};


void
string_lastIndexOf(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);


void
string_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);
 
} // end of gameswf namespace

// __STRING_H__
#endif
