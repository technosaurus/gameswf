// as_xmlsocket.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script XMLSocket implementation code for the gameswf SWF player library.

#include "gameswf/gameswf_as_classes/as_xmlsocket.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_function.h"
//#include "gameswf/gameswf_log.h"

#define XML_TIMEOUT 1 //sec
#define XML_MAXDATASIZE 102400 //100KB

namespace gameswf
{

	void	as_xmlsock_connect(const fn_call& fn)
	{
		if (fn.nargs != 2)
		{
			fn.result->set_bool(false);
			return;
		}

		assert(fn.this_ptr);
		as_xmlsock* xmls = fn.this_ptr->cast_to_as_xmlsock();
		assert(xmls);

		fn.result->set_bool(xmls->connect(fn.arg(0).to_string(), (int) fn.arg(1).to_number()));
	}

	void	as_xmlsock_close(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_xmlsock* xmls = fn.this_ptr->cast_to_as_xmlsock();
		assert(xmls);

		xmls->close();
	}

	void	as_xmlsock_send(const fn_call& fn)
	{
		if (fn.nargs != 1)
		{
			return;
		}

		assert(fn.this_ptr);
		as_xmlsock* xmls = fn.this_ptr->cast_to_as_xmlsock();
		assert(xmls);

		xmls->send(fn.arg(0));
	}

	void	as_global_xmlsock_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
		fn.result->set_as_object_interface(new as_xmlsock);
	}


	as_xmlsock::as_xmlsock() :
		m_iface(NULL),
		m_ns(NULL)
	{
		set_member("connect", &as_xmlsock_connect);
		set_member("close", &as_xmlsock_close);
		set_member("send", &as_xmlsock_send);
		m_iface = new net_interface_tcp();
	}

	as_xmlsock::~as_xmlsock()
	{
		close();
		delete m_ns;
		m_ns = NULL;
		delete m_iface;
		m_iface = NULL;
	}

	bool as_xmlsock::connect(const char* host, int port)
	{
		m_ns = m_iface->connect(host, port);
		bool is_connected = m_ns ? true : false;

		as_value function;
		if (get_member("onConnect", &function))
		{
			as_environment* env = function.to_as_function()->m_env;
			assert(env);

			env->push(is_connected);
			call_method(function, env, NULL, 1, env->get_top_index());
			env->drop(1);
		}

		// add to net listener
		if (is_connected)
		{
			get_root()->add_listener(this, movie_root::ADVANCE);
		}

		return is_connected;
	}

	void as_xmlsock::close()
	{
		get_root()->remove_listener(this);
	}

	void as_xmlsock::send(const as_value& val) const
	{
		if (m_ns)
		{
			m_ns->write_string(val.to_tu_string(), XML_TIMEOUT);
		}
	}

	// called from movie_root
	// To check up presence of data which have come from a network
	void	as_xmlsock::advance(float delta_time)
	{
		assert(m_ns);

		if (m_ns->is_readable())
		{
			tu_string str;
			m_ns->read_line(&str, XML_MAXDATASIZE, XML_TIMEOUT);

			// If the connection has been gracefully closed, 
			// the size of return value is zero
			if (str.size() == 0)
			{
				close();
				as_value function;
				if (get_member("onClose", &function))
				{
					as_environment* env = function.to_as_function()->m_env;
					assert(env);

					call_method(function, env, NULL, 0, env->get_top_index());
				}
			}
			else
			{
				as_value function;
				if (get_member("onData", &function))
				{
					as_environment* env = function.to_as_function()->m_env;
					assert(env);

					env->push(str);
					call_method(function, env, NULL, 1, env->get_top_index());
					env->drop(1);
				}
			}
		}
	}

};
