// as_sound.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_sound.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_sound.h"
#include "gameswf/gameswf_movie_def.h"

namespace gameswf
{

	void	sound_start(const fn_call& fn)
	{
		sound_handler* s = get_sound_handler();
		if (s != NULL)
		{
			assert(fn.this_ptr);
			as_sound*	snd = fn.this_ptr->cast_to_as_sound();
			if (snd)
			{
				int offset = 0;
				int loops = 0;
				if (fn.nargs >= 2)
				{
					offset = (int) fn.arg(0).to_number();
					loops = (int) fn.arg(1).to_number();
				}
				s->play_sound(snd->m_id, loops);
			}
		}
	}


	void	sound_stop(const fn_call& fn)
	{
		sound_handler* s = get_sound_handler();
		if (s != NULL)
		{
			assert(fn.this_ptr);
			as_sound*	snd = fn.this_ptr->cast_to_as_sound();
			assert(snd);
			s->stop_sound(snd->m_id);
		}
	}

	void	sound_attach(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			log_error("attach sound needs one argument\n");
			return;
		}

		assert(fn.this_ptr);
		as_sound*	snd = fn.this_ptr->cast_to_as_sound();
		assert(snd);

		snd->m_name = fn.arg(0).to_tu_string();

		assert(fn.env);

		// find target movieclip
		character* target = snd->m_target == NULL ? fn.env->get_target() : snd->m_target.get_ptr();
			
		// find resource
		resource* res = NULL;
		if (target)
		{
			res = target->find_exported_resource(snd->m_name);
		}

		if (res == NULL)
		{
			log_error("import error: resource '%s' is not exported\n", snd->m_name.c_str());
			return;
		}

		int si = 0;
		sound_sample_impl* ss = (sound_sample_impl*) res->cast_to_sound_sample();

		if (ss != NULL)
		{
			si = ss->m_sound_handler_id;
		}
		else
		{
			log_error("sound sample is NULL\n");
			return;
		}

		// sanity check
		assert(si >= 0 && si < 1000);
		snd->m_id = si;
	}

	void	sound_volume(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			log_error("set volume of sound needs one argument\n");
			return;
		}
		
		int volume = (int) fn.arg(0).to_number();

		// sanity check
		if (volume >= 0 && volume <= 100)
		{
			sound_handler* s = get_sound_handler();
			if (s != NULL)
			{
				assert(fn.this_ptr);
				as_sound*	snd = fn.this_ptr->cast_to_as_sound();
				assert(snd);
				s->set_volume(snd->m_id, volume);
			}
		}
	}

	// Sound([target:Object])
	//  Creates a new Sound object for a specified movie clip.
	void	as_global_sound_ctor(const fn_call& fn)
	{
		smart_ptr<as_sound> snd = new as_sound();
		if (fn.nargs > 0)
		{
			assert(fn.env);
			snd->m_target = fn.env->find_target(fn.arg(0));
		}

		// methods
		snd->set_member("attachSound", &sound_attach);
		snd->set_member("start", &sound_start);
		snd->set_member("stop", &sound_stop);
		snd->set_member("setVolume", &sound_volume);

		fn.result->set_as_object_interface(snd.get_ptr());
	}

}
