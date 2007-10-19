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
			mcl->add_listener(fn.arg(0));
			fn.result->set_bool(true);
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
			mcl->remove_listener(fn.arg(0));
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
			character* loading_movie = fn.env->load_file(fn.arg(0).to_string(), fn.arg(1));
			if (loading_movie != NULL)
			{
				loading_movie->set_mcloader(mcl);
				mcl->on_event(event_id(event_id::ONLOAD_START, loading_movie));
				fn.result->set_bool(true);
				return;
			}
			mcl->on_event(event_id(event_id::ONLOAD_ERROR, loading_movie));
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

	void as_mcloader::add_listener(as_value& listener)
	{
		m_listeners.add(listener.to_object());
	}

	void as_mcloader::clear_listener()
	{
		m_listeners.clear();
	}

	void as_mcloader::remove_listener(as_value& listener)
	{
		m_listeners.remove(listener.to_object());
	}

	bool	as_mcloader::on_event(const event_id& id)
	{
		as_environment env;
		int nargs = 1;
		switch (id.m_id)
		{
			case event_id::ONLOAD_START:
			case event_id::ONLOAD_INIT:
			case event_id::ONLOAD_COMPLETE:
				break;

			case event_id::ONLOAD_ERROR:
				nargs = 2;
				env.push("URLNotFound");	// 2-d param
				break;

			case event_id::ONLOAD_PROGRESS:
				{
					nargs = 3;	
					assert(id.m_target);
					sprite_instance* m = id.m_target->cast_to_sprite();

					// 8 is (file_start_pos(4 bytes) + header(4 bytes))
					int total = m->get_file_bytes() - 8;
					int loaded = m->get_loaded_bytes();
					env.push(total);
					env.push(loaded);
					break;
				}

			default:
				assert(0);

		}

		env.push(id.m_target);	// 1-st param

		m_listeners.notify(id.get_function_name(), 
			fn_call(NULL, NULL, &env, nargs, env.get_top_index()));

		return false;
	}

};
