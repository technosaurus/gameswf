// tu_timer.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility/profiling timer.


#include "base/tu_timer.h"


#ifdef _WIN32

#include <windows.h>


uint64	tu_timer::get_profile_ticks()
{
	// @@ use rdtsc?

	LARGE_INTEGER	li;
	QueryPerformanceCounter(&li);

	return li.QuadPart;
}


double	tu_timer::profile_ticks_to_seconds(uint64 ticks)
{
	LARGE_INTEGER	freq;
	QueryPerformanceFrequency(&freq);

	double	seconds = (double) ticks;
	seconds /= (double) freq.QuadPart;

	return seconds;
}


#else	// not _WIN32


// TODO implement Linux profiling timer


#endif	// not _WIN32


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
