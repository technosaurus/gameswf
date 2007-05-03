// gameswf_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf_function.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif // _WIN32

namespace gameswf
{

	bool string_to_number(double* result, const char* str)
	// Utility.  Try to convert str to a number.  If successful,
	// put the result in *result, and return true.  If not
	// successful, put 0 in *result, and return false.
	{
		char* tail = 0;
		*result = strtod(str, &tail);
		if (tail == str || *tail != 0)
		{
			// Failed conversion to Number.
			return false;
		}
		return true;
	}

	as_value::as_value(as_object_interface* obj)
		:
		m_type(OBJECT),
		m_object_value(obj)
	{
		if (m_object_value)
		{
			m_object_value->add_ref();
		}
	}


	as_value::as_value(as_as_function* func)
		:
		m_type(AS_FUNCTION),
		m_as_function_value(func)
	{
		if (m_as_function_value)
		{
			m_as_function_value->add_ref();
		}
	}

	as_value::as_value(as_as_function* getter, as_as_function* setter)
		:
		m_type(PROPERTY),
		m_getter(getter),
		m_setter(setter)
	{
		if (m_getter)
		{
			m_getter->add_ref();
		}
		if (m_setter)
		{
			m_setter->add_ref();
		}
	}

	const char*	as_value::to_string() const
	// Conversion to string.
	{
		return to_tu_string().c_str();
	}


	const tu_stringi&	as_value::to_tu_stringi() const
	{
		return reinterpret_cast<const tu_stringi&>(to_tu_string());
	}


	const tu_string&	as_value::to_tu_string() const
	// Conversion to const tu_string&.
	{
		if (m_type == STRING) { /* don't need to do anything */ }
		else if (m_type == NUMBER)
		{
			// @@ Moock says if value is a NAN, then result is "NaN"
			// INF goes to "Infinity"
			// -INF goes to "-Infinity"
			char buffer[50];
			snprintf(buffer, 50, "%.14g", m_number_value);
			m_string_value = buffer;
		}
		else if (m_type == UNDEFINED)
		{
			// Behavior depends on file version.  In
			// version 7+, it's "undefined", in versions
			// 6-, it's "".
			//
			// We'll go with the v7 behavior by default,
			// and conditionalize via _versioned()
			// functions.
			m_string_value = "undefined";
		}
		else if (m_type == NULLTYPE)
		{ 
			m_string_value = "null";
		}
		else if (m_type == BOOLEAN)
		{
			m_string_value = this->m_boolean_value ? "true" : "false";
		}
		else if (m_type == OBJECT)
		{
			// @@ Moock says, "the value that results from
			// calling toString() on the object".
			//
			// The default toString() returns "[object
			// Object]" but may be customized.
			//
			// A Movieclip returns the absolute path of the object.

			const char*	val = NULL;
			if (m_object_value)
			{
				val = m_object_value->get_text_value();
			}

			if (val)
			{
				m_string_value = val;
			}
			else
			{
				// Do we have a "toString" method?
				//
				// TODO: we need an environment in order to call toString()!

				// This is the default.
				m_string_value = "[object Object]";
			}
		}
		else if (m_type == C_FUNCTION)
		{
			char buffer[50];
			snprintf(buffer, 50, "<c_function 0x%X>", (void*) (m_c_function_value));
			m_string_value = buffer;
		}
		else if (m_type == AS_FUNCTION)
		{
			char buffer[50];
			snprintf(buffer, 50, "<as_function 0x%X>", (void*) (m_as_function_value));
			m_string_value = buffer;
		}
		else if (m_type == PROPERTY)
		{
			m_string_value = get_property().to_tu_string();
		}
		else
		{
			m_string_value = "<bad type>";
			assert(0);
		}
		
		return m_string_value;
	}


	const tu_string&	as_value::to_tu_string_versioned(int version) const
	// Conversion to const tu_string&.
	{
		if (m_type == UNDEFINED)
		{
			// Version-dependent behavior.
			if (version <= 6)
			{
				m_string_value = "";
			}
			else
			{
				m_string_value = "undefined";
			}
			return m_string_value;
		}
		
		return to_tu_string();
	}

	double	as_value::to_number() const
	// Conversion to double.
	{
		if (m_type == STRING)
		{
			// @@ Moock says the rule here is: if the
			// string is a valid float literal, then it
			// gets converted; otherwise it is set to NaN.
			//
			// Also, "Infinity", "-Infinity", and "NaN"
			// are recognized.
			if (! string_to_number(&m_number_value, m_string_value.c_str()))
			{
				// Failed conversion to Number.
				m_number_value = 0.0;	// TODO should be NaN
			}
			return m_number_value;
		}
		else if (m_type == NULLTYPE)
		{
 			// Evan: from my tests
			return 0;
		}
		else if (m_type == BOOLEAN)
		{
			// Evan: from my tests
			return (this->m_boolean_value) ? 1 : 0;
		}
		else if (m_type == NUMBER)
		{
			return m_number_value;
		}
		else if (m_type == OBJECT && m_object_value != NULL)
		{
			// @@ Moock says the result here should be
			// "the return value of the object's valueOf()
			// method".
			//
			// Arrays and Movieclips should return NaN.

			// Text characters with var names could get in
			// here.
			const char* textval = m_object_value->get_text_value();
			if (textval)
			{
				return atof(textval);
			}

			return 0.0;
		}
		else if (m_type == PROPERTY)
		{
			return get_property().to_number();
		}
		else
		{
			return 0.0;
		}
	}


