// as_load_vars.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_loadvars.h"
#include "gameswf/gameswf_root.h"
#include "base/container.h"
#include "net/http_helper.h"

namespace gameswf
{

	// Point(x:Number, y:Number)
	void	as_global_loadvars_ctor(const fn_call& fn)
	{
		smart_ptr<as_loadvars>	obj;
		obj = new as_loadvars();
		fn.result->set_as_object(obj.get_ptr());
	}

	//addRequestHeader(header:Object, headerValue:String) : Void
	void	as_loadvars_addrequestheader(const fn_call& fn)
	{
		if( fn.nargs == 1 )
		{
			as_loadvars * loadvars = cast_to<as_loadvars>( fn.this_ptr );

			assert( loadvars );

			assert( 0 && "todo" );
		}
		else if( fn.nargs == 2 )
		{
			as_loadvars * loadvars = cast_to<as_loadvars>( fn.this_ptr );

			assert( loadvars );

			if( fn.arg(0).is_string() && fn.arg(1).is_string() )
				loadvars->add_header( fn.arg(0).to_tu_string(), fn.arg(1).to_tu_string() );
		}
	}

	//decode(queryString:String) : Void
	void	as_loadvars_decode(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	//getBytesLoaded() : Number
	void	as_loadvars_getbytesloaded(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	//getBytesTotal() : Number
	void	as_loadvars_getbytestotal(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	//load(url:String) : Boolean
	void	as_loadvars_load(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	//send(url:String, target:String, [method:String]) : Boolean
	void	as_loadvars_send(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	//sendAndLoad(url:String, target:Object, [method:String]) : Boolean
	void	as_loadvars_sendandload(const fn_call& fn)
	{
		if (fn.nargs < 2)
		{
			fn.result->set_bool(false);
			return;
		}

		const char * method = "POST";
		if( fn.nargs == 3 )
		{
			method = fn.arg(2).to_string();

			assert( strcmp(method, "POST") == 0 || strcmp(method,"GET") == 0 );
		}

		as_loadvars* loadvars = cast_to<as_loadvars>(fn.this_ptr);

		assert(loadvars);

		fn.result->set_bool( loadvars->send_and_load( fn.arg(0).to_string(), fn.arg(1).to_object(), method) );
	}
   
	//toString() : String
	void	as_loadvars_tostring(const fn_call& fn)
	{
		assert( 0 && "todo" );
	}

	as_loadvars::as_loadvars():
		m_iface(NULL),
		m_ns(NULL),
		m_target(NULL)
	{
		builtin_member( "addRequestHeader" , as_loadvars_addrequestheader );
		builtin_member( "decode" , as_loadvars_decode );
		builtin_member( "getBytesLoaded" , as_loadvars_getbytesloaded );
		builtin_member( "getBytesTotal" , as_loadvars_getbytestotal );
		builtin_member( "load" , as_loadvars_load );
		builtin_member( "send" , as_loadvars_send );
		builtin_member( "sendAndLoad" , as_loadvars_sendandload );
		builtin_member( "toString" , as_loadvars_tostring );

		m_iface = new net_interface_tcp();

		m_headers.set("Host", ""); //Force it to be in the first place
		m_headers.set("Content-Type", "application/x-www-form-urlencoded");
		m_headers.set("Cache-Control", "no-cache");
		m_headers.set("User-Agent", "gameswf");
		m_headers.set("Content-Length", "0");
	}

	bool	as_loadvars::send_and_load(const char * c_url, as_object * target, const tu_string & method)
	{
		char* url = strdup(c_url);

		// remove the http:// if it exists

		int start = 0;
		if( memcmp( url, "http://", 7 ) == 0 )
		{
			start = 7;
		}

		// get host name from url
		// find the first '/'
		int i, n;
		for (i = start, n = strlen(url); url[i] != '/' && i < n; i++);
		if (i == n)
		{
			// '/' is not found
			fprintf(stderr, "invalid url '%s'\n", url);
			free(url);
			return NULL;
		}

		tu_string uri = url + i;
		url[i] = 0;
		tu_string host = url + start;
		free(url);
		url = NULL;

		m_ns = m_iface->connect(host, 80);
		bool is_connected = m_ns ? true : false;

		if( !is_connected )
		{
			as_value function;
			if (m_target && m_target->get_member("onLoad", &function))
			{
				as_environment env;
				env.push(false);
				call_method(function, &env, m_target, 1, env.get_top_index());
			}
			return false;
		}

		m_target = target;
		get_root()->m_advance_listener.add(this);

		tu_string information;
		//create information in the form of name=value (urlencoded)
		string_hash<tu_string>::iterator it = m_values.begin();
		for(bool first = true; it != m_values.end(); ++it)
		{
			tu_string name, value;

			name = it->first;
			value = it->second;

			url_encode( &name );
			url_encode( &value );
			information += string_printf("%s%s= %s", first? "":"\r\n",name.c_str(), value.c_str() );
			first = false;
		}

		tu_string request = string_printf( "%s %s HTTP/1.1\r\n", method.c_str(), uri.c_str() );

		m_headers.set("Host", host);
		m_headers.set("Content-Length", string_printf( "%i", information.size()));
		// Process header
		for( string_hash<tu_string>::iterator it2 = m_headers.begin(); it2 != m_headers.end(); ++it2 )
		{
			request += string_printf( "%s: %s\r\n", it2->first.c_str(), it2->second.c_str() );
		}
		request += "\r\n";
		request += information;

		printf( request.c_str() );
		m_ns->write_string(request, 1);
		m_state = PARSE_REQUEST;

		return is_connected;
	}

	void	as_loadvars::add_header(const tu_string& name, const tu_string& value)
	{
		m_headers.set(name, value);
	}

	void	as_loadvars::advance(float delta_time)
	{
		assert(m_ns);

		tu_string str;

		while(m_ns->is_readable())
		{
			int byte_read = m_ns->read_line(&str, 100000, 0);

			if( byte_read == -1 || m_state == PARSE_END )
			{
				//Handle end of connection
				if( m_target )
				{
					as_value function;
					if (m_target->get_member("onHttpStatus", &function))
					{
						as_environment env;
						env.push(m_http_status);
						call_method(function, &env, m_target, 0, env.get_top_index());
					}

					if (m_target->get_member("onLoad", &function))
					{
						as_environment env;
						env.push(m_state != PARSE_END);
						call_method(function, &env, m_target, 1, env.get_top_index());
					}

					if (m_target->get_member("onData", &function))
					{
						as_environment env;
						env.push(""); //todo: gather raw data and provide it to the function
						call_method(function, &env, m_target, 1, env.get_top_index());
					}
				}

				get_root()->m_advance_listener.remove(this);
				return;
			}

			switch(m_state)
			{
				case PARSE_REQUEST:
					parse_request(str);
					break;

				case PARSE_HEADER:
					parse_header(str);
					break;

				case PARSE_CONTENT:
					parse_content(str);
					break;
			}

			str.clear();
		}
	}

	bool	as_loadvars::set_member(const tu_stringi& name, const as_value& val)
	{
		// todo: check for callbacks

		if( name == "onData"
			|| name == "onHTTPStatus"
			||name == "onLoad"
			)
		{
			return as_object::set_member( name, val );
		}

		m_values.set( name.to_tu_string(), val.to_tu_string() );

		return true;
	}

	bool	as_loadvars::get_member(const tu_stringi& name, as_value* val)
	{
		string_hash<tu_string>::iterator it = m_values.find( name.to_tu_string() );
		if( it != m_values.end() )
		{
			val->set_string( it->second );
			return true;
		}

		return as_object::get_member( name, val );
	}

	void	as_loadvars::parse_request(const tu_string& line)
	{
		assert(m_state == PARSE_REQUEST);
		
		// Method.
		const char* first_space = strchr(line.c_str(), ' ');
		if (first_space == NULL || first_space == line.c_str())
		{
			m_state = PARSE_END;
			return;
		}

		// URI.
		const char* second_space = strchr(first_space + 1, ' ');
		assert( second_space );
		m_http_status = atoi(first_space + 1);
		// todo use http status to trigger a onLoad(false)

		m_state = PARSE_HEADER;
	}

	void	as_loadvars::parse_header(const tu_string& line)
	{
		if( line == "\r\n" )
		{
			m_state = PARSE_CONTENT;
		}
	}

	void	as_loadvars::parse_content(const tu_string& line)
	{
		const char *after_name = strstr(line.c_str(), "=");

		if( after_name == NULL )
			return;

		tu_string name = tu_string(line.c_str(), after_name - line.c_str());
		tu_string value = tu_string( after_name + 1 ); //Skip the "= "

		url_decode( &name );
		url_decode( &value );

		if( m_target )
		{
			m_target->set_member(name, as_value(value));
		}
	}
};
