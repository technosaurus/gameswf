// as_loadvars.h	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_AS_LOADVARS_H
#define GAMESWF_AS_LOADVARS_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_character.h"
#include "net/net_interface_tcp.h"

namespace gameswf
{

	void	as_global_loadvars_ctor(const fn_call& fn);

	enum loadvars_state
	{
		PARSE_REQUEST,
		PARSE_HEADER,
		PARSE_CONTENT,
		PARSE_END
	};

	struct as_loadvars : public as_object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_LOADVARS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		net_interface_tcp* m_iface;
		net_socket* m_ns;
		as_object* m_target;
		string_hash<tu_string> m_headers;
		string_hash<tu_string> m_values;
		loadvars_state m_state;
		int m_http_status;

		as_loadvars();
		bool	send_and_load(const char * url, as_object * target, const tu_string & method);
		void	add_header(const tu_string& name, const tu_string& value);
		void	advance(float delta_time);

		exported_module virtual bool	set_member(const tu_stringi& name, const as_value& val);
		exported_module virtual bool	get_member(const tu_stringi& name, as_value* val);

	private:

		void parse_request(const tu_string& line);
		void parse_header(const tu_string& line);
		void parse_content(const tu_string& line);
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_LOADVARS_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
