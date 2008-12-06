// tu_timer.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility/profiling timer.


#ifndef TU_TIMER_H
#define TU_TIMER_H

#include <time.h>
#include "base/tu_types.h"

namespace tu_timer
{

	// General-purpose wall-clock timer.  May not be hi-res enough
	// for profiling.
	exported_module uint64 get_ticks();
	exported_module double ticks_to_seconds(uint64 ticks);

	// current ticks to seconds
	exported_module double ticks_to_seconds();

	// Sleep the current thread for the given number of
	// milliseconds.  Don't rely on the sleep period being very
	// accurate.
	exported_module void sleep(int milliseconds);
	
	// Hi-res timer for CPU profiling.

	// Return a hi-res timer value.  Time 0 is arbitrary, so
	// generally you want to call this at the start and end of an
	// operation, and pass the difference to
	// profile_ticks_to_seconds() to find out how long the
	// operation took.
	exported_module uint64	get_profile_ticks();

	// Convert a hi-res ticks value into seconds.
	exported_module double	profile_ticks_to_seconds(uint64 profile_ticks);

	exported_module double	profile_ticks_to_milliseconds(uint64 ticks);

	// Returns the systime
	exported_module time_t get_systime();

	// Get/Set the day of the month (an integer from 1 to 31)
	exported_module int get_date(time_t t);
	exported_module void set_date(time_t* t, int day);

	// Get/Set the day of the week (0 for Sunday, 1 for Monday, and so on)
	exported_module int get_day(time_t t);
	exported_module void set_day(time_t* t, int day);

	// Returns the hour (an integer from 0 to 23)
	exported_module int get_hours(time_t t);
	exported_module void set_hours(time_t* t, int hours);

	// Get/Set the full year (a four-digit number, such as 2000)
	exported_module int get_fullyear(time_t t);
	exported_module void set_fullyear(time_t* t, int year);

	// Get/Set the year - 1900
	exported_module int get_year(time_t t);
	void set_year(time_t* t, int year);
	
	// Returns the milliseconds (an integer from 0 to 999)
	exported_module int get_milli(time_t t);

	// Get/Set the month (0 for January, 1 for February, and so on)
	exported_module int get_month(time_t t);
	exported_module void set_month(time_t* t, int month);

	// Get/Set the minutes (an integer from 0 to 59)
	exported_module int get_minutes(time_t t);
	exported_module void set_minutes(time_t* t, int hours);

	// Get/Set the seconds (an integer from 0 to 59)
	exported_module int get_seconds(time_t t);
	exported_module void set_seconds(time_t* t, int hours);

};


#endif // TU_TIMER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
