// gameswf_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.

#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_movie_def.h"
#include <float.h>

#ifdef _WIN32
#define snprintf _snprintf
#define isnan _isnan
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

	as_value::as_value(const as_value& getter, const as_value& setter)
		:
		m_property_target(NULL),
		m_type(PROPERTY)
	{
		m_property = new as_property(getter, setter);
		m_property->add_ref();
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
			if (isnan(m_number_value)) {
				m_string_value = "NaN";
			} else {
				char buffer[50];
				snprintf(buffer, 50, "%.14g", m_number_value);
				m_string_value = buffer;
			}
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
			// Moock says, "the value that results from
			// calling toString() on the object".
			//
			// The default toString() returns "[object
			// Object]" but may be customized.
			//
			// A Movieclip returns the absolute path of the object.
			//
			// call_to_string() should have checked for a
			// toString() method, before we get down here,
			// so we just handle the default cases here.

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
				// This is the default.
				m_string_value = "[object Object]";
			}
		}
		else if (m_type == C_FUNCTION)
		{
			char buffer[50];
			snprintf(buffer, 50, "<c_function 0x%p>", (void*) (m_c_function_value));
			m_string_value = buffer;
		}
		else if (m_type == AS_FUNCTION)
		{
			char buffer[50];
			snprintf(buffer, 50, "<as_function 0x%p>", (void*) (m_as_function_value));
			m_string_value = buffer;
		}
		else if (m_type == PROPERTY)
		{
			as_value val;
			get_property(&val);
			m_string_value = val.to_tu_string();
		}
		else
		{
			m_string_value = "<bad type>";
			assert(0);
		}

		return m_string_value;
	}

	const tu_string& as_value::call_to_string(as_environment* env) const
	// Handles the case of this being an object, and calls our
	// toString() method if available.
	{
		assert(env);
		
		if (m_type == OBJECT && m_object_value) {
			as_value method;
			if (m_object_value->get_member("toString", &method)) {
				as_value result = call_method0(method, env, m_object_value);
				m_string_value = result.to_tu_string();  // TODO: Should we recurse here?  Need to experiment.
				return m_string_value;
			}
		}

		// Default behavior.
//		int version = env->get_target()->get_movie_definition()->get_version();
		int version = get_current_root()->get_movie_version();
		return to_tu_string_versioned(version);
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
			as_value val;
			get_property(&val);
			return val.to_number();
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
			as_value val;
			get_property(&val);
			return val.to_bool();
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
			return m_as_function_value->m_properties.get_ptr();
		}
//		else if (m_type == PROPERTY)
//		{
//			return get_property().to_object();
//		}
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

	void	as_value::operator=(const as_value& v)
	{
		switch (v.m_type)
		{
		case UNDEFINED:
			set_undefined();
			break;
		case NULLTYPE:
			set_null();
			break;
		case BOOLEAN:
			set_bool(v.m_boolean_value);
			break;
		case STRING:
			set_tu_string(v.m_string_value);
			break;
		case NUMBER:
			set_double(v.m_number_value);
			break;
		case OBJECT:
			set_as_object_interface(v.m_object_value);
			break;
		case C_FUNCTION:
			set_as_c_function_ptr(v.m_c_function_value);
			break;
		case AS_FUNCTION:
			set_as_as_function(v.m_as_function_value);
			break;
		case PROPERTY:
			drop_refs(); 
			m_type = PROPERTY;
			m_property = v.m_property;
			m_property->add_ref();
			m_property_target = v.m_property_target;
			if (m_property_target)
			{
				m_property_target->add_ref();
			}
			break;
		default:
			assert(0);
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
		else if (m_type == OBJECT)
		{
			return m_object_value == v.to_object();
		}
		else
		{
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
		else if (m_type == PROPERTY)
		{
			if (m_property)
			{
				m_property->drop_ref();
				m_property = NULL;
				if (m_property_target)
				{
					m_property_target->drop_ref();
					m_property_target = NULL;
				}
			}
		}
	}

	void	as_value::set_property(const as_value& val)
	{
		assert(m_property);
		m_property->set(m_property_target, val);
	}

	void as_value::get_property(as_value* val) const
	{
		assert(m_property);
		m_property->get(m_property_target, val);
	}

	//
	//	as_property
	//

	as_property::as_property(const as_value& getter,	const as_value& setter)
		:
		m_getter_type(UNDEFINED),
		m_setter_type(UNDEFINED),
		m_getter(NULL),
		m_setter(NULL)
	{
		if (getter.get_type() == as_value::AS_FUNCTION && getter.to_as_function())
		{
			m_getter_type = AS_FUNCTION;
			m_getter = getter.to_as_function();
			assert(m_getter);
			m_getter->add_ref();
		}
		else
		if (getter.get_type() == as_value::C_FUNCTION && getter.to_c_function())
		{
			m_getter_type = C_FUNCTION;
			m_c_getter = getter.to_c_function();
		}

		if (setter.get_type() == as_value::AS_FUNCTION && setter.to_as_function())
		{
			m_setter_type = AS_FUNCTION;
			m_setter = setter.to_as_function();
			m_setter->add_ref();
		}
		else
		if (getter.get_type() == as_value::C_FUNCTION && setter.to_c_function())
		{
			m_setter_type = as_property::C_FUNCTION;
			m_c_setter = setter.to_c_function();
		}
	}

	as_property::~as_property()
	{
		if (m_getter_type == AS_FUNCTION && m_getter)
		{
			m_getter->drop_ref();
		}
		if (m_setter_type == as_property::AS_FUNCTION && m_setter)
		{
			m_setter->drop_ref();
		}
	}

	void	as_property::set(as_object_interface* target, const as_value& val)
	{
//		assert(target);

		as_environment env;
		env.push(val);
		if (m_setter && m_setter_type == AS_FUNCTION)
		{
			(*m_setter)(fn_call(NULL, target,	&env, 1, env.get_top_index()));
		}
		else
		if (m_c_setter && m_setter_type == as_property::C_FUNCTION)
		{
			(m_c_setter)(fn_call(NULL, target, &env, 1, env.get_top_index()));
		}
	}

	void as_property::get(as_object_interface* target, as_value* val) const
	{
//		assert(target);

		// env is used when m_getter->m_env is NULL
		as_environment env;
		if (m_getter && m_getter_type == AS_FUNCTION)
		{
			(*m_getter)(fn_call(val, target, &env, 0,	0));
		}
		else
		if (m_c_getter && m_getter_type == C_FUNCTION)
		{
			(m_c_getter)(fn_call(val, target, &env, 0,	0));
		}
	}

}
