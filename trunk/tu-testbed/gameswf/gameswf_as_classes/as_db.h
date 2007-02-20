// as_db.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL implementation code for the gameswf SWF player library.

#ifndef GAMESWF_AS_DB_H
#define GAMESWF_AS_DB_H

#ifdef USE_MYSQL

#include <mysql/mysql.h>
#include "../gameswf_as_classes/as_table.h"

namespace gameswf
{

	void	as_global_mysqldb_ctor(const fn_call& fn);

	struct as_db : public as_object
	{

		as_db();
		~as_db();

		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual void	set_member(const tu_stringi& name, const as_value& val);

		bool connect(const char* host, const char* dbname, const char* user, const char* pwd);
		void disconnect();
		as_table* open(const char* sql);
		int run(const char *sql);
		void set_autocommit(bool mode);
		void commit();

	private:

		bool runsql(const char* sql);
		MYSQL* m_db;
	};

}

#else

namespace gameswf
{
	void	as_global_mysqldb_ctor(const fn_call& fn);
}

#endif

#endif // GAMESWF_AS_DB_H
