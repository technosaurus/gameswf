// gameswf_value.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.


#ifndef GAMESWF_VALUE_H
#define GAMESWF_VALUE_H

#include "base/container.h"
#include "base/smart_ptr.h"
#include <wchar.h>

namespace gameswf
{
	struct fn_call;
	struct as_as_function;
	struct as_object_interface;
	struct as_environment;

	typedef void (*as_c_function_ptr)(const fn_call& fn);

	bool string_to_number(double* result, const char* str);

	struct as_value
	{
		enum type
		{
			UNDEFINED,
			NULLTYPE,
			BOOLEAN,
			STRING,
			NUMBER,
			OBJECT,
			C_FUNCTION,
			AS_FUNCTION,	// ActionScript function.
			PROPERTY
		};
		type	m_type;
		mutable tu_string	m_string_value;
		union
		{
			bool m_boolean_value;
			// @@ hm, what about PS2, where double is bad?	should maybe have int&float types.
			mutable	double	m_number_value;
			as_object_interface*	m_object_value;
			as_c_function_ptr	m_c_function_value;
			as_as_function*	m_as_function_value;
			struct
			{
				as_object_interface*	m_target;
				as_as_function*	m_getter;
				as_as_function*	m_setter;
			};
		};

		as_value()
			:
			m_type(UNDEFINED),
			m_number_value(0.0)
		{
		}

		as_value(const as_value& v)
			:
			m_type(UNDEFINED),
			m_number_value(0.0)
		{
			*this = v;
		}

		as_value(const char* str)
			:
			m_type(STRING),
			m_string_value(str),
			m_number_value(0.0)
		{
		}

		as_value(const wchar_t* wstr)
			:
			m_type(STRING),
			m_string_value(""),
			m_number_value(0.0)
		{
			// Encode the string value as UTF-8.
			//
			// Is this dumb?  Alternatives:
			//
			// 1. store a tu_wstring instead of tu_string?
			// Bloats typical ASCII strings, needs a
			// tu_wstring type, and conversion back the
			// other way to interface with char[].
			// 
			// 2. store a tu_wstring as a union with
			// tu_string?  Extra complexity.
			//
			// 3. ??
			//
			// Storing UTF-8 seems like a pretty decent
			// way to do it.  Everything else just
			// continues to work.

#if (WCHAR_MAX != MAXINT)
			tu_string::encode_utf8_from_wchar(&m_string_value, (const uint16 *)wstr);
#else
# if (WCHAR_MAX != MAXSHORT)
# error "Can't determine the size of wchar_t"
# else
			tu_string::encode_utf8_from_wchar(&m_string_value, (const uint32 *)wstr);
# endif
#endif
		}

		as_value(bool val)
			:
			m_type(BOOLEAN),
			m_boolean_value(val)
		{
		}

		as_value(int val)
			:
			m_type(NUMBER),
			m_number_value(double(val))
		{
		}

		as_value(float val)
			:
			m_type(NUMBER),
			m_number_value(double(val))
		{
		}

		as_value(double val)
			:
			m_type(NUMBER),
			m_number_value(val)
		{
		}

		as_value(as_object_interface* obj);

		as_value(as_c_function_ptr func) :
			m_type(C_FUNCTION),
			m_c_function_value(func)
		{
			m_c_function_value = func;
		}

		as_value(as_as_function* func);

		as_value(as_object_interface*	target,	as_as_function* getter, as_as_function* setter);

		~as_value() { drop_refs(); }

		// Useful when changing types/values.
		exported_module void	drop_refs();

		exported_module type	get_type() const { return m_type; }

		// Return true if this value is callable.
		exported_module bool is_function() const
		{
			return m_type == C_FUNCTION || m_type == AS_FUNCTION;
		}

		exported_module const char*	to_string() const;
		exported_module const tu_string&	to_tu_string() const;
		exported_module const tu_string&	to_tu_string_versioned(int version) const;
		exported_module const tu_stringi&	to_tu_stringi() const;
		exported_module double	to_number() const;
		exported_module bool	to_bool() const;
		exported_module as_object_interface*	to_object() const;
		exported_module as_c_function_ptr	to_c_function() const;
		exported_module as_as_function*	to_as_function() const;
		exported_module const tu_string& call_to_string(as_environment* env) const;

		exported_module void	convert_to_number();
		exported_module void	convert_to_string();
		exported_module void	convert_to_string_versioned(int version);

		// These set_*()'s are more type-safe; should be used
		// in preference to generic overloaded set().  You are
		// more likely to get a warning/error if misused.
		exported_module void	set_tu_string(const tu_string& str) { drop_refs(); m_type = STRING; m_string_value = str; }
		exported_module void	set_string(const char* str) { drop_refs(); m_type = STRING; m_string_value = str; }
		exported_module void	set_double(double val) { drop_refs(); m_type = NUMBER; m_number_value = val; }
		exported_module void	set_bool(bool val) { drop_refs(); m_type = BOOLEAN; m_boolean_value = val; }
		exported_module void	set_as_property(as_object_interface* target, as_as_function* getter, as_as_function* setter);
		exported_module void	set_int(int val) { set_double(val); }
		exported_module void	set_as_object_interface(as_object_interface* obj);
		exported_module void	set_as_c_function_ptr(as_c_function_ptr func)
		{
			drop_refs(); m_type = C_FUNCTION; m_c_function_value = func;
		}
		exported_module void	set_as_as_function(as_as_function* func);
		exported_module void	set_undefined() { drop_refs(); m_type = UNDEFINED; }
		exported_module void	set_null() { drop_refs(); m_type = NULLTYPE; }

