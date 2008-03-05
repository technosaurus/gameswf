// as_geom.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_geom.h"
#include "gameswf/gameswf_as_classes/as_point.h"
#include "gameswf/gameswf_as_classes/as_matrix.h"

namespace gameswf
{
	//
	// geom object
	//

	as_object* geom_init()
	{
		// Create built-in geom object.
		as_object*	geom_obj = new as_object;

		// constant
		geom_obj->set_member("Point", as_value(as_global_point_ctor));
		geom_obj->set_member("Matrix", as_value(as_global_matrix_ctor));

		return geom_obj;
	}

}
