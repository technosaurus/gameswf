// array.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_array.h"
//#include "gameswf/gameswf_log.h"

namespace gameswf
{

	void	as_array_tostring(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_array* a = fn.this_ptr->cast_to_as_array();
		assert(a);

		fn.result->set_tu_string(a->to_string());
	}

	void	as_array_push(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_array* a = fn.this_ptr->cast_to_as_array();
		assert(a);

		if (fn.nargs > 0)
		{
			a->push_back(fn.arg(0));
		}
		fn.result->set_int(a->size());
	}


	void	as_global_array_ctor(const fn_call& fn)
	// Constructor for ActionScript class Array.
	{
		smart_ptr<as_array>	ao = new as_array;

		// case of "var x = ["abc","def", 1,2,3,4,..];"
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

		// case of "var x = new Array(1,2,3,4,5,6,7,8,..);"
		{
			assert(fn.env);

			// Use the arguments as initializers.
			as_value	index_number;
			for (int i = 0; i < fn.nargs; i++)
			{
				index_number.set_int(i);
				ao->set_member(index_number.to_string(), fn.arg(i));
			}
		}

		fn.result->set_as_object_interface(ao.get_ptr());
	}


	as_array::as_array()
	{
		//			this->set_member("join", &array_not_impl);
		//			this->set_member("concat", &array_not_impl);
		//			this->set_member("slice", &array_not_impl);
		//			this->set_member("unshift", &array_not_impl);
		//			this->set_member("pop", &array_not_impl);
		//			this->set_member("shift", &array_not_impl);
		//			this->set_member("splice", &array_not_impl);
		//			this->set_member("sort", &array_not_impl);
		//			this->set_member("sortOn", &array_not_impl);
		//			this->set_member("reverse", &array_not_impl);
		set_member("toString", &as_array_tostring);
		set_member_flags("toString", as_prop_flags::DONT_ENUM);
		set_member("push", &as_array_push);
		set_member_flags("push", as_prop_flags::DONT_ENUM);
	}

	bool as_array::get_member(const tu_stringi& name, as_value* val)
	{
		if (name == "length")
		{
			val->set_int(size());
			return true;
		}
		
		return as_object::get_member(name, val);
	}

	bool as_array::set_member(const tu_stringi& name, const as_value& val)
	{
		if (name == "length")
		{
			// can't set length property
			return true;
		}
		return as_object::set_member(name, val);
	}

	tu_string as_array::to_string()
	{
		// receive indexes
		// array may contain not numerical indexes
		array<tu_stringi> idx;
		for (stringi_hash<as_member>::iterator it = m_members.begin(); it != m_members.end(); ++it)
		{
			if (it->second.get_member_flags().get_dont_enum() == false)
			{
				idx.push_back(it->first);
			}
		}

		// sort indexes
		int n = idx.size();
		for (int i = 0; i < n - 1; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				if (idx[i] > idx[j])
				{
					tu_stringi temp;
					temp = idx[i];
					idx[i] = idx[j];
					idx[j] = temp;
				}
			}
		}

		// form string
		tu_string s;
		for (int i = 0; i < n; i++)
		{
			as_value val;
			as_object::get_member(idx[i], &val);
			s += val.to_string();

			if (i < n - 1)
			{
				s +=  ",";
			}
		}

		return s;
	}

	int as_array::size()
	// get array size, excluding own methods
	{
		int n = 0;
		for (stringi_hash<as_member>::iterator it = m_members.begin(); it != m_members.end(); ++it)
		{
			if (it->second.get_member_flags().get_dont_enum() == false)
			{
				n++;
			}
		}
		return n;
	}

	void as_array::push_back(const as_value& val)
	// Insert the given value at the end of the array.
	{
		as_value index(size());
		set_member(index.to_tu_stringi(), val);
	}

	void as_array::erase(const tu_stringi& index)
	// erases one member from array
	{
		m_members.erase(index);
	}

};
