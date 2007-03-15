// as_sound.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "../gameswf_as_classes/as_sound.h"
#include "../gameswf_log.h"
#include "../gameswf_sound.h"
#include "../gameswf_movie_def.h"

namespace gameswf
{

	void	sound_start(const fn_call& fn)
	{
		IF_VERBOSE_ACTION(log_msg("-- start sound \n"));
		sound_handler* s = get_sound_handler();
		if (s != NULL)
		{
			as_sound*	so = (as_sound*) (as_object*) fn.this_ptr;
			assert(so);
			s->play_sound(so->sound_id, 0);
		}
	}


	void	sound_stop(const fn_call& fn)
	{
		IF_VERBOSE_ACTION(log_msg("-- stop sound \n"));
		sound_handler* s = get_sound_handler();
		if (s != NULL)
		{
			as_sound*	so = (as_sound*) (as_object*) fn.this_ptr;
			assert(so);
			s->stop_sound(so->sound_id);
		}
	}

	void	sound_attach(const fn_call& fn)
	{
		IF_VERBOSE_ACTION(log_msg("-- attach sound \n"));
		if (fn.nargs < 1)
		{
			log_error("attach sound needs one argument\n");
			return;
		}

		as_sound*	so = (as_sound*) (as_object*) fn.this_ptr;
		assert(so);

		so->sound = fn.arg(0).to_tu_string();

		// check the import.
		movie_definition_sub*	def = (movie_definition_sub*)
			fn.env->get_target()->get_root_movie()->get_movie_definition();
		assert(def);
		smart_ptr<resource> res = def->get_exported_resource(so->sound);
		if (res == NULL)
		{
			log_error("import error: resource '%s' is not exported\n", so->sound.c_str());
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
		so->sound_id = si;
	}

	void	as_global_sound_ctor(const fn_call& fn)
	// Constructor for ActionScript class Sound.
	{
		smart_ptr<as_object>	sound_obj(new as_sound);

		// methods
		sound_obj->set_member("attachSound", &sound_attach);
		sound_obj->set_member("start", &sound_start);
		sound_obj->set_member("stop", &sound_stop);

		fn.result->set_as_object_interface(sound_obj.get_ptr());
	}

}
