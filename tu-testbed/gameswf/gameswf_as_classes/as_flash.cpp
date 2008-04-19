// as_flash.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_flash.h"
#include "gameswf/gameswf_as_classes/as_geom.h"

namespace gameswf
{
	//
	// flash object
	//

	as_object* flash_init(player* boss)
	{
		// Create built-in flash object.
		as_object*	flash_obj = new as_object(boss);

		// constant
		flash_obj->builtin_member("geom", geom_init(boss));

		return flash_obj;
	}

}
