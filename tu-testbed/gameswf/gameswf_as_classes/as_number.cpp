// as_number.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_number.h"

namespace gameswf
{

	// Number(num:Object)
	void	as_global_number_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.result->set_as_object(new as_number(fn.arg(0).to_number()));
		}	
	}

	void	number_to_string(const fn_call& fn)
	{
		as_number* obj = cast_to<as_number>(fn.this_ptr);
		if (obj)
		{
			if (fn.nargs > 0)
			{
				fn.result->set_string(obj->to_string(fn.arg(0).to_int()));
			}
			else
			{
				fn.result->set_string(obj->to_string());
			}
		}
	}

	void	number_value_of(const fn_call& fn)
	{
		as_number* obj = cast_to<as_number>(fn.this_ptr);
		if (obj)
		{
			fn.result->set_as_object(obj);
		}
	}

	as_number::as_number(double val) :
		m_val(val)
	{
		stringi_hash<as_c_function_ptr>* map = get_standard_method_map(BUILTIN_NUMBER_METHOD);
		if (map->size() == 0)
		{
			map->add("toString", number_to_string);
			map->add("valueOf", number_value_of);
		}
	}

	// NOT THREAD SAFE!!!
	const char*	as_number::to_string()
	{
		// @@ Moock says if value is a NAN, then result is "NaN"
		// INF goes to "Infinity"
		// -INF goes to "-Infinity"
		if (isnan(m_val))
		{
			return "NaN";
		} 
		else
		{
			static char buffer[50];
			snprintf(buffer, 50, "%.14g", m_val);
			return buffer;
		}
	}

	// radix:Number - Specifies the numeric base (from 2 to 36) to use for 
	// the number-to-string conversion. 
	// If you do not specify the radix parameter, the default value is 10.
	static const char s_hex_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZW";
	const char*	as_number::to_string(int radix)
	{
		tu_string res;
		int val = (int) m_val;
		if (radix >= 2 && radix <= strlen(s_hex_digits))
		{
			do
			{
				int k = val % radix;
				val = (int) (val / radix);
				tu_string digit;
				digit += s_hex_digits[k];
				res = digit + res;
			}
			while (val > 0);
		}
		return res;
	}

	bool	as_number::get_member(const tu_stringi& name, as_value* val)
	{
		// first try built-ins methods
		stringi_hash<as_c_function_ptr>* map = get_standard_method_map(BUILTIN_NUMBER_METHOD);
		as_c_function_ptr builtin;
		if (map->get(name, &builtin))
		{
			val->set_as_c_function_ptr(builtin);
			return true;
		}

		return as_object::get_member(name, val);
	}


};
