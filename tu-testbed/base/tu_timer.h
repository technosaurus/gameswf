// tu_timer.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility/profiling timer.


#ifndef TU_TIMER_H


#include "base/tu_types.h"


namespace tu_timer
{
	uint64	get_profile_ticks();

	double	profile_ticks_to_seconds(uint64 profile_ticks);
};


#endif // TU_TIMER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
