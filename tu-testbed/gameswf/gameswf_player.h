// gameswf.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameswf player implementation
// Here are collected all static variables,
// init_action() and clear_libray()


#ifndef GAMESWF_PLAYER_H
#define GAMESWF_PLAYER_H

#include "base/utility.h"
#include "base/tu_loadlib.h"
#include "gameswf/gameswf_object.h"

namespace gameswf
{

	struct heap;

	exported_module as_object* get_global();
	heap* get_heap();
	Uint64 get_start_time();

	stringi_hash< smart_ptr<character_def> >* get_movie_library();

	as_value	get_property(as_object* obj, int prop_number);
	void	set_property(as_object* obj, int prop_number, const as_value& val);

	fscommand_callback	get_fscommand_callback();
	void	register_fscommand_callback(fscommand_callback handler);

	string_hash<tu_loadlib*>* get_shared_libs();
	void clear_shared_libs();


	struct heap
	{
		hash<smart_ptr<as_object>, bool> m_heap;

		void clear();
		bool is_garbage(as_object* obj);
		void set(as_object* obj, bool garbage);
		void set_as_garbage();
		void clear_garbage();

	};

	struct gameswf_player : public ref_counted
	{

		exported_module  gameswf_player();
		exported_module  ~gameswf_player();

		exported_module void verbose_action(bool val);
		exported_module void verbose_parse(bool val);
		exported_module void set_bootup_options(const tu_string& param);
	
		// Create/hook built-ins.
		exported_module void action_init();
	
		// Clean up any stray heap stuff we've allocated.
		exported_module void	action_clear();

	};
}

#endif

