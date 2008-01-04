 // as_table.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple, working and convenient MYSQL extension
// for the gameswf SWF player library.

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_as_classes/as_table.h"

namespace gameswf
{

	void	to_string_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		assert(fn.this_ptr);	assert(fn.env);
		fn.result->set_string("[as_table]");
	}

	void	size_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		fn.result->set_int(tbl->size());
	}

	void	next_method(const fn_call& fn)
	// Moves row pointer to next row
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		tbl->next();
	}

	void	prev_method(const fn_call& fn)
	// Moves row pointer to prev row
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		tbl->prev();
	}

	void	first_method(const fn_call& fn)
	// Moves row pointer to the first row
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		tbl->first();
	}

	void	field_count_method(const fn_call& fn)
	// Returns amount of the fields in the table
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		fn.result->set_int(tbl->fld_count());
	}

	void	goto_record_method(const fn_call& fn)
	// Moves row pointer to the fn.arg(0).to_number()
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		assert(fn.nargs == 1);
		tbl->goto_record((int) fn.arg(0).to_number());
	}

	void	get_title_method(const fn_call& fn)
	// Returns the name of fn.arg(0).to_number() field
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		assert(fn.nargs == 1);
		fn.result->set_string(tbl->get_field_title((int) fn.arg(0).to_number()));
	}

	void	to_get_recno_method(const fn_call& fn)
	// Returns the current record number
	{
		assert(fn.this_ptr);	assert(fn.env);
		as_table* tbl = fn.this_ptr->cast_to_as_table();
		if (tbl->size() > 0)
		{
			fn.result->set_int(tbl->get_recno() + 1);
		}
	}

	as_table::as_table():
		m_index(0)
	{
		as_object::set_member("size", &size_method);
		as_object::set_member("next", &next_method);
		as_object::set_member("prev", &prev_method);
		as_object::set_member("first", &first_method);
		as_object::set_member("fields", &field_count_method);
		as_object::set_member("goto", &goto_record_method);
		as_object::set_member("title", &get_title_method);
		as_object::set_member("toString", &to_string_method);
		as_object::set_member("getRecNo", &to_get_recno_method);
	}

	as_table::~as_table()
	{
	}

	bool	as_table::get_member(const tu_stringi& name, as_value* val)
	{
		// check table methods
		if (as_object::get_member(name, val) == false)
		{

			if (m_index >= m_data.size())
			{
				return false;
			}

			if (m_data[m_index]->get_member(name, val))
			{
				return true;
			}

			// try as field number
			int n = atoi(name.c_str());
			if (n >= 0 && n < (int) m_title.size())
			{
				if (m_data[m_index]->get_member(m_title[n], val))
				{
					return true;
				}
			}

			val->set_undefined();
			return false;
		}
		return true;
	}

	int as_table::size() const
	{
		return m_data.size();
	}

	int as_table::get_recno() const
	{
		return m_index;
	}

	void as_table::prev()
	{
		if (m_index > 0)
		{
			m_index--;
		}
	}

	void as_table::next()
	{
		if (m_index < m_data.size() - 1)
		{
			m_index++;
		}
	}

	void as_table::first()
	{
		m_index = 0;
	}

	int as_table::fld_count()
	{
		return m_title.size();
	}

	void as_table::goto_record(int index)
	{
		if (index < (int) m_data.size() && index >= 0)
		{
			m_index = index;
		}
	}

	const char* as_table::get_field_title(int n)
	{
		if (n >= 0 && n < (int) m_title.size())
		{
			return m_title[n].c_str();
		}
		return NULL;
	}

	void as_table::retrieve_data(MYSQL_RES* result)
	{
		MYSQL_FIELD* fld = mysql_fetch_fields(result);
		int num_fields = mysql_num_fields(result);
		int num_rows =  (int) mysql_num_rows(result);

		m_title.resize(num_fields);
		for (int i = 0; i < num_fields; i++)
		{
			m_title[i] = fld[i].name;
		}

		m_data.resize(num_rows);
		for (int i = 0; i < num_rows; i++)
		{
			MYSQL_ROW row = mysql_fetch_row(result);

			m_data[i] = new as_object();

			for (int j = 0; j < num_fields; j++)
			{
				as_value val;
				if (row[j] == NULL)
				{
					//					val.set_undefined();
					val.set_string("");
				}
				else
				{
					switch (fld[j].type)
					{
					case MYSQL_TYPE_TINY:
					case MYSQL_TYPE_SHORT:
					case MYSQL_TYPE_INT24:
						val.set_int(atoi(row[j]));
						break;

					case MYSQL_TYPE_DECIMAL:
					case MYSQL_TYPE_LONG:
					case MYSQL_TYPE_FLOAT:
					case MYSQL_TYPE_DOUBLE:
					case MYSQL_TYPE_LONGLONG:
						val.set_double(atof(row[j]));
						break;

					case MYSQL_TYPE_NULL:
					case MYSQL_TYPE_TIMESTAMP:
					case MYSQL_TYPE_DATE:
					case MYSQL_TYPE_TIME:
					case MYSQL_TYPE_DATETIME:
					case MYSQL_TYPE_YEAR:
					case MYSQL_TYPE_NEWDATE:
					case MYSQL_TYPE_VARCHAR:
					case MYSQL_TYPE_BIT:
					case MYSQL_TYPE_NEWDECIMAL:
					case MYSQL_TYPE_ENUM:
					case MYSQL_TYPE_SET:
					case MYSQL_TYPE_TINY_BLOB:
					case MYSQL_TYPE_MEDIUM_BLOB:
					case MYSQL_TYPE_LONG_BLOB:
					case MYSQL_TYPE_BLOB:
					case MYSQL_TYPE_VAR_STRING:
					case MYSQL_TYPE_STRING:
					case MYSQL_TYPE_GEOMETRY:
						val.set_string(row[j]);
						break;
					}
				}
				m_data[i]->set_member(fld[j].name, val);
			}
		}
	}

}

#else

#endif	// #if TU_CONFIG_LINK_TO_MYSQL == 1
