// gameswf_timers.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_timers.h"
#include "gameswf_root.h"

// hack: timer_id is't a number but the reference

namespace gameswf 
{

  void  as_global_setinterval(const fn_call& fn)
  {
		as_timer* timer = NULL;
		if (fn.nargs >= 3)
		{
			as_value func;
			if (fn.arg(0).get_type() == as_value::OBJECT)
			{
				fn.arg(0).to_object()->get_member(fn.arg(1).to_tu_string(), &func);
			}
			else
			{
				func = fn.arg(0);
			}

			if (func.get_type() == as_value::AS_FUNCTION)
			{
				timer = new as_timer(func, fn.arg(2).to_number());
			}
		}
		fn.result->set_as_object_interface(timer);
  }

  void  as_global_clearinterval(const fn_call& fn)
  {
		if (fn.nargs == 1)
		{
			if (fn.arg(0).to_object())
			{
				as_timer* timer = fn.arg(0).to_object()->cast_to_as_timer();
				if (timer)
				{
					timer->clear();
				}
			}
		}
	}

	as_timer::as_timer(as_value& func, double interval):
		m_interval((float) interval / 1000.0f),
		m_func(func),
		m_delta_time(0.0f)
	{
			get_root()->add_listener(this, movie_root::ADVANCE);
	}

	void as_timer::advance(float delta_time)
	{
		m_delta_time += delta_time;
		if (m_delta_time >= m_interval)
		{
			m_delta_time = 0.0f;

			assert(m_func.get_type() == as_value::AS_FUNCTION);

			as_environment* env = m_func.to_as_function()->m_env;
			assert(env);

	//		env->push(is_connected);
			call_method(m_func, env, NULL, 0, env->get_top_index());
	//		env->drop(1);
		}
	}

	void as_timer::clear()
	{
		get_root()->remove_listener(this);
	}

}
