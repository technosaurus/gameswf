// as_table.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL table implementation code for the gameswf SWF player library.

#ifndef GAMESWF_AS_TABLE_H
#define GAMESWF_AS_TABLE_H

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include <mysql/mysql.h>
#include "gameswf/gameswf_action.h"	// for as_object

namespace gameswf
{

	struct as_table: public as_object
	{

		as_table();
		~as_table();

		virtual as_table* cast_to_as_table() { return this; }
		virtual bool	get_member(const tu_stringi& name, as_value* val);

		int size() const;
		bool prev();
		bool next();
		void first();
		int fld_count();
		bool goto_record(int index);
		const char* get_field_title(int n);
		void retrieve_data(MYSQL_RES* result);
		int get_recno() const;


	private:
		int m_index;
		array< smart_ptr<as_object> > m_data;	// [columns][rows]
		array< tu_stringi > m_title;
	};

}

#else

#endif

#endif // GAMESWF_AS_TABLE_H
