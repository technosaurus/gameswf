// as_netconnection.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf/gameswf_as_classes/as_netconnection.h"

namespace gameswf
{

	void	as_global_netconnection_ctor(const fn_call& fn)
	// Constructor for ActionScript class NetConnection.
	{
		fn.result->set_as_object_interface(new as_netconnection);
	}

	void	as_netconnection_connect(const fn_call& fn)
	{
		// Opens a local connection through which you can play back video files
		// from an HTTP address or from the local file system.
		assert(fn.this_ptr);
		as_netconnection* nc = fn.this_ptr->cast_to_as_netconnection();
		assert(nc);

		if (fn.nargs == 1)
		{
			assert(fn.env);
			
			if (fn.arg(0).get_type() == as_value::NULLTYPE)
			// local file system
			{
				fn.result->set_bool(true);
				return;
			}
			else
			// from an HTTP address
			{
				//todo
			}
		}

		fn.result->set_bool(false);
		return;
	}

	as_netconnection::as_netconnection()
	{
		set_member("connect", &as_netconnection_connect);
	}


} // end of gameswf namespace
