// as_db.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple, working and convenient MYSQL extension
// for the gameswf SWF player library.

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include "gameswf/gameswf_as_classes/as_db.h"
#include "gameswf/gameswf_log.h"

namespace gameswf
{

	#define ulong Uint32

	void	as_db_connect(const fn_call& fn)
	//  Closes a previously opened connection & create new connection to db
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (fn.nargs < 5)
		{
			log_error("Db.Connect needs host, dbname, user, pwd, socket args\n");
			return;
		}

		if (db)
		{
			fn.result->set_bool(db->connect(fn.arg(0).to_string(),	// host
				fn.arg(1).to_string(),	// dbname
				fn.arg(2).to_string(),	// user
				fn.arg(3).to_string(),	// pwd
				fn.arg(4).to_string()));	// socket
		}
	}

	void	as_db_disconnect(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (db)
		{
			db->disconnect();
		}
	}

	void	as_db_open(const fn_call& fn)
	// Creates new table from sql statement & returns pointer to it
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (fn.nargs < 1)
		{
			log_error("Db.open needs 1 arg\n");
			return;
		}

		as_table* tbl = db == NULL ? NULL : db->open(fn.arg(0).to_string());
		if (tbl)
		{
			fn.result->set_as_object_interface(tbl);
			return;
		}
		fn.result->set_undefined();
	}

	void	as_db_run(const fn_call& fn)
	// Executes sql statement & returns affected rows
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (fn.nargs < 1)
		{
			log_error("Db.run needs 1 arg\n");
			return;
		}

		if (db)
		{
			fn.result->set_int(db->run(fn.arg(0).to_string()));
		}
	}

	void	as_db_commit(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (db)
		{
			db->commit();
		}
	}

	void as_db_autocommit_setter(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_db* db = fn.this_ptr->cast_to_as_db();

		if (db)
		{
			db->set_autocommit(fn.arg(0).to_bool());
		}
	}

	void	as_global_mysqldb_ctor(const fn_call& fn)
	// Constructor for ActionScript class NetStream.
	{
		smart_ptr<as_object>	db(new as_db);

		// methods
		db->set_member("connect", as_db_connect);
		db->set_member("disconnect", as_db_disconnect);
		db->set_member("open", as_db_open);
		db->set_member("run", as_db_run);
		db->set_member("commit", as_db_commit);
		db->set_member("auto_commit", as_value(NULL, as_db_autocommit_setter));

		fn.result->set_as_object_interface(db.get_ptr());
	}


	as_db::as_db(): m_db(NULL)
	{
	}

	as_db::~as_db()
	{
		disconnect();
	}

	void as_db::disconnect()
	{
		if (m_db != NULL)
		{
			mysql_close(m_db);    
			m_db = NULL;
		}
	}

	bool as_db::connect(const char* host,
		const char* dbname, 
		const char* user, 
		const char* pwd,
		const char* socket)
	{
		// Closes a previously opened connection &
		// also deallocates the connection handle
		disconnect();

		m_db = mysql_init(NULL);

		if ( m_db == NULL )
		{
			return false;
		}

		// real connect
		if (mysql_real_connect(
			m_db,
			strlen(host) > 0 ? host : NULL,
			strlen(user) > 0 ? user : NULL,
			strlen(pwd) > 0 ? pwd : NULL,
			strlen(dbname) > 0 ? dbname : NULL,
			0,
			strlen(socket) > 0 ? socket : NULL,
			CLIENT_MULTI_STATEMENTS) == NULL)
		{
			log_error("%s\n", mysql_error(m_db));
			return false;
		}

		set_autocommit(true);
		return true;
	}

	int as_db::run(const char *sql)
	{
		if (runsql(sql))
		{
			return (int) mysql_affected_rows(m_db);
		}
		return 0;
	}

	as_table* as_db::open(const char* sql)
	{
		if (runsql(sql))
		{
			// query succeeded, process any data returned by it
			MYSQL_RES* result = mysql_store_result(m_db);
			if (result)
			{
				as_table* tbl = new as_table();
				tbl->retrieve_data(result);
				mysql_free_result(result);
				return tbl;
			}
			log_error("select query does not return data\n");
			log_error("%s\n", sql);
		}
		return NULL;
	}

	void as_db::set_autocommit(bool autocommit)
	{
		mysql_autocommit(m_db, autocommit ? 1 : 0);
	}

	void as_db::commit()
	{
		mysql_commit(m_db);
	}

	bool as_db::runsql(const char* sql)
	{
		if (m_db == NULL)
		{
			log_error("missing connection\n");
			log_error("%s\n", sql);
			return false;
		}

		if (mysql_query(m_db, sql))
		{
			log_error("%s\n", mysql_error(m_db));
			log_error("%s\n", sql);
			return false;
		}
		return true;
	}

}

#else

#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_log.h"

namespace gameswf
{
	void	as_global_mysqldb_ctor(const fn_call& fn)
		// Constructor for ActionScript class NetStream.
	{
		fn.result->set_undefined();
		log_error("'new MyDb' requires USE_MYSQL compiler option\n");
	}
}

#endif