	bool	as_value::to_bool() const
	// Conversion to boolean.
	{
		// From Moock
		if (m_type == STRING)
		{
			if (m_string_value == "false")
			{
				return false;
			}
			else if (m_string_value == "true")
			{
				return true;
			}
			else
			{
				// @@ Moock: "true if the string can
				// be converted to a valid nonzero
				// number".
				//
				// Empty string --> false
				return to_number() != 0.0;
			}
		}
		else if (m_type == NUMBER)
		{
			// @@ Moock says, NaN --> false
			return m_number_value != 0.0;
		}
		else if (m_type == BOOLEAN)
		{
			return this->m_boolean_value;
		}
		else if (m_type == OBJECT)
		{
			return m_object_value != NULL;
		}
		else if (m_type == C_FUNCTION)
		{
			return m_c_function_value != NULL;
		}
		else if (m_type == AS_FUNCTION)
		{
			return m_as_function_value != NULL;
		}
		else if (m_type == PROPERTY)
		{
			return get_property().to_bool();
		}
		else
		{
			assert(m_type == UNDEFINED || m_type == NULLTYPE);
			return false;
		}
	}

	
	as_object_interface*	as_value::to_object() const
	// Return value as an object.
	{
		if (m_type == OBJECT)
		{
			// OK.
			return m_object_value;
		}
		else if (m_type == AS_FUNCTION && m_as_function_value != NULL)
		{
			// Make sure this as_function has properties &
			// a prototype object attached to it, since those
			// may be about to be referenced.
			m_as_function_value->lazy_create_properties();
			assert(m_as_function_value->m_properties);

			return m_as_function_value->m_properties;
		}
		else
		{
			return NULL;
		}
	}


	as_c_function_ptr	as_value::to_c_function() const
	// Return value as a C function ptr.  Returns NULL if value is
	// not a C function.
	{
		if (m_type == C_FUNCTION)
		{
			// OK.
			return m_c_function_value;
		}
		else
		{
			return NULL;
		}
	}

	as_as_function*	as_value::to_as_function() const
	// Return value as an ActionScript function.  Returns NULL if value is
	// not an ActionScript function.
	{
		if (m_type == AS_FUNCTION)
		{
			// OK.
			return m_as_function_value;
		}
		else
		{
			return NULL;
		}
	}


	void	as_value::convert_to_number()
	// Force type to number.
	{
		set_double(to_number());
	}


	void	as_value::convert_to_string()
	// Force type to string.
	{
		drop_refs();
		to_tu_string();	// init our string data.
		m_type = STRING;	// force type.
	}


	// 
	void	as_value::convert_to_string_versioned(int version)
	// Force type to string.
	{
		drop_refs();
		to_tu_string_versioned(version);	// init our string data.
		m_type = STRING;	// force type.
	}


	void	as_value::set_as_object_interface(as_object_interface* obj)
	{
		if (m_type != OBJECT || m_object_value != obj)
		{
			drop_refs();
			m_type = OBJECT;
			m_object_value = obj;
			if (m_object_value)
			{
				m_object_value->add_ref();
			}
		}
	}

	void	as_value::set_as_as_function(as_as_function* func)
	{
		if (m_type != AS_FUNCTION || m_as_function_value != func)
		{
			drop_refs();
			m_type = AS_FUNCTION;
			m_as_function_value = func;
			if (m_as_function_value)
			{
				m_as_function_value->add_ref();
			}
		}
	}


	bool	as_value::operator==(const as_value& v) const
	// Return true if operands are equal.
	{
		bool this_nulltype = (m_type == UNDEFINED || m_type == NULLTYPE);
		bool v_nulltype = (v.get_type() == UNDEFINED || v.get_type() == NULLTYPE);
		if (this_nulltype || v_nulltype)
		{
			return this_nulltype == v_nulltype;
		}
		else if (m_type == STRING)
		{
			return m_string_value == v.to_tu_string();
		}
		else if (m_type == NUMBER)
		{
			return m_number_value == v.to_number();
		}
		else if (m_type == BOOLEAN)
		{
			return m_boolean_value == v.to_bool();
		}
		else
		{
			// Evan: what about objects???
			// TODO
			return m_type == v.m_type;
		}
	}

	
	bool	as_value::operator!=(const as_value& v) const
	// Return true if operands are not equal.
	{
		return ! (*this == v);
	}

	
	void	as_value::string_concat(const tu_string& str)
	// Sets *this to this string plus the given string.
	{
		drop_refs();
		to_tu_string();	// make sure our m_string_value is initialized
		m_type = STRING;
		m_string_value += str;
	}

	void	as_value::drop_refs()
	// Drop any ref counts we have; this happens prior to changing our value.
	{
		if (m_type == AS_FUNCTION)
		{
			if (m_as_function_value)
			{
				m_as_function_value->drop_ref();
				m_as_function_value = 0;
			}
		}
		else if (m_type == OBJECT)
		{
			if (m_object_value)
			{
				m_object_value->drop_ref();
				m_object_value = 0;
			}
		}
	}

	void	as_value::set_property(const as_value& v)
	{
		m_setter->m_env->push(v);
		(*m_setter)(fn_call(NULL, NULL, m_setter->m_env, 1, m_setter->m_env->get_top_index()));
		m_setter->m_env->drop(1);
	}

	as_value as_value::get_property() const
	{
		as_value val;
		(*m_getter)(fn_call(&val, NULL, m_getter->m_env, 0, m_getter->m_env->get_top_index()));
		return val;
	}

}
