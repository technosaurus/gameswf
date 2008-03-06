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

	// Unique id of a gameswf resource
	enum { m_class_id = AS_PLUGIN_MYTABLE };
	virtual bool is(int class_id)
	{
		if (m_class_id == class_id) return true;
		else return as_object::is(class_id);
	}

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
