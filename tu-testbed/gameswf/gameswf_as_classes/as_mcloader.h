// as_mcloader.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_MCLOADER_H
#define GAMESWF_AS_MCLOADER_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_listener.h"	// for listener
#include "net/http_client.h"

namespace gameswf
{

	void	as_global_mcloader_ctor(const fn_call& fn);

	struct as_mcloader : public as_object
	{
		struct loadable_movie
		{
			loadable_movie() :
				m_init_event_issued(false)
			{
			}

			smart_ptr<sprite_instance> m_movie;
			weak_ptr<sprite_instance> m_target;
			bool m_init_event_issued;
		};

		listener m_listeners;
		array<loadable_movie> m_movie;

		as_mcloader();
		~as_mcloader();
		
		virtual void	advance(float delta_time);
		virtual as_mcloader* cast_to_as_mcloader() { return this; }

	};

}	// end namespace gameswf


#endif // GAMESWF_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
