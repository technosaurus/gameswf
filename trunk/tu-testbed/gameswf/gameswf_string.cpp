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
	}

	String::String(const tu_string& str) : m_value(str)
	{
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
		const tu_string& initial_value = env->bottom(first_arg).to_string();
		string_as_object* str = new string_as_object(initial_value);
		result->set_as_object_interface(str);
	}

} // end of gameswf namespace
