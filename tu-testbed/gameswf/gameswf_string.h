// gameswf_string.h	-- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain. Do whatever
// you want with it.

#ifndef __STRING_H__
#define __STRING_H__

// tulrich: NO <iostream> ALLOWED IN GAMESWF
//#include <iostream>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"

namespace gameswf 
{
	// @@ tulrich: rename this!  Or, just put it in string_as_object.
	class String
	{
	public:
		String(const char* str);
		String(const tu_string& str);
		~String();
		void test(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

		// tulrich: Most/all of these should take const references.
		// Some should probably return void also?
		tu_string charAt(short index);
		short charCodeAt(short index);
		tu_string concat(tu_string str1, tu_string str2);
		tu_string fromCharCode(short code);
		short indexOf(tu_string str, int short);
		short indexOf(tu_string str);
		tu_string lastIndexOf(tu_string, int index);
		tu_string lastIndexOf(tu_string);
		tu_string slice(int start);
		tu_string slice(int start, int end);

		tu_string split(tu_string str);
		tu_string split(tu_string, int limit);
		tu_string substr(int start);
		tu_string substr(int start, int end);
		tu_string substring(int start);
		tu_string substring(int start, int end);
		tu_string toLowerCase();
		tu_string toUpperCase();

		int length();
	private:
		tu_string m_value;
	};

	struct string_as_object : public gameswf::as_object
	{
          //String str;
        };
        
	struct tu_string_as_object : public gameswf::as_object
	{
          tu_string str;
	};


	void string_lastIndexOf(const fn_call& fn);
	void string_new(const fn_call& fn);
 	void string_from_char_code(const fn_call& fn);
	void string_char_code_at(const fn_call& fn);

} // end of gameswf namespace

#endif // __STRING_H__
