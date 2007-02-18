// array.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "../gameswf_as_classes/as_array.h"
//#include "../gameswf_log.h"

namespace gameswf
{

	void	as_global_array_ctor(const fn_call& fn)
	// Constructor for ActionScript class Array.
	{
		smart_ptr<as_array>	ao = new as_array;

		// case of "var x = ["abc","def", 1,2,3,4];"
		// called from "init array" operation only
		if (fn.nargs == -1 && fn.first_arg_bottom_index == -1)
		{
			assert(fn.env);

			// Use the arguments as initializers.
			int	size = (int) fn.env->pop().to_number();
			as_value	index_number;
			for (int i = 0; i < size; i++)
			{
				index_number.set_int(i);
				ao->set_member(index_number.to_string(), fn.env->pop());
			}
		}
		else

		// case of "var x = new Array(777)"
		if (fn.nargs == 1)
		{
			// Create an empty array with the given number of undefined elements.
			int size = fn.arg(0).get_type() == as_value::NUMBER ? (int) fn.arg(0).to_number() : 0;
			as_value	index_number;
			for (int i = 0; i < size; i++)
			{
				index_number.set_int(i);
				ao->set_member(index_number.to_string(), as_value());
			}
		}
		else

		// case of "var x = new Array()" or unhandled case 
		{
			// Empty array.
		}

		fn.result->set_as_object_interface(ao.get_ptr());
	}


	as_array::as_array()
	{
		//			this->set_member("join", &array_not_impl);
		//			this->set_member("concat", &array_not_impl);
		//			this->set_member("slice", &array_not_impl);
		//			this->set_member("push", &array_not_impl);
		//			this->set_member("unshift", &array_not_impl);
		//			this->set_member("pop", &array_not_impl);
		//			this->set_member("shift", &array_not_impl);
		//			this->set_member("splice", &array_not_impl);
		//			this->set_member("sort", &array_not_impl);
		//			this->set_member("sortOn", &array_not_impl);
		//			this->set_member("reverse", &array_not_impl);
		//			this->set_member("toString", &array_not_impl);
		//	
	}

	bool as_array::get_member(const tu_stringi& name, as_value* val)
	{
		if (name == "length")
		{
			val->set_int(as_object::m_members.size());
			return true;
		}
		return as_object::get_member(name, val);
	}

	void as_array::set_member(const tu_stringi& name, const as_value& val)
	{
		if (name == "length")
		{
			// can't set length property
			return;
		}
		as_object::set_member(name, val);
	}


};
