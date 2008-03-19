// gameswf_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.

#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_as_classes/as_number.h"
#include "gameswf/gameswf_as_classes/as_boolean.h"
#include <float.h>

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

	// utility
	double get_nan()
	{
		double zero = 0.0;
		return zero / zero;
	}

	as_value::as_value(as_object* obj) :
		m_type(OBJECT),
		m_object_value(obj)
	{
		if (m_object_value)
		{
			m_object_value->add_ref();
		}
	}


	as_value::as_value(as_s_function* func)	:
		m_type(UNDEFINED)
	{
		set_as_object(func);
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
		switch (m_type)
		{
			case STRING:
				// don't need to do anything
				break;

			case UNDEFINED:
				// Behavior depends on file version.  In
				// version 7+, it's "undefined", in versions
				// 6-, it's "".
				//
				// We'll go with the v7 behavior by default,
				// and conditionalize via _versioned()
				// functions.
				m_string_value = "undefined";
				break;

			case NULLTYPE:
				m_string_value = "null";
				break;

			case BOOLEAN:
				m_string_value = m_bool ? "true" : "false";
				break;

			case NUMBER:
				// @@ Moock says if value is a NAN, then result is "NaN"
				// INF goes to "Infinity"
				// -INF goes to "-Infinity"
				if (isnan(m_number))
				{
					m_string_value = "NaN";
				} 
				else
				{
					char buffer[50];
					snprintf(buffer, 50, "%.14g", m_number);
					m_string_value = buffer;
				}
				break;

			case OBJECT:
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

				m_string_value = m_object_value->to_string();
				break;
	
			case PROPERTY:
			{
				as_value val;
				get_property(&val);
				m_string_value = val.to_tu_string();
				break;
			}

			default:
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
				as_value result = call_method(method, env, m_object_value, 0, env->get_top_index());
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
		switch (m_type)
		{
			case STRING:
			{
				// @@ Moock says the rule here is: if the
				// string is a valid float literal, then it
				// gets converted; otherwise it is set to NaN.
				//
				// Also, "Infinity", "-Infinity", and "NaN"
				// are recognized.
				double val;
				if (! string_to_number(&val, m_string_value.c_str()))
				{
					// Failed conversion to Number.
					val = 0.0;	// TODO should be NaN
				}
				return val;
			}

			case NULLTYPE:
	 			// Evan: from my tests
				return 0;

			case NUMBER:
				return m_number;

			case BOOLEAN:
				return m_bool ? 1 : 0;

			case OBJECT:
				return m_object_value->to_number();
	
			case PROPERTY:
			{
				as_value val;
				get_property(&val);
				return val.to_number();
			}

			default:
				return 0.0;
		}
	}


	bool	as_value::to_bool() const
	// Conversion to boolean.
	{
		switch (m_type)
		{
			case STRING:
				// From Moock
				if (get_current_root()->get_movie_version() >= 7)
				{
					return m_string_value.size() > 0 ? true : false;
				}

				if (m_string_value == "false")
				{
					return false;
				}
				else
				if (m_string_value == "true")
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

			case OBJECT:
				return m_object_value->to_bool();

			case PROPERTY:
			{
				as_value val;
				get_property(&val);
				return val.to_bool();
			}

			case NUMBER:
				return m_number != 0;

			case BOOLEAN:
				return m_bool;

			default:
				assert(m_type == UNDEFINED || m_type == NULLTYPE);
				return false;
		}
	}

	
	as_object*	as_value::to_object() const
	// Return value as an object.
	{
		if (m_type == OBJECT)
		{
			// OK.
			return m_object_value;
		}
		return NULL;
	}

	as_function*	as_value::to_function() const
	// Return value as an function.
	{
		if (m_type == OBJECT)
		{
			// OK.
			return cast_to<as_function>(m_object_value);
		}
		return NULL;
	}

	void	as_value::set_as_object(as_object* obj)
	{
		if (m_type != OBJECT || m_object_value != obj)
		{
			drop_refs();
			if (obj)
			{
				m_type = OBJECT;
				m_object_value = obj;
				m_object_value->add_ref();
			}
			else
			{
				m_type = NULLTYPE;
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
			case NUMBER:
				set_double(v.m_number);
				break;
			case BOOLEAN:
				set_bool(v.m_bool);
				break;
			case NULLTYPE:
				set_null();
				break;
			case STRING:
				set_tu_string(v.m_string_value);
				break;
			case OBJECT:
				set_as_object(v.m_object_value);
				break;

			case PROPERTY:
				drop_refs(); 
				
				// is binded property ?
				if (v.m_property_target == NULL)
				{
					m_type = PROPERTY;
					m_property = v.m_property;
					m_property->add_ref();
					m_property_target = v.m_property_target;
					if (m_property_target)
					{
						m_property_target->add_ref();
					}
				}
				else
				{
					v.get_property(this);
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

		switch (m_type)
		{
			case STRING:
				return m_string_value == v.to_tu_string();

			case NUMBER:
				return m_number == v.to_number();

			case BOOLEAN:
				return m_bool == v.to_bool();

			case OBJECT:
				return m_object_value == v.to_object();

			case PROPERTY:
			{
				as_value prop;
				get_property(&prop);
				return prop == v;
			}

			default:
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
		if (m_type == OBJECT)
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

	as_value::as_value(float val) :
		m_type(UNDEFINED)
	{
		set_double(val);
	}

	as_value::as_value(int val) :
		m_type(UNDEFINED)
	{
		set_double(val);
	}

	as_value::as_value(double val) :
		m_type(NUMBER),
		m_number(val)
	{
	}

	void	as_value::set_double(double val)
	{
		drop_refs(); m_type = NUMBER; m_number = val;
	}

	as_value::as_value(bool val) :
		m_type(BOOLEAN),
		m_bool(val)
	{
	}

	void	as_value::set_bool(bool val)
	{
		drop_refs(); m_type = BOOLEAN; m_bool = val;
	}


	bool as_value::is_function()
	{
		if (m_type == OBJECT)
		{
			return cast_to<as_function>(m_object_value);
		}
		return false;
	}

	as_value::as_value(as_c_function_ptr func) :
		m_type(UNDEFINED)
	{
		set_as_object(new as_c_function(func));
	}

	void	as_value::set_as_c_function(as_c_function_ptr func)
	{
		set_as_object(new as_c_function(func));
	}

	const char*	as_value::typeof() const
	{
		switch (m_type)
		{
			case UNDEFINED:
				return "undefined";

			case STRING:
				return "string";

			case NULLTYPE:
				return "null";

			case NUMBER:
				return "number";

			case BOOLEAN:
				return "boolean";

			case OBJECT:
				return m_object_value->typeof();
				break;

			case PROPERTY:
			{
				as_value val;
				get_property(&val);
				return val.typeof();
			}
		}
		assert(0);
		return 0;
	}


	bool as_value::get_method(as_value* func, const tu_string& name)
	{
		switch (m_type)
		{
			case STRING:
			{
				stringi_hash<as_value>* map = get_standard_method_map(BUILTIN_STRING_METHOD);
				return map->get(name, func);
			}

			case NUMBER:
			{
				stringi_hash<as_value>* map = get_standard_method_map(BUILTIN_NUMBER_METHOD);
				return map->get(name, func);
			}

			case BOOLEAN:
			{
				stringi_hash<as_value>* map = get_standard_method_map(BUILTIN_BOOLEAN_METHOD);
				return map->get(name, func);
			}

			case OBJECT:
			{
				return m_object_value->get_member(name, func);
			}
		}
		return false;
	}

	//
	//	as_property
	//

	as_property::as_property(const as_value& getter,	const as_value& setter) :
		m_getter(NULL),
		m_setter(NULL)
	{
		m_getter = cast_to<as_function>(getter.to_object());
		if (m_getter)
		{
			m_getter->add_ref();
		}

		m_setter = cast_to<as_function>(setter.to_object());
		if (m_setter)
		{
			m_setter->add_ref();
		}
	}

	as_property::~as_property()
	{
		if (m_getter)
		{
			m_getter->drop_ref();
		}
		if (m_setter)
		{
			m_setter->drop_ref();
		}
	}

	void	as_property::set(as_object* target, const as_value& val)
	{
		assert(target);

		as_environment env;
		env.push(val);
		if (m_setter)
		{
			(*m_setter)(fn_call(NULL, target,	&env, 1, env.get_top_index()));
		}
	}

	void as_property::get(as_object* target, as_value* val) const
	{
		assert(target);

		// env is used when m_getter->m_env is NULL
		as_environment env;
		if (m_getter)
		{
			(*m_getter)(fn_call(val, target, &env, 0,	0));
		}
	}

}