		void	set_property(const as_value& v);
		as_value get_property() const;

		exported_module void	operator=(const as_value& v)
		{
			if (v.m_type == UNDEFINED) set_undefined();
			else if (v.m_type == NULLTYPE) set_null();
			else if (v.m_type == BOOLEAN) set_bool(v.m_boolean_value);
			else if (v.m_type == STRING) set_tu_string(v.m_string_value);
			else if (v.m_type == NUMBER) set_double(v.m_number_value);
			else if (v.m_type == OBJECT) set_as_object_interface(v.m_object_value);
			else if (v.m_type == C_FUNCTION) set_as_c_function_ptr(v.m_c_function_value);
			else if (v.m_type == AS_FUNCTION) set_as_as_function(v.m_as_function_value);
			else if (v.m_type == PROPERTY) set_as_property(v.m_target, v.m_getter, v.m_setter);
			else assert(0);
		}

		exported_module bool	operator==(const as_value& v) const;
		exported_module bool	operator!=(const as_value& v) const;
		exported_module bool	operator<(const as_value& v) const { return to_number() < v.to_number(); }
		exported_module void	operator+=(const as_value& v) { set_double(this->to_number() + v.to_number()); }
		exported_module void	operator-=(const as_value& v) { set_double(this->to_number() - v.to_number()); }
		exported_module void	operator*=(const as_value& v) { set_double(this->to_number() * v.to_number()); }
		exported_module void	operator/=(const as_value& v) { set_double(this->to_number() / v.to_number()); }  // @@ check for div/0
		exported_module void	operator&=(const as_value& v) { set_int(int(this->to_number()) & int(v.to_number())); }
		exported_module void	operator|=(const as_value& v) { set_int(int(this->to_number()) | int(v.to_number())); }
		exported_module void	operator^=(const as_value& v) { set_int(int(this->to_number()) ^ int(v.to_number())); }
		exported_module void	shl(const as_value& v) { set_int(int(this->to_number()) << int(v.to_number())); }
		exported_module void	asr(const as_value& v) { set_int(int(this->to_number()) >> int(v.to_number())); }
		exported_module void	lsr(const as_value& v) { set_int((Uint32(this->to_number()) >> int(v.to_number()))); }

		exported_module void	string_concat(const tu_string& str);

		exported_module tu_string* get_mutable_tu_string() { assert(m_type == STRING); return &m_string_value; }
	};


	//
	// as_prop_flags
	//
	// flags defining the level of protection of a member
	struct as_prop_flags
	{
		enum flag
		{
			DONT_ENUM = 0x01,
			DONT_DELETE = 0x02,
			READ_ONLY = 0x04
		};

		// Numeric flags
		int m_flags;
		// if true, this value is protected (internal to gameswf)
		bool m_is_protected;

		// mask for flags
		static const int as_prop_flags_mask = DONT_ENUM | DONT_DELETE | READ_ONLY;

		// Default constructor
		as_prop_flags() : m_flags(0), m_is_protected(false)
		{
		}

		// Constructor, from numerical value
		as_prop_flags(int flags) : m_flags(flags), m_is_protected(false)
		{
		}

		// accessor to m_readOnly
		bool get_read_only() const { return (((this->m_flags & READ_ONLY) != 0) ? true : false); }

		// accessor to m_dontDelete
		bool get_dont_delete() const { return (((this->m_flags & DONT_DELETE) != 0) ? true : false); }

		// accessor to m_dontEnum
		bool get_dont_enum() const { return (((this->m_flags & DONT_ENUM) != 0) ? true : false);	}

		// accesor to the numerical flags value
		int get_flags() const { return this->m_flags; }

		// accessor to m_is_protected
		bool get_is_protected() const { return this->m_is_protected; }
		// setter to m_is_protected
		void set_get_is_protected(const bool is_protected) { this->m_is_protected = is_protected; }

		// set the numerical flags value (return the new value )
		// If unlocked is false, you cannot un-protect from over-write,
		// you cannot un-protect from deletion and you cannot
		// un-hide from the for..in loop construct
		int set_flags(const int setTrue, const int set_false = 0)
		{
			if (!this->get_is_protected())
			{
				this->m_flags = this->m_flags & (~set_false);
				this->m_flags |= setTrue;
			}

			return get_flags();
		}
	};

	//
	// as_member
	//
	// member for as_object: value + flags
	struct as_member {
		// value
		as_value m_value;
		// Properties flags
		as_prop_flags m_flags;

		// Default constructor
		as_member() {
		}

		// Constructor
		as_member(const as_value &value,const as_prop_flags flags=as_prop_flags())
			:
			m_value(value),
			m_flags(flags)
		{
		}

		// accessor to the value
		const as_value& get_member_value() const { return m_value; }
		// accessor to the properties flags
		const as_prop_flags& get_member_flags() const { return m_flags; }

		// set the value
		void set_member_value(const as_value &value)  { m_value = value; }
		// accessor to the properties flags
		void set_member_flags(const as_prop_flags &flags)  { m_flags = flags; }
	};

}


#endif
