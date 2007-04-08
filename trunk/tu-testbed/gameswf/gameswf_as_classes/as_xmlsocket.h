// as_xmlsocket.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script XMLSocket implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_XMLSOCKET_H
#define GAMESWF_AS_XMLSOCKET_H

#include "../gameswf_action.h"	// for as_object
#include "../../net/http_client.h"

namespace gameswf
{

	void	as_global_xmlsock_ctor(const fn_call& fn);

	struct as_xmlsock : public as_object
	{
		net_interface_tcp* m_iface;
		net_socket* m_ns;

		as_xmlsock();
		~as_xmlsock();

		virtual bool	on_event(const event_id& id);

		bool connect(const char* host, int port);
		void close();
		void send(const as_value& val) const;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
