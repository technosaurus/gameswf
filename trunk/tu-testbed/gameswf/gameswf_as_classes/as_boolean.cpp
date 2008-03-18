// as_boolean.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_boolean.h"

namespace gameswf
{

	// boolean(num:Object)
	void	as_global_boolean_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.result->set_as_object(new as_boolean(fn.arg(0).to_bool()));
		}	
	}

	void	boolean_to_string(const fn_call& fn)
	{
		as_boolean* obj = cast_to<as_boolean>(fn.this_ptr);
		if (obj)
		{
			fn.result->set_string(obj->to_string());
		}
	}

	void	boolean_value_of(const fn_call& fn)
	{
		as_boolean* obj = cast_to<as_boolean>(fn.this_ptr);
		if (obj)
		{
			fn.result->set_as_object(obj);
		}
	}

	as_boolean::as_boolean(double val) :
		m_val(val)
	{
		stringi_hash<as_c_function_ptr>* map = get_standard_method_map(BUILTIN_BOOLEAN_METHOD);
		if (map->size() == 0)
		{
			map->add("toString", boolean_to_string);
			map->add("valueOf", boolean_value_of);
		}
	}

	const char*	as_boolean::to_string()
	{
		return m_val ? "true" : "false";
	}

	bool	as_boolean::get_member(const tu_stringi& name, as_value* val)
	{
		// first try built-ins methods
		stringi_hash<as_c_function_ptr>* map = get_standard_method_map(BUILTIN_BOOLEAN_METHOD);
		as_c_function_ptr builtin;
		if (map->get(name, &builtin))
		{
			val->set_as_c_function_ptr(builtin);
			return true;
		}

		return as_object::get_member(name, val);
	}


};
