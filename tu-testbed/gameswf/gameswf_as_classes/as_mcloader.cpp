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
			fn.result->set_bool(mcl->add_listener(fn.arg(0)));
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
			fn.result->set_bool(mcl->remove_listener(fn.arg(0)));
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

	bool as_mcloader::add_listener(as_value& listener)
	{
		if (listener.to_object())
		{
			m_listener[listener.to_object()] = 0;
			return true;
		}
		return false;
	}

	void as_mcloader::clear_listener()
	{
		m_listener.clear();
	}

	bool as_mcloader::remove_listener(as_value& listener)
	{
		if (listener.to_object())
		{
			m_listener.erase(listener.to_object());
			return true;
		}
		return false;
	}

	bool	as_mcloader::on_event(const event_id& id)
	{
		for (hash< weak_ptr<as_object_interface>, int >::iterator it = m_listener.begin(); it != m_listener.end(); )
		{
			as_value function;

			smart_ptr<as_object_interface> listener = it->first;
			if (listener == NULL)
			{
				// cleanup the garbage
				m_listener.erase(it);
				continue;
			}

			if (listener->get_member(id.get_function_name(), &function))
			{
				as_environment env;
				int param_count = 1;
				switch (id.m_id)
				{
					case event_id::ONLOAD_START:
					case event_id::ONLOAD_INIT:
					case event_id::ONLOAD_COMPLETE:
						break;

					case event_id::ONLOAD_ERROR:
						param_count = 2;
						env.push("URLNotFound");	// 2-d param
						break;

					case event_id::ONLOAD_PROGRESS:
					{
						param_count = 3;	
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
				call_method(function, &env, NULL, param_count, env.get_top_index());
			}
			++it;
		}
		return false;
	}

};
