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

#include <sys/time.h>

// On Linux all data is in seconds and milliseconds, so this is useless. We do it
// anyway to stay Windows compatible. Since the result is always passed to
// profile_ticks_to_seconds, we don't really need to do anything here.
uint64	tu_timer::get_profile_ticks()
{
	struct timeval tv;
	struct timezone tz;
	double result;
	
	gettimeofday(&tv, &tz);

	result = tv.tv_sec * 1.0;
	result += tv.tv_usec * 0.000001;
	// printf("%s: %ld + %ld %f\n", __PRETTY_FUNCTION__, tv.tv_sec, tv.tv_usec, result);
	
	return result;
}

// On Linux there is no conversion required, as all data is in seconds and milliseconds
double	tu_timer::profile_ticks_to_seconds(uint64 ticks)
{
#if 1
	struct timeval tv;
	struct timezone tz;
	double result;
	
	gettimeofday(&tv, &tz);

	result = tv.tv_sec * 1.0;
	result += tv.tv_usec * 0.000001;
	// printf("%s: %ld + %ld %f\n", __PRETTY_FUNCTION__, tv.tv_sec, tv.tv_usec, result);
	
	return result;
#else
	return ticks;
#endif
}

#endif	// not _WIN32


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
