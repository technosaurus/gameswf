// as_date.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Date class lets you retrieve date and time values relative to universal time
// (Greenwich mean time, now called universal time or UTC) 
// or relative to the operating system on which Flash Player is running

#include "gameswf/gameswf_as_classes/as_date.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_function.h"

namespace gameswf
{

	void	as_global_date_ctor(const fn_call& fn)
	{
		gc_ptr<as_date>	obj = new as_date(fn);
		fn.result->set_as_object(obj.get_ptr());
	}

	// getTime() : Number
	// Returns the number of milliseconds since midnight January 1, 1970, universal time
	void	as_date_gettime(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_double(dt->get_fulltime());
	}

	// setTime(millisecond:Number) : Number
	// Sets the date for the specified Date object in milliseconds since midnight on January 1, 1970, 
	// and returns the new time in milliseconds.
	void	as_date_settime(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		dt->set_time(static_cast<time_t>(fn.arg(0).to_number() / 1000));
	}

	// getDate() : Number
	// Returns the day of the month (an integer from 1 to 31)
	void	as_date_getdate(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_date(dt->get_time()));
	}

	void	as_date_setdate(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_date(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getDay() : Number
	// Returns the day of the week (0 for Sunday, 1 for Monday, and so on)
	void	as_date_getday(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_day(dt->get_time()));
	}

	void	as_date_setday(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_day(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getFullYear() : Number
	// Returns the full year (a four-digit number, such as 2000)
	void	as_date_getfullyear(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_fullyear(dt->get_time()));
	}

	void	as_date_setfullyear(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_fullyear(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getHours() : Number
	// Returns the hour (an integer from 0 to 23)
	void	as_date_gethours(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_hours(dt->get_time()));
	}

	void	as_date_sethours(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_hours(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getMilliseconds() : Number
	// Returns the milliseconds (an integer from 0 to 999)
	void	as_date_getmilli(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_milli(dt->get_time()));
	}

	void	as_date_setmilli(const fn_call& fn)
	{
		// TODO
	}

	// getMinutes() : Number
	// Returns the minutes (an integer from 0 to 59)
 	void	as_date_getminutes(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_minutes(dt->get_time()));
	}

	void	as_date_setminutes(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_minutes(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getMonth() : Number
	// Returns the month (0 for January, 1 for February, and so on)
	void	as_date_getmonth(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_month(dt->get_time()));
	}

	// public setMonth(month:Number) : Number
	// Sets the month for the specified Date object in local time and returns the new time in milliseconds.
	void	as_date_setmonth(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_month(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}


	// getSeconds() : Number
	// Returns the seconds (an integer from 0 to 59)
	void	as_date_getseconds(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_seconds(dt->get_time()));
	}

	void	as_date_setseconds(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_seconds(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// getYear() : Number
	// Returns the year - 1900
	void	as_date_getyear(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);

		fn.result->set_int(tu_timer::get_year(dt->get_time()));
	}

	// public setYear(year:Number) : Number
	// Sets the year for the specified Date object in local time and returns the new time in milliseconds.
	void	as_date_setyear(const fn_call& fn)
	{
		as_date* dt = cast_to<as_date>(fn.this_ptr);
		assert(dt);
		if (fn.nargs > 0)
		{
			tu_timer::set_year(dt->get_time_ptr(), fn.arg(0).to_int());
			fn.result->set_double(dt->get_fulltime());
		}
	}

	// Date([yearOrTimevalue:Number], [month:Number], [date:Number],
	// [hour:Number], [minute:Number], [second:Number], [millisecond:Number])
	as_date::as_date(const fn_call& fn) :
		as_object(fn.get_player()),
		m_time(tu_timer::get_systime())
	{
		// predefined
		int year = tu_timer::get_fullyear(m_time);
		int month = tu_timer::get_month(m_time);
		int day = tu_timer::get_day(m_time);
		int hour = tu_timer::get_hours(m_time);
		int minute = tu_timer::get_minutes(m_time);
		int second = tu_timer::get_seconds(m_time);

		// reset if there are args
		if (fn.nargs > 0)
		{
			year = fn.arg(0).to_int();
			month = 0;
			day = 0;
			hour = 0;
			minute = 0;
			second = 0;
			if (fn.nargs > 1)
			{
				month = fn.arg(1).to_int();
				if (fn.nargs > 2)
				{
					day = fn.arg(2).to_int();
					if (fn.nargs > 3)
					{
						hour = fn.arg(3).to_int();
						if (fn.nargs > 4)
						{
							minute = fn.arg(4).to_int();
							if (fn.nargs > 5)
							{
								second = fn.arg(5).to_int();
							}
						}
					}
				}
			}
		}

		tu_timer::set_fullyear(&m_time, year);
		tu_timer::set_month(&m_time, month);
		tu_timer::set_date(&m_time, day);
		tu_timer::set_hours(&m_time, hour);
		tu_timer::set_minutes(&m_time, minute);
		tu_timer::set_seconds(&m_time, second);

		builtin_member("getTime", as_date_gettime);
		builtin_member("setTime", as_date_settime);

		builtin_member("getYear", as_date_getyear);
		builtin_member("setYear", as_date_setyear);

		builtin_member("getMonth", as_date_getmonth);
		builtin_member("setMonth", as_date_setmonth);

		builtin_member("getDate", as_date_getdate);
		builtin_member("setDate", as_date_setdate);

		builtin_member("getDay", as_date_getday);
		builtin_member("setDay", as_date_setday);

		builtin_member("getFullYear", as_date_getfullyear);
		builtin_member("setFullYear", as_date_setfullyear);

		builtin_member("getHours", as_date_gethours);
		builtin_member("setHours", as_date_sethours);

		builtin_member("getMilliseconds", as_date_getmilli);
		builtin_member("setMilliseconds", as_date_setmilli);

		builtin_member("getMinutes", as_date_getminutes);
		builtin_member("setMinutes", as_date_setminutes);

		builtin_member("getSeconds", as_date_getseconds);
		builtin_member("setSeconds", as_date_setseconds);

	}

		double as_date::get_fulltime() const
		{
			return (double) m_time * 1000;	// *1000 means that time in milliseconds
		}

		time_t as_date::get_time() const
		{
			return m_time;
		}

		void as_date::set_time(time_t t)
		{
			m_time = t;
		}

		time_t* as_date::get_time_ptr()
		{
			return &m_time;
		}
};
