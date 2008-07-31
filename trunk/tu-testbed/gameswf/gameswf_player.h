// gameswf.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameswf player implementation
// Here are collected all static variables,
// init_action() and clear_libray()


#ifndef player_H
#define player_H

#include "base/utility.h"
#include "base/tu_loadlib.h"
#include "gameswf/gameswf_object.h"

namespace gameswf
{
	as_value	get_property(as_object* obj, int prop_number);
	void	set_property(as_object* obj, int prop_number, const as_value& val);

	fscommand_callback	get_fscommand_callback();
	void	register_fscommand_callback(fscommand_callback handler);

	string_hash<tu_loadlib*>* get_shared_libs();
	void clear_shared_libs();

	// for setInterval and setTimeout
	struct timer : public ref_counted
	{
		weak_ptr<as_object> m_this_ptr;
		weak_ptr<as_function> m_func;
		float m_interval;	// sec
		float m_time_remainder;
		array<as_value> m_arg;
		bool m_do_once;

		timer() :
			m_interval(0.0f),
			m_time_remainder(0.0f),
			m_do_once(false)
		{
		}

		void advance(float delta_time);
	};

	struct player : public ref_counted
	{
		hash<gc_ptr<as_object>, bool> m_heap;
		gc_ptr<as_object>	m_global;
		Uint64 m_start_time;
		weak_ptr<root> m_current_root;
		tu_string m_workdir;
		stringi_hash<gc_ptr<character_def> > m_chardef_library;
		tu_string m_flash_vars;
		bool m_force_realtime_framerate;

		// it's used to watch texture memory
		bool m_log_bitmap_info;

		// Players count to release all static stuff at the right time
		static int s_player_count;

		// timers, for setInterval and setTimeout global functions
		array< gc_ptr<timer> > m_timer;
		
		exported_module  player();
		exported_module  ~player();

		// external interface
		exported_module root* get_root();
		exported_module void set_root(root* m);
		exported_module void notify_key_event(key::code k, bool down);
		exported_module void verbose_action(bool val);
		exported_module void verbose_parse(bool val);
		exported_module void set_flash_vars(const tu_string& param);
		exported_module const char* get_workdir() const;
		exported_module void set_workdir(const char* dir);
	
		// @@ Hm, need to think about these creation API's.  Perhaps
		// divide it into "low level" and "high level" calls.  Also,
		// perhaps we need a "context" object that contains all
		// global-ish flags, libraries, callback pointers, font
		// library, etc.
		//
		// Create a gameswf::movie_definition from the given file name.
		// Normally, will also try to load any cached data file
		// (".gsc") that corresponds to the given movie file.  This
		// will still work even if there is no cache file.  You can
		// disable the attempts to load cache files by calling
		// gameswf::use_cache_files(false).
		//
		// Uses the registered file-opener callback to read the files
		// themselves.
		//
		// This calls add_ref() on the newly created definition; call
		// drop_ref() when you're done with it.
		// Or use smart_ptr<T> from base/smart_ptr.h if you want.
		exported_module movie_definition*	create_movie(const char* filename);
		exported_module	gc_ptr<root>  load_file(const char* filename);


		// Create/hook built-ins.
		exported_module void action_init();
	
		// Clean up any stray heap stuff we've allocated.
		exported_module void	action_clear();

		// library stuff, for sharing resources among different movies.
		stringi_hash<gc_ptr<character_def> >* get_chardef_library();
		void clear_library();

		as_object* get_global() const;
		void notify_key_object(key::code k, bool down);
		Uint64 get_start_time()	{ return m_start_time; }

		exported_module const bool get_force_realtime_framerate() const;
		exported_module void set_force_realtime_framerate(const bool force_realtime_framerate);
		exported_module bool get_log_bitmap_info() const { return m_log_bitmap_info; }
		exported_module void set_log_bitmap_info(bool log_bitmap_info) { m_log_bitmap_info = log_bitmap_info; }

		// timing
		int	create_timer();
		timer*	get_timer(int timer_id);
		exported_module void	advance_timer(float delta_time);

		// the garbage manager
		void set_alive(as_object* obj);
		bool is_garbage(as_object* obj);
		void clear_heap();
		void set_as_garbage();
		void clear_garbage();
		

	};
}

#endif

