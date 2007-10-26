// gameswf_listener.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_LISTENER_H
#define GAMESWF_LISTENER_H

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_function.h"
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"

namespace gameswf
{

	struct listener
	{
		array< weak_ptr<as_object_interface> > m_listeners;

		exported_module void add(as_object_interface* listener);
		exported_module void remove(as_object_interface* listener);

		exported_module void notify(const event_id& ev);
		exported_module void notify(const tu_string& event_name, const fn_call& fn);
		exported_module void advance(float delta_time);

		exported_module void clear_garbage();

		exported_module int size() const { return m_listeners.size(); }
		exported_module void clear() { m_listeners.clear(); }
		exported_module as_object_interface*	operator[](const tu_stringi& name) const;
	};

}

#endif
