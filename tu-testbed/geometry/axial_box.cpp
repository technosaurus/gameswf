// axial_box.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Simple AABB structure


#include "geometry/axial_box.h"
#include "base/tu_random.h"
#include "base/utility.h"


vec3	axial_box::get_random_point() const
// Return a random point inside this box.
{
	return vec3(
		flerp(m_min[0], m_max[0], tu_random::get_unit_float()),
		flerp(m_min[1], m_max[1], tu_random::get_unit_float()),
		flerp(m_min[2], m_max[2], tu_random::get_unit_float()));
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
