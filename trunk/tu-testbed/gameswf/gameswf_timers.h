// gameswf_timers.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_TIMER_H
#define GAMESWF_TIMER_H

#include "gameswf_action.h"	// for as_object

namespace gameswf 
{
  void  as_global_setinterval(const fn_call& fn);
  void  as_global_clearinterval(const fn_call& fn);

	struct as_timer : public as_object
	{
		float m_interval;	// sec
		as_value m_func;
		float m_delta_time;

		as_timer(as_value& func, double intarval);

		virtual void advance(float delta_time);
		virtual as_timer* cast_to_as_timer() { return this; }

		void clear();
	};


}

#endif
