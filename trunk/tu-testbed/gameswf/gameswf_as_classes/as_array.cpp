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
			a->remove(0, &val);
			*fn.result = val;
		}
	}

	// public splice(startIndex:Number, [deleteCount:Number], [value:Object]) : Array
	// adds elements to and removes elements from an array.
	// This method modifies the array without making a copy.
	void	as_array_splice(const fn_call& fn)
	{
		as_array* a = cast_to<as_array>(fn.this_ptr);
		if (a && fn.nargs >= 1)
		{
			int index = fn.arg(0).to_int();
			int asize = a->size();	// for optimization
			if (index >=0 && index < asize)
			{

				// delete items

				as_array* deleted_items = new as_array(a->get_player());
				fn.result->set_as_object(deleted_items);

				int delete_count = asize - index;
				if (fn.nargs >= 2)
				{
					delete_count = fn.arg(1).to_int();
					if (delete_count > asize - index)
					{
						delete_count = asize - index;
					}
				}

				for (int i = 0; i < delete_count; i++)
				{
					as_value val;
					a->remove(index, &val);
					deleted_items->push(val);
				}

				// insert values

				if (fn.nargs >= 3)
				{
					const as_value& val = fn.arg(2);
					as_array* obj = cast_to<as_array>(val.to_object());
					if (obj)
					{
						// insert an array
						for (int i = obj->size() - 1; i >= 0; i--)
						{
							as_value val;
							obj->get_member(tu_string(i), &val);
							a->insert(index, val);
						}
					}
					else
					{
						// insert an item
						a->insert(index, val);
					}
				}
			}
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
		//			this->set_member("sort", &array_not_impl);
		//			this->set_member("sortOn", &array_not_impl);
		//			this->set_member("reverse", &array_not_impl);
		builtin_member("shift", as_array_shift);
		builtin_member("toString", as_array_tostring);
		builtin_member("push", as_array_push);
		builtin_member("pop", as_array_pop);
		builtin_member("length", as_value(as_array_length, as_value()));
		builtin_member("splice", as_array_splice);

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

	void as_array::remove(int index, as_value* val)
	// Removes the element starting from 'index' and returns the value of that element.
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

		if (index >= 0 && index < idx.size())
		{
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

			// keep the element starting from 'index'
			get_member(tu_string(idx[index]), val);

			// shift elements
			for (int i = index + 1; i < idx.size(); i++)
			{
				as_value v;
				get_member(tu_string(idx[i]), &v);
				set_member(tu_string(idx[i - 1]), v);
			}

			// remove the last element
			erase(tu_string(idx[idx.size() - 1]));
		}
		else
		{
			//assert(0);
		}
	}


	void as_array::insert(int index, const as_value& val)
	// Inserts the element after 'index'
	{
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

		if (index >= 0 && index < idx.size())
		{
			// add new index
			idx.push_back(idx.size());

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

			// shift elements
			for (int i = idx.size() - 1; i > index; i--)
			{
				as_value v;
				get_member(tu_string(idx[i - 1]), &v);
				set_member(tu_string(idx[i]), v);
			}

			// insert
			set_member(tu_string(idx[index]), val);

		}
		else
		{
			//assert(0);
		}
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
