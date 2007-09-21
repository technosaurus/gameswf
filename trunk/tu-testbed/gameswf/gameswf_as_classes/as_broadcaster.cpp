// as_broadcaster.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Provides event notification and listener management capabilities that
// you can add to user-defined objects. This class is intended for advanced 
// users who want to create custom event handling mechanisms. 
// You can use this class to make any object an event broadcaster and 
// to create one or more listener objects that receive notification anytime 
// the broadcasting object calls the broadcastMessage() method. 

#include "gameswf/gameswf_as_classes/as_broadcaster.h"
#include "gameswf/gameswf_as_classes/as_array.h"
#include "gameswf/gameswf_function.h"

namespace gameswf
{
	// (listenerObj:Object) : Boolean
	void	as_broadcast_addlistener(const fn_call& fn)
	// Registers an object to receive event notification messages.
	// This method is called on the broadcasting object and
	// the listener object is sent as an argument.
	{
		assert(fn.this_ptr);
		as_value val;
		if (fn.this_ptr->get_member("_listeners", &val))
		{
			as_object_interface* obj = val.to_object();
			if (obj)
			{
				as_listener* asl = obj->cast_to_as_listener();
				if (asl)
				{
					as_object_interface* listener = fn.arg(0).to_object();
					if (listener)
					{
						asl->add(listener);
					}
				}
			}
		}
	}

	// (listenerObj:Object) : Boolean
	void	as_broadcast_removelistener(const fn_call& fn)
	// Removes an object from the list of objects that receive event notification messages. 
	{
		assert(fn.this_ptr);
		as_value val;
		if (fn.this_ptr->get_member("_listeners", &val))
		{
			as_object_interface* obj = val.to_object();
			if (obj)
			{
				as_listener* asl = obj->cast_to_as_listener();
				if (asl)
				{
					asl->remove(fn.arg(0).to_object());
				}
			}
		}
	}

	// (eventName:String, ...) : Void
	void	as_broadcast_sendmessage(const fn_call& fn)
	// Sends an event message to each object in the list of listeners.
	// When the message is received by the listening object,
	// gameswf attempts to invoke a function of the same name on the listening object. 
	{
		assert(fn.this_ptr);
		as_value val;
		if (fn.this_ptr->get_member("_listeners", &val))
		{
			as_object_interface* obj = val.to_object();
			if (obj)
			{
				as_listener* asl = obj->cast_to_as_listener();
				if (asl)
				{
					asl->broadcast(fn);
				}
			}
		}
	}

	// public static initialize(obj:Object) : Void
	// Adds event notification and listener management functionality to a given object.
	// This is a static method; it must be called by using the AsBroadcaster class
	void	as_broadcaster_initialize(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			as_object_interface* obj = fn.arg(0).to_object();
			if (obj)
			{
				obj->set_member("_listeners", new as_listener());
				obj->set_member("addListener", &as_broadcast_addlistener);
				obj->set_member("removeListener", &as_broadcast_removelistener);
				obj->set_member("broadcastMessage", &as_broadcast_sendmessage);
			}
		}
	}

	as_object* broadcaster_init()
	{
		as_object* bc = new as_object();

		// methods
		bc->set_member("initialize", &as_broadcaster_initialize);

		return bc;
	}

	bool	as_listener::get_member(const tu_stringi& name, as_value* val)
	{
		if (name == "length")
		{
			val->set_int(m_listener.size());
			return true;
		}
		
		int index = atoi(name.c_str());
		if (index >= 0 && index < m_listener.size())
		{
			val->set_as_object_interface(m_listener[index].get_ptr());
			return true;
		}
		return false;
	}

	void as_listener::add(as_object_interface* listener)
	{
		m_listener.push_back(listener);
	}

	void as_listener::remove(as_object_interface* listener)
	{
		for (int i = 0; i < m_listener.size(); )
		{
			if (m_listener[i].get_ptr() == listener)
			{
				m_listener.remove(i);
				continue;
			}
			i++;
		}
	}

	void	as_listener::broadcast(const fn_call& fn)
	{
		tu_string event_name = fn.arg(0).to_tu_string();
		for (int i = 0; i < m_listener.size(); )
		{
			// listener was deleted
			if (m_listener[i].get_ptr() == NULL)
			{
				m_listener.remove(i);
				continue;
			}

			as_value function;
			if (m_listener[i]->get_member(event_name, &function))
			{
				call_method(function, fn.env, m_listener[i].get_ptr(),
					fn.nargs - 1, fn.first_arg_bottom_index - 1);
			}
			i++;
		}
	}

};
