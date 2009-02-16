// array.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_array.h"
#include "gameswf/gameswf_function.h"

namespace gameswf
{

	void	as_array_join(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);

		if(a && fn.nargs > 0)
		{
			tu_string result;
			const char * separator = fn.arg(0).to_string();

			int size = a->size();
			for(int index = 0; index < size; ++index )
			{
				as_value _index( index );
				as_value value;
				if( index != 0 ) result += separator;
				a->get_member( _index.to_tu_stringi(), &value );

				result += value.to_tu_string();
			}

			fn.result->set_tu_string( result );
		}
		else
		{
			fn.result->set_undefined();
		}
	}

	void	as_array_tostring(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a)
		{
			fn.result->set_tu_string(a->to_string());
		}
	}

	void	as_array_push(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a)
		{
			if (fn.nargs > 0)
			{
				a->push(fn.arg(0));
			}
			fn.result->set_int(a->size());
		}
	}

	// remove the first item of array
	void	as_array_shift(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a)
		{
			as_value val;
			a->shift(&val);
			*fn.result = val;
		}
	}

	void	as_array_pop(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a)
		{
			as_value val;
			a->pop(&val);
			*fn.result = val;
		}
	}

	void	as_array_length(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a)
		{
			fn.result->set_int(a->size());
		}
	}

	void	as_global_array_ctor(const fn_call& fn)
	// Constructor for ActionScript class Array.
	{
		gc_ptr<as_array>	ao = new as_array(fn.get_player());

		// case of "var x = ["abc","def", 1,2,3,4,..];"
		// called from "init array" operation only
		if (fn.nargs == -1 && fn.first_arg_bottom_index == -1)
		{

			// Use the arguments as initializers.
			int	size = fn.env->pop().to_int();
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
			int size = fn.arg(0).to_int();
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

		fn.result->set_as_object(ao.get_ptr());
	}


	as_array::as_array(player* player) :
		as_object(player)
	{
		builtin_member("join", as_array_join);
		//			this->set_member("concat", &array_not_impl);
		//			this->set_member("slice", &array_not_impl);
		//			this->set_member("unshift", &array_not_impl);
		//			this->set_member("splice", &array_not_impl);
		//			this->set_member("sort", &array_not_impl);
		//			this->set_member("sortOn", &array_not_impl);
		//			this->set_member("reverse", &array_not_impl);
		builtin_member("shift", as_array_shift);
		builtin_member("toString", as_array_tostring);
		builtin_member("push", as_array_push);
		builtin_member("pop", as_array_pop);
		builtin_member("length", as_value(as_array_length, as_value()));

		set_ctor(as_global_array_ctor);
	}

	const char* as_array::to_string()
	{
		// receive indexes
		// array may contain not numerical indexes
		array<tu_stringi> idx;
		for (stringi_hash<as_value>::iterator it = m_members.begin(); it != m_members.end(); ++it)
		{
			if (it->second.is_enum())
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
					tu_swap(&idx[i], &idx[j]);
				}
			}
		}

		// form string
		m_string_value = "";
		for (int i = 0; i < n; i++)
		{
			as_value val;
			as_object::get_member(idx[i], &val);
			m_string_value += val.to_string();

			if (i < n - 1)
			{
				m_string_value +=  ",";
			}
		}

		return m_string_value.c_str();
	}

	int as_array::size()
	// get array size, excluding own methods
	{
		int n = 0;
		for (stringi_hash<as_value>::iterator it = m_members.begin(); it != m_members.end(); ++it)
		{
			if (it->second.is_enum())
			{
				n++;
			}
		}
		return n;
	}

	void as_array::push(const as_value& val)
	// Insert the given value at the end of the array.
	{
		as_value index(size());
		set_member(index.to_tu_stringi(), val);
	}

	void as_array::shift(as_value* val)
	// Removes the first element from an array and returns the value of that element.
	{
		assert(val);

		// receive numerical indexes
		array<int> idx;
		for (stringi_hash<as_value>::iterator it = m_members.begin(); it != m_members.end(); ++it)
		{
			int index;
			if (string_to_number(&index, it->first.c_str()) && it->second.is_enum())
			{
				idx.push_back(index);
			}
		}

		// sort indexes
		for (int i = 0, n = idx.size(); i < n - 1; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				if (idx[i] > idx[j])
				{
					tu_swap(&idx[i], &idx[j]);
				}
			}
		}

		// keep the first element
		get_member(tu_string(idx[0]), val);

		// shift elements
		for (int i = 1; i < idx.size(); i++)
		{
			as_value v;
			get_member(tu_string(idx[i]), &v);
			set_member(tu_string(idx[i - 1]), v);
		}

		// remove the last element
		erase(tu_string(idx[idx.size() - 1]));
	}

	void as_array::pop(as_value* val)
	// Removes the last element from an array and returns the value of that element.
	{
		assert(val);
		as_value index(size() - 1);
		if (get_member(index.to_tu_stringi(), val))
		{
			erase(index.to_tu_stringi());
		}
	}

	void as_array::erase(const tu_stringi& index)
	// erases one member from array
	{
		m_members.erase(index);
	}

	void as_array::clear()
	// erases all members from array
	{
		for (int i = 0, n = size(); i < n; i++)
		{
			as_value index(i);
			m_members.erase(index.to_tu_stringi());
		}
	}

};
