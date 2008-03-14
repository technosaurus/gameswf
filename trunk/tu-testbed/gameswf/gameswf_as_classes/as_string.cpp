// as_string.cpp      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation of ActionScript String class.

#include "gameswf/gameswf_as_classes/as_string.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_as_classes/as_array.h"
#include "base/utf8.h"

namespace gameswf
{
	// An as_object that is just a thin wrapper around a string
	// pointer.
	struct tu_string_as_object : public gameswf::as_object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_STRING };
		virtual bool is(int class_id)
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		const tu_string& m_string;
		tu_string_as_object(const tu_string& str)	:
			m_string(str)
		{
		}

		virtual bool get_member(const tu_stringi& name, as_value* val)
		// Overload this, to catch references to "length".
		{
			if (name == "length") {
				val->set_int(m_string.length());
				return true;
			}
			return as_object::get_member(name, val);
		}
	};

	// Subclass that holds an actual string value.  This is the
	// implementation of the ActionScript String class.
	struct tu_string_as_object_instance : public tu_string_as_object {
		tu_string_as_object_instance()
			: tu_string_as_object(m_string_value) {
		}
		
		void set_value(const tu_string& str) {
			m_string_value = str;
		}

	private:
		tu_string m_string_value;
	};


	void string_method(const fn_call& fn, const tu_stringi& method_name, const tu_string& this_string)
	// Executes the string method named by method_name.
	{
		// We keep a static string instance to hold a table of
		// string methods.
		// TODO: s_static_string_instance.drop_ref() on exit from gameswf
		static smart_ptr<as_object> s_static_string_instance;

		if (!s_static_string_instance.get_ptr()) {
			as_value result;
			string_ctor(fn_call(&result, NULL, fn.env, 0, 0));
			s_static_string_instance = result.to_object();
		}
		assert(s_static_string_instance.get_ptr());

		as_value method;
		if (s_static_string_instance->get_member(method_name, &method)) {
			tu_string_as_object tsao(this_string);
			*(fn.result) = call_method(method, fn.env, &tsao, fn.nargs, fn.first_arg_bottom_index);
		} else {
			log_error("error: string_method can't find method %s\n", method_name.c_str());
		}
	}

	void string_char_code_at(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);

		int	index = (int) fn.arg(0).to_number();
		if (index >= 0 && index < this_ptr->m_string.utf8_length()) {
			fn.result->set_double(this_ptr->m_string.utf8_char_at(index));
			return;
		}

		fn.result->set_nan();
	}
  
	void string_concat(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);

		tu_string result(this_ptr->m_string);
		for (int i = 0; i < fn.nargs; i++) {
			result += fn.arg(i).call_to_string(fn.env);
		}

		fn.result->set_tu_string(result);
	}
  
	void string_from_char_code(const fn_call& fn)
	{
		// Takes a variable number of args.  Each arg
		// is a numeric character code.  Construct the
		// string from the character codes.
		tu_string result;
		for (int i = 0; i < fn.nargs; i++) {
			uint32 c = (uint32) fn.arg(i).to_number();
			result.append_wide_char(c);
		}

		fn.result->set_tu_string(result);
	}

	void string_index_of(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);

		if (fn.nargs < 1)
		{
			fn.result->set_double(-1);
		} else {
			int	start_index = 0;
			if (fn.nargs > 1) {
				start_index = (int) fn.arg(1).to_number();
			}
			const char*	str = this_ptr->m_string.c_str();
			const char*	p = strstr(
				str + start_index,	// FIXME: not UTF-8 correct!
				fn.arg(0).to_string());
			if (p == NULL) {
				fn.result->set_double(-1);
				return;
			}

			fn.result->set_double(tu_string::utf8_char_count(str, p - str));
		}
	}


	void string_last_index_of(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);

		if (fn.nargs < 1)
		{
			fn.result->set_double(-1);
		} else {
			int	start_index = 0;
			if (fn.nargs > 1) {
				start_index = (int) fn.arg(1).to_number();
			}
			const char* str = this_ptr->m_string.c_str();
			const char* last_hit = NULL;
			const char* haystack = str + start_index;	// FIXME: not UTF-8 correct!
			for (;;) {
				const char*	p = strstr(haystack, fn.arg(0).to_string());
				if (p == NULL) {
					break;
				}
				last_hit = p;
				haystack = p + 1;
			}
			if (last_hit == NULL) {
				fn.result->set_double(-1);
			} else {
				fn.result->set_double(tu_string::utf8_char_count(str, last_hit - str));
			}
		}
	}
	
	void string_slice(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		int len = this_str.utf8_length();
		int start = 0;
		if (fn.nargs >= 1) {
			start = (int) fn.arg(0).to_number();
			if (start < 0) {
				start = len + start;
			}
		}
		int end = len;
		if (fn.nargs >= 2) {
			end = (int) fn.arg(1).to_number();
			if (end < 0) {
				end = len + end;
			}
		}

		start = iclamp(start, 0, len);
		end = iclamp(end, start, len);

		fn.result->set_tu_string(this_str.utf8_substring(start, end));
	}

	void string_split(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		smart_ptr<as_array> arr(new as_array);

		const tu_string* delimiter = NULL;
		int delimiter_len = 0;
		if (fn.nargs >= 1) {
			delimiter = &(fn.arg(0).call_to_string(fn.env));
			delimiter_len = delimiter->length();
		}
		int max_count = this_str.utf8_length();
		if (fn.nargs >= 2) {
			max_count = (int) fn.arg(1).to_number();
		}

		const char* p = this_str.c_str();
		const char* word_start = p;
		for (int i = 0; i < max_count; ) {
			const char* n = p;
			if (delimiter_len == 0) {
				utf8::decode_next_unicode_character(&n);
				if (n == p) {
					break;
				}
				tu_string word(p, n - p);
				as_value val;
				as_value index(i);
				val.set_tu_string(word);
				arr->set_member(index.to_tu_string(), val);
				p = n;
				i++;
			} else {
				bool match = strncmp(p, delimiter->c_str(), delimiter_len) == 0;
				if (*p == 0 || match) {
					// Emit the previous word.
					tu_string word(word_start, p - word_start);
					as_value val;
					as_value index(i);
					val.set_tu_string(word);
					arr->set_member(index.to_tu_string(), val);
					i++;

					if (match) {
						// Skip the delimiter.
						p += delimiter_len;
						word_start = p;
					}

					if (*p == 0) {
						break;
					}
				} else {
					utf8::decode_next_unicode_character(&p);
				}
			}
		}

		fn.result->set_as_object(arr.get_ptr());
	}

	// public substr(start:Number, length:Number) : String
	void string_substr(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		if (fn.nargs < 1)
		{
			return;
		}

		// Pull a slice out of this_string.
		int	utf8_len = this_str.utf8_length();
		int	len = utf8_len;

		int start = (int) fn.arg(0).to_number();
		start = iclamp(start, 0, utf8_len);

		if (fn.nargs >= 2)
		{
			len = (int) fn.arg(1).to_number();
			len = iclamp(len, 0, utf8_len);
		}

		int end = start + len;
		if (end > utf8_len)
		{
			end = utf8_len;
		}

		if (start < end)
		{
			fn.result->set_tu_string(this_str.utf8_substring(start, end));
		}
	}

	void string_substring(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		// Pull a slice out of this_string.
		int	start = 0;
		int	utf8_len = this_str.utf8_length();
		int	end = utf8_len;
		if (fn.nargs >= 1)
		{
			start = (int) fn.arg(0).to_number();
			start = iclamp(start, 0, utf8_len);
		}
		if (fn.nargs >= 2)
		{
			end = (int) fn.arg(1).to_number();
			end = iclamp(end, 0, utf8_len);
		}

		if (end < start) swap(&start, &end);	// dumb, but that's what the docs say
		assert(end >= start);

		fn.result->set_tu_string(this_str.utf8_substring(start, end));
	}

	void string_to_lowercase(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		fn.result->set_tu_string(this_str.utf8_to_lower());
	}
	
	void string_to_uppercase(const fn_call& fn) 
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		fn.result->set_tu_string(this_str.utf8_to_upper());
	}

	void string_char_at(const fn_call& fn)
	{
		const tu_string_as_object* this_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_ptr);
		const tu_string& this_str = this_ptr->m_string;

		int	index = (int) fn.arg(0).to_number();
		if (index >= 0 && index < this_ptr->m_string.utf8_length()) 
		{
			char c[2];
			c[0] = this_str.utf8_char_at(index);
			c[1] = 0;
			fn.result->set_tu_string(c);
		}
	}
	

	void string_to_string(const fn_call& fn)
	{
		tu_string_as_object* this_string_ptr = cast_to<tu_string_as_object>(fn.this_ptr);
		assert(this_string_ptr);

		fn.result->set_tu_string(this_string_ptr->m_string);
	}

	void string_ctor(const fn_call& fn)
	{
		smart_ptr<tu_string_as_object_instance> str = new tu_string_as_object_instance;

		if (fn.nargs > 0)
		{
			str->set_value(fn.arg(0).call_to_string(fn.env));
		}
		
		// TODO fill in the rest
		str->set_member("toString", string_to_string);
		str->set_member("fromCharCode", string_from_char_code);
		str->set_member("charCodeAt", string_char_code_at);
		str->set_member("concat", string_concat);
		str->set_member("indexOf", string_index_of);
		str->set_member("lastIndexOf", string_last_index_of);
		str->set_member("slice", string_slice);
		str->set_member("split", string_split);
		str->set_member("substring", string_substring);
		str->set_member("substr", string_substr);
		str->set_member("toLowerCase", string_to_lowercase);
		str->set_member("toUpperCase", string_to_uppercase);
		str->set_member("charAt", string_char_at);
    
		fn.result->set_as_object(str.get_ptr());
	}

} // namespace gameswf
