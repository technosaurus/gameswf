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
				as_array* a = obj->cast_to_as_array();
				if (a)
				{
					as_value& listener = fn.arg(0);
					if (listener.get_type() == as_value::OBJECT)
					{
						a->push_back(listener);
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
				as_array* a = obj->cast_to_as_array();
				if (a)
				{
					as_value& listener = fn.arg(0);
					for (int i = 0; i < a->size(); i++)
					{
						as_value index(i);
						as_value ilistener;
						a->get_member(index.to_tu_stringi(), &ilistener);
						if (ilistener == listener)
						{
							// Decrement array length 
							a->erase(index.to_tu_stringi());
						}
					}
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
				as_array* a = obj->cast_to_as_array();
				if (a)
				{
					tu_string event_name = fn.arg(0).to_tu_string();
					for (int i = 0; i < a->size(); i++)
					{
						as_value index(i);
						as_value val;
						a->get_member(index.to_tu_stringi(), &val);
						as_object_interface* listener = val.to_object();
						if (listener)
						{
							as_value function;
							if (listener->get_member(event_name, &function))
							{
								call_method(function, fn.env, listener,
									fn.nargs - 1, fn.first_arg_bottom_index - 1);
							}
						}
					}
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
				obj->set_member("_listeners", new as_array());
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

};
