// tu_timer.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility/profiling timer.


#include <time.h>	// [ANSI/System V]
#include "base/tu_timer.h"


time_t tu_timer::get_systime()
// Returns the time as seconds elapsed since midnight, January 1, 1970.
{
	time_t ltime;
	time(&ltime);
	return ltime;
}

int tu_timer::get_date(time_t t)
// Returns the day of the month (an integer from 1 to 31)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_mday : -1;
}

void tu_timer::set_date(time_t* t, int day)
// Sets the day of the month (an integer from 1 to 31)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_mday = day;
		*t = mktime(gmt);
	}
}


int tu_timer::get_day(time_t t)
// Returns the day of the week (0 for Sunday, 1 for Monday, and so on)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_wday : -1;
}

void tu_timer::set_day(time_t* t, int day)
// Sets the day of the week (0 for Sunday, 1 for Monday, and so on)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_wday = day;
		*t = mktime(gmt);
	}
}

int tu_timer::get_hours(time_t t)
// Returns the hour (an integer from 0 to 23)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_hour : -1;
}

void tu_timer::set_hours(time_t* t, int hours)
// Sets the hour (an integer from 0 to 23)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_hour = hours;
		*t = mktime(gmt);
	}
}

int tu_timer::get_fullyear(time_t t)
// Returns the full year (a four-digit number, such as 2000)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_year + 1900 : -1;
}

void tu_timer::set_fullyear(time_t* t, int year)
// Sets the full year (a four-digit number, such as 2000)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_year = year - 1900;	// (current year minus 1900).
		*t = mktime(gmt);
	}
}

int tu_timer::get_year(time_t t)
// Returns year minus 1900
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_year : -1;
}

void tu_timer::set_year(time_t* t, int year)
// year minus 1900
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_year = year;
		*t = mktime(gmt);
	}
}

int tu_timer::get_milli(time_t t)
// Returns the milliseconds (an integer from 0 to 999)
{
	return 0;	// TODO
}

int tu_timer::get_month(time_t t)
// Returns the month (0 for January, 1 for February, and so on)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_mon : -1;
}

void tu_timer::set_month(time_t* t, int month)
// Sets the month (0 for January, 1 for February, and so on)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_mon = month;
		*t = mktime(gmt);
	}
}

int tu_timer::get_minutes(time_t t)
// Returns the minutes (an integer from 0 to 59)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_min : -1;
}

void tu_timer::set_minutes(time_t* t, int minutes)
// Sets the minutes (an integer from 0 to 59)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_min = minutes;
		*t = mktime(gmt);
	}
}

int tu_timer::get_seconds(time_t t)
// Returns the seconds (an integer from 0 to 59)
{
	struct tm* gmt = localtime(&t);
	return gmt ? gmt->tm_sec : -1;
}

void tu_timer::set_seconds(time_t* t, int seconds)
// Sets the seconds (an integer from 0 to 59)
{
	struct tm* gmt = localtime(t);
	if (gmt)
	{
		gmt->tm_sec = seconds;
		*t = mktime(gmt);
	}
}

#ifdef _WIN32

#include <windows.h>


uint64 tu_timer::get_ticks()
{
	return timeGetTime();
}


double tu_timer::ticks_to_seconds(uint64 ticks)
{
	return ticks * (1.0f / 1000.f);
}

double tu_timer::ticks_to_seconds()
{
	return get_ticks() * (1.0f / 1000.f);
}


void tu_timer::sleep(int milliseconds)
{
	::Sleep(milliseconds);
}


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
#include <unistd.h>


// The profile ticks implementation is just fine for a normal timer.


uint64 tu_timer::get_ticks()
{
	return (uint64) profile_ticks_to_milliseconds(get_profile_ticks());
}


double tu_timer::ticks_to_seconds(uint64 ticks)
{
	return profile_ticks_to_seconds(ticks);
}


void tu_timer::sleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}


uint64	tu_timer::get_profile_ticks()
{
	// @@ TODO prefer rdtsc when available?

	// Return microseconds.
	struct timeval tv;
	uint64 result;
	
	gettimeofday(&tv, 0);

	result = tv.tv_sec * 1000000;
	result += tv.tv_usec;
	
	return result;
}


double	tu_timer::profile_ticks_to_seconds(uint64 ticks)
{
	// ticks is microseconds.  Convert to seconds.
	return ticks / 1000000.0;
}

double	tu_timer::profile_ticks_to_milliseconds(uint64 ticks)
{
	// ticks is microseconds.  Convert to milliseconds.
	return ticks / 1000.0;
}

#endif	// not _WIN32


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
