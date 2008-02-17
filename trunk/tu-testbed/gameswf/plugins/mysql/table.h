// mytable.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL table implementation for the gameswf SWF player library.

#ifndef GAMESWF_MYTABLE_H
#define GAMESWF_MYTABLE_H

#include <mysql/mysql.h>
#include "gameswf/gameswf_action.h"	// for as_object

using namespace gameswf;

struct mytable: public as_object
{

	mytable();
	~mytable();

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


#endif // GAMESWF_MYTABLE_H
