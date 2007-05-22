// as_netconnection.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_NETCONNECTION_H
#define GAMESWF_NETCONNECTION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf/gameswf_action.h"	// for as_object

namespace gameswf
{

	void	as_global_netconnection_ctor(const fn_call& fn);

	struct as_netconnection : public as_object
	{
		as_netconnection();
		virtual as_netconnection* cast_to_as_netconnection() { return this; }
	};

} // end of gameswf namespace

#endif

