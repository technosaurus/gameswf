// gameswf_listener.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_listener.h"

namespace gameswf
{

	// for Key, keyPress, ...
	void listener::notify(const event_id& ev)
	{
		// may be called from multithread plugin ==>
		// we should check current root
		if (get_current_root() == NULL)
		{
			return;
		}

		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		array< weak_ptr<as_object_interface> > listeners;
		listeners = m_listeners;
		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			smart_ptr<as_object_interface> obj = listeners[i];
			if (obj != NULL)
			{
				obj->on_event(ev);
			}
		}
		clear_garbage();
	}

	// for asBroadcaster, ...
	void listener::notify(const tu_string& event_name, const fn_call& fn)
	{
		// may be called from multithread plugin ==>
		// we should check current root
		if (get_current_root() == NULL)
		{
			return;
		}

		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		array< weak_ptr<as_object_interface> > listeners;
		listeners = m_listeners;
		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			smart_ptr<as_object_interface> obj = listeners[i];
			if (obj != NULL)	// is listener destroyed ?
			{
				as_value function;
				if (obj->get_member(event_name, &function))
				{
					call_method(function, fn.env, obj.get_ptr(),
						fn.nargs, fn.env->get_top_index());
				}
			}
		}
		clear_garbage();
	}

	// for video, timer, ...
	void	listener::advance(float delta_time)
	{
		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		array< weak_ptr<as_object_interface> > listeners;
		listeners = m_listeners;
		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			smart_ptr<as_object_interface> obj = listeners[i];
			if (obj != NULL)
			{
				obj->advance(delta_time);
			}
		}
		clear_garbage();
	}

	void listener::add(as_object_interface* listener) 
	{
		clear_garbage();

		// sanity check
		assert(m_listeners.size() < 1000);
//		printf("m_listeners size=%d\n", m_listeners.size());

		if (listener)
		{
			for (int i = 0, n = m_listeners.size(); i < n; i++)
			{
				smart_ptr<as_object_interface> obj = m_listeners[i];
				if (obj == listener)
				{
					return;
				}
			}
			m_listeners.push_back(listener);
		}
	} 

	void listener::remove(as_object_interface* listener) 
	{
		// to null out but to not delete since 'remove' may be called
		// from notify and consequently the size of 'm_listeners' cannot be changed	

		for (int i = 0, n = m_listeners.size(); i < n; i++)
		{
			smart_ptr<as_object_interface> obj = m_listeners[i];
			if (obj == listener)
			{
				m_listeners[i] = NULL;
			}
		}
	} 

	void listener::clear_garbage()
	{
		int i = 0;
		while (i < m_listeners.size())
		{
			smart_ptr<as_object_interface> obj = m_listeners[i];
			if (obj == NULL)	// listener was destroyed
			{
				// cleanup the garbage
				m_listeners.remove(i);
				continue;
			}
			i++;
		}
	}

	as_object_interface*	listener::operator[](const tu_stringi& name) const
	{
		int index = atoi(name.c_str());
		if (index >= 0 && index < m_listeners.size())
		{
			return m_listeners[index].get_ptr();
		}
		return NULL;
	}
}
