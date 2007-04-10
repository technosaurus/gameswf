// as_mcloader.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_MCLOADER_H
#define GAMESWF_AS_MCLOADER_H

#include "../gameswf_action.h"	// for as_object
#include "../../net/http_client.h"

namespace gameswf
{

	void	as_global_mcloader_ctor(const fn_call& fn);

	struct as_mcloader : public as_object
	{
		hash< smart_ptr<as_object>, int > m_listener;

		as_mcloader();
		~as_mcloader();
		
		virtual bool	on_event(const event_id& id);

		bool add_listener(as_value& listener);
		bool remove_listener(as_value& listener);
		bool load_clip(const char* url, as_value& container);


	};

}	// end namespace gameswf


#endif // GAMESWF_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
