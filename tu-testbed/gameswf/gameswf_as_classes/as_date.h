// as_date.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Date class lets you retrieve date and time values relative to universal time
// (Greenwich mean time, now called universal time or UTC) 
// or relative to the operating system on which Flash Player is running


#ifndef GAMESWF_AS_DATE_H
#define GAMESWF_AS_DATE_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "base/tu_timer.h"

namespace gameswf
{

	void	as_global_date_ctor(const fn_call& fn);

	struct as_date : public as_object
	{
		as_date(const fn_call& fn);
		virtual as_date* cast_to_as_date() { return this; }
		Uint64 get_time() const;

		private:

		Uint64 m_time;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_DATE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
