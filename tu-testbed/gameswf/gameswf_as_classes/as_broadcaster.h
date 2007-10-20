// as_broadcaster.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script AsBroadcaster implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_BROADCASTER_H
#define GAMESWF_AS_BROADCASTER_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_listener.h"
#include "base/tu_queue.h"

namespace gameswf
{

	// Create built-in broadcaster object.
	as_object* broadcaster_init();

	struct as_listener : public as_object
	{
		as_listener();
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual as_listener* cast_to_as_listener() { return this; }
		void add(as_object_interface* listener);
		void remove(as_object_interface* listener);
		void	broadcast(const fn_call& fn);
		void	notify(const fn_call& fn);
	
		private :

		listener m_listeners;
		bool m_reentrance;
		tu_queue< array <as_value>* > m_event;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_BROADCASTER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
