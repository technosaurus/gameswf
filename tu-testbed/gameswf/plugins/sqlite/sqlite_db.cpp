// sqlite_db.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient sqlite plugin
// for the gameswf SWF player library.

#include "sqlite_db.h"
#include "gameswf/gameswf_mutex.h"
#include "gameswf/gameswf_log.h"

#define ulong Uint32

namespace sqlite_plugin
{
	extern tu_mutex s_sqlite_plugin_mutex;

	void	sqlite_connect(const fn_call& fn)
	//  Closes a previously opened connection & create new connection to db
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		fn.result->set_bool(false);
		if (db && fn.nargs > 0)
		{
			fn.result->set_bool(db->connect(fn.arg(0).to_string()));
		}
	}

	void	sqlite_disconnect(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			db->disconnect();
		}
	}

	void	sqlite_open(const fn_call& fn)
	// Creates new table from sql statement & returns pointer to it
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 1)
			{
				log_error("Db.open() needs 1 arg\n");
				return;
			}

			sqlite_table* tbl = (db == NULL) ? NULL : db->open(fn.arg(0).to_string());
			if (tbl)
			{
				fn.result->set_as_object(tbl);
				return;
			}
		}
	}

	void	sqlite_run(const fn_call& fn)
	// Executes sql statement & returns affected rows
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
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

	void	sqlite_commit(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			db->commit();
		}
	}

	void sqlite_autocommit_setter(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->set_autocommit(fn.arg(0).to_bool());
		}
	}

	void sqlite_trace_setter(const fn_call& fn)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->m_trace = fn.arg(0).to_bool();
		}
	}

	void sqlite_create_function(const fn_call& fn)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs >= 2)
		{
			db->create_function(fn.arg(0).to_string(), fn.arg(1).to_function());
		}
	}

	// DLL interface
	extern "C"
	{
		exported_module as_object* gameswf_module_init(player* player, const array<as_value>& params)
		{
			return new sqlite_db(player);
		}
	}

	sqlite_db::sqlite_db(player* player) :
		as_object(player),
		m_trace(false),
		m_db(NULL),
		m_autocommit(true)
	{
		// methods
		builtin_member("connect", sqlite_connect);
		builtin_member("disconnect", sqlite_disconnect);
		builtin_member("open", sqlite_open);
		builtin_member("run", sqlite_run);
		builtin_member("commit", sqlite_commit);
		builtin_member("auto_commit", as_value(as_value(), sqlite_autocommit_setter));
		builtin_member("trace", as_value(as_value(), sqlite_trace_setter));
		builtin_member("createFunction", sqlite_create_function);
	}

	sqlite_db::~sqlite_db()
	{
		disconnect();
	}

	void sqlite_db::disconnect()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (m_db != NULL)
		{
			sqlite3_close(m_db);
			m_db = NULL;
		}
	}

	bool sqlite_db::connect(const char* dbfile)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		// Closes a previously opened connection &
		// also deallocates the connection handle
		disconnect();

	  int rc = sqlite3_open_v2(dbfile, &m_db, SQLITE_OPEN_READWRITE, NULL);
	  if (rc != SQLITE_OK)
		{
			log_error("Can't open sqlite database: %s\n", sqlite3_errmsg(m_db));
			sqlite3_close(m_db);
			return false;
		}

		set_autocommit(true);
		return true;
	}

	bool sqlite_db::runsql(const char* sql)
	{
		if (m_db == NULL)
		{
			log_error("%s\n", sql);
			log_error("missing connection\n");
			return false;
		}

		sqlite3_stmt* stmt;		// Statement handle
		int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL);
	  if (rc != SQLITE_OK)
		{
			log_error("%s\n%s\n", sql, sqlite3_errmsg(m_db));
			return false;
		}

		rc = sqlite3_step(stmt);
		switch (rc)
		{
			case SQLITE_DONE:	// has finished executing
				sqlite3_finalize(stmt);
				return true;

			case SQLITE_ROW:	// has another row ready
				assert(m_result);
				m_result->retrieve_data(stmt);
				sqlite3_finalize(stmt);
				return true;

			default:
				log_error("%s\nerror #%d:%s\n", sql, rc, sqlite3_errmsg(m_db));
				sqlite3_finalize(stmt);
				return false;
		}
		return false;
	}

	int sqlite_db::run(const char *sql)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);
		if (m_trace) log_msg("run: %s\n", sql);

		runsql(sql);
		return sqlite3_changes(m_db);
	}

	sqlite_table* sqlite_db::open(const char* sql)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);
		if (m_trace) log_msg("open: %s\n", sql);

		m_result = new sqlite_table(get_player());
		runsql(sql);
		return m_result;
	}

	void sqlite_db::set_autocommit(bool autocommit)
	{
		m_autocommit = autocommit;
		commit();
	}

	void sqlite_db::commit()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		run("commit");
		if (m_autocommit == false)
		{
			run("begin");
		}
	}

	void sqlite_func(sqlite3_context *context, int argc, sqlite3_value **argv)
	{
		// TODO
		switch (sqlite3_value_type(argv[0]))
		{
			case SQLITE_INTEGER:
			{
				long long int iVal = sqlite3_value_int64(argv[0]);
				iVal = ( iVal > 0) ? 1: ( iVal < 0 ) ? -1: 0;
				sqlite3_result_int64(context, iVal);
				break;
			}
			case SQLITE_NULL:
			{
				sqlite3_result_null(context);
				break;
			}
			case SQLITE3_TEXT:
			{
				const unsigned char* str = sqlite3_value_text(argv[0]);
				sqlite3_result_text(context, (const char*) str, -1, NULL);
				break;
			}
			default:
			{
				double rVal = sqlite3_value_double(argv[0]);
				rVal = ( rVal > 0) ? 1: ( rVal < 0 ) ? -1: 0;
				sqlite3_result_double(context, rVal);
				break;
			}
		}
	}

	void sqlite_db::create_function(const char* name, as_function* func)
	{
		if (m_db)
		{
			sqlite3_create_function(m_db, name, -1, SQLITE_UTF8, this, sqlite_func, NULL, NULL);
		}
	}
}

