// as_mcloader.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the gameswf SWF player library.

#include "gameswf/gameswf_as_classes/as_mcloader.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_sprite.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_function.h"
//#include "gameswf/gameswf_log.h"

namespace gameswf
{

	void	as_mcloader_addlistener(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_mcloader* mcl = fn.this_ptr->cast_to_as_mcloader();
		assert(mcl);

		if (fn.nargs == 1)
		{
			mcl->m_listeners.add(fn.arg(0).to_object());
			fn.result->set_bool(true);
			mcl->get_root()->m_advance_listener.add(mcl);
			return;
		}
		fn.result->set_bool(false);
	}

	void	as_mcloader_removelistener(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_mcloader* mcl = fn.this_ptr->cast_to_as_mcloader();
		assert(mcl);

		if (fn.nargs == 1)
		{
			mcl->m_listeners.remove(fn.arg(0).to_object());
			fn.result->set_bool(true);
			return;
		}
		fn.result->set_bool(false);
	}

	void	as_mcloader_loadclip(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_mcloader* mcl = fn.this_ptr->cast_to_as_mcloader();
		assert(mcl);

		if (fn.nargs == 2)
		{
			sprite_instance* target;
			character* ch = fn.env->load_file(fn.arg(0).to_string(), fn.arg(1),	&target, false);
			array<as_value> event_args;
			event_args.push_back(ch);
			if (ch != NULL)
			{
				as_mcloader::loadable_movie lm;
				lm.m_movie = ch->cast_to_sprite();
				assert(lm.m_movie != NULL);
				lm.m_target = target;
				mcl->m_movie.push_back(lm);

				mcl->m_listeners.notify(event_id(event_id::ONLOAD_START, &event_args));
				fn.result->set_bool(true);
				return;
			}
			event_args.push_back("URLNotFound");	// 2-d param
			mcl->m_listeners.notify(event_id(event_id::ONLOAD_ERROR, &event_args));
		}
		fn.result->set_bool(false);
	}

	void	as_mcloader_unloadclip(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.env->load_file("", fn.arg(0));
			fn.result->set_bool(true);
			return;
		}
		fn.result->set_bool(false);
	}

	void	as_mcloader_getprogress(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			as_object_interface* obj = fn.arg(0).to_object();
			if (obj)
			{
				sprite_instance* m = obj->cast_to_sprite();
				if (m)
				{
					as_object* info = new as_object();
					info->set_member("bytesLoaded", (int) m->get_loaded_bytes());
					info->set_member("bytesTotal", (int) m->get_file_bytes());
					fn.result->set_as_object_interface(info);
					return;
				}
			}
		}
		fn.result->set_as_object_interface(NULL);
	}

	void	as_global_mcloader_ctor(const fn_call& fn)
	// Constructor for ActionScript class MovieClipLoader
	{
		fn.result->set_as_object_interface(new as_mcloader);
	}

	as_mcloader::as_mcloader()
	{
		set_member("addListener", &as_mcloader_addlistener);
		set_member("removeListener", &as_mcloader_removelistener);
		set_member("loadClip", &as_mcloader_loadclip);
		set_member("unloadClip", &as_mcloader_unloadclip);
		set_member("getProgress", &as_mcloader_getprogress);
	}

	as_mcloader::~as_mcloader()
	{
	}
	
	void	as_mcloader::advance(float delta_time)
	{
		if (m_movie.size() == 0)
		{
			get_root()->m_advance_listener.remove(this);
			return;
		}

		for (int i = 0; i < m_movie.size();)
		{

			sprite_instance* ch = m_movie[i].m_movie.get_ptr();
			assert(ch);

			array<as_value> event_args;
			event_args.push_back(ch);


			int nframe = m_movie[i].m_movie->get_loading_frame();
			if (nframe == 1 && m_movie[i].m_init_event_issued == false)
			{
				place_instance(ch, m_movie[i].m_target.get_ptr());
				m_listeners.notify(event_id(event_id::ONLOAD_INIT, &event_args));
			}

			int loaded = ch->get_loaded_bytes();
			int total = ch->get_file_bytes();

			if (loaded < total)
			{
				event_args.push_back(loaded);
				event_args.push_back(total);
				m_listeners.notify(event_id(event_id::ONLOAD_PROGRESS, &event_args));
			}
			else
			{
				event_args.push_back(loaded);
				event_args.push_back(total);
				m_listeners.notify(event_id(event_id::ONLOAD_PROGRESS, &event_args));
				m_listeners.notify(event_id(event_id::ONLOAD_COMPLETE, &event_args));

				m_movie.remove(i);
				continue;
			}

			i++;

		}
	}

	void	as_mcloader::place_instance(sprite_instance* ch, sprite_instance* target)
	{
		if (target)
		{
			if (target == target->get_root_movie())
			{
				// mcLoader can't replace 
				ch->execute_frame_tags(0);
				set_current_root(ch->get_root());
				ch->on_event(event_id::LOAD);
			}
			else
			{
				// container
				const char* name = target->get_name();
				Uint16 depth = target->get_depth();
				bool use_cxform = false;
				cxform color_transform = target->get_cxform();
				bool use_matrix = false;
				const matrix& mat = target->get_matrix();
				float ratio = target->get_ratio();
				Uint16 clip_depth = target->get_clip_depth();

				ch->get_parent()->replace_display_object(
					ch,
					name,
					depth,
					use_cxform,
					color_transform,
					use_matrix,
					mat,
					ratio,
					clip_depth);
			}
		}
	}

};
