// mydb.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient MYSQL plugin
// for the gameswf SWF player library.

#include "db.h"
#include "gameswf/gameswf_log.h"

#define ulong Uint32

mydb* get_mydb(as_object_interface* obj)
// gets plugin pointer
{
	if (obj)
	{
		// TODO: safe pointer conversion
		return (mydb*) obj->cast_to_as_object();
	}
	return NULL;
}

void	mydb_connect(const fn_call& fn)
//  Closes a previously opened connection & create new connection to db
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		if (fn.nargs < 5)
		{
			log_error("Db.Connect() needs host, dbname, user, pwd, socket args\n");
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
}

void	mydb_disconnect(const fn_call& fn)
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		db->disconnect();
	}
}

void	mydb_open(const fn_call& fn)
// Creates new table from sql statement & returns pointer to it
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		if (fn.nargs < 1)
		{
			log_error("Db.open() needs 1 arg\n");
			return;
		}

		mytable* tbl = db == NULL ? NULL : db->open(fn.arg(0).to_string());
		if (tbl)
		{
			fn.result->set_as_object_interface(tbl);
			return;
		}
	}
}

void	mydb_run(const fn_call& fn)
// Executes sql statement & returns affected rows
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		if (fn.nargs < 1)
		{
			log_error("Db.run() needs 1 arg\n");
			return;
		}

		fn.result->set_int(db->run(fn.arg(0).to_string()));
	}
}

void	mydb_commit(const fn_call& fn)
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		db->commit();
	}
}

void mydb_autocommit_setter(const fn_call& fn)
{
	mydb* db = get_mydb(fn.this_ptr);
	if (db)
	{
		db->set_autocommit(fn.arg(0).to_bool());
	}
}

// DLL interface
extern "C"
{
	exported_module as_object* gameswf_module_init(const array<as_value>& params)
	{
		return new mydb();
	}
}

mydb::mydb(): m_db(NULL)
{
	// methods
	as_object::set_member("connect", mydb_connect);
	as_object::set_member("disconnect", mydb_disconnect);
	as_object::set_member("open", mydb_open);
	as_object::set_member("run", mydb_run);
	as_object::set_member("commit", mydb_commit);
	as_object::set_member("auto_commit", as_value(NULL, mydb_autocommit_setter));
}

mydb::~mydb()
{
	disconnect();
}

void mydb::disconnect()
{
	if (m_db != NULL)
	{
		mysql_close(m_db);    
		m_db = NULL;
	}
}

bool mydb::connect(const char* host, const char* dbname, const char* user, 
										const char* pwd, const char* socket)
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

int mydb::run(const char *sql)
{
	if (runsql(sql))
	{
		return (int) mysql_affected_rows(m_db);
	}
	return 0;
}

mytable* mydb::open(const char* sql)
{
	if (runsql(sql))
	{
		// query succeeded, process any data returned by it
		MYSQL_RES* result = mysql_store_result(m_db);
		if (result)
		{
			mytable* tbl = new mytable();
			tbl->retrieve_data(result);
			mysql_free_result(result);
			return tbl;
		}
		log_error("select query does not return data\n");
		log_error("%s\n", sql);
	}
	return NULL;
}

void mydb::set_autocommit(bool autocommit)
{
	mysql_autocommit(m_db, autocommit ? 1 : 0);
}

void mydb::commit()
{
	mysql_commit(m_db);
}

bool mydb::runsql(const char* sql)
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

