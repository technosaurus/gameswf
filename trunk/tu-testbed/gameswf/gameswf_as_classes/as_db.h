// as_db.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL implementation code for the gameswf SWF player library.

#ifndef GAMESWF_AS_DB_H
#define GAMESWF_AS_DB_H

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include <mysql/mysql.h>
#include "gameswf/gameswf_as_classes/as_table.h"

namespace gameswf
{

	void	as_global_mysqldb_ctor(const fn_call& fn);

	struct as_db : public as_object
	{

		as_db();
		~as_db();

		virtual as_db* cast_to_as_db() { return this; }
		bool connect(const char* host, const char* dbname, const char* user,
			const char* pwd, const char* socket);
		void disconnect();
		as_table* open(const char* sql);
		int run(const char *sql);
		void set_autocommit(bool autocommit);
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
