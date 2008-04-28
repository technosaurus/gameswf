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
    enum builtin_object
    {
        BUILTIN_OBJECT_METHOD,
        BUILTIN_SPRITE_METHOD,
        BUILTIN_NUMBER_METHOD,
        BUILTIN_BOOLEAN_METHOD,
        BUILTIN_STRING_METHOD,
        // and so far

        BUILTIN_COUNT
    };	

	as_value	get_property(as_object* obj, int prop_number);
	void	set_property(as_object* obj, int prop_number, const as_value& val);

	fscommand_callback	get_fscommand_callback();
	void	register_fscommand_callback(fscommand_callback handler);

	string_hash<tu_loadlib*>* get_shared_libs();
	void clear_shared_libs();

	struct player : public ref_counted
	{
		hash<smart_ptr<as_object>, bool> m_heap;
		smart_ptr<as_object>	m_global;
		Uint64 m_start_time;
		weak_ptr<root> m_current_root;
		tu_string m_workdir;        
        stringi_hash<as_value>*	m_standard_method_map[BUILTIN_COUNT];
        stringi_hash< smart_ptr<character_def> > m_chardef_library;

        // Players count to release all static stuff at the right time
        static int s_player_count;
        
		exported_module  player();
		exported_module  ~player();

		// external interface
		exported_module root* get_root();
		exported_module void set_root(root* m);
		exported_module void notify_key_event(key::code k, bool down);
		exported_module void verbose_action(bool val);
		exported_module void verbose_parse(bool val);
		exported_module void set_bootup_options(const tu_string& param);
		exported_module const char* get_workdir() const;
		exported_module void set_workdir(const char* dir);
	
		// Create/hook built-ins.
		exported_module void action_init();
	
		// Clean up any stray heap stuff we've allocated.
		exported_module void	action_clear();

        // standard method map, this stuff should be high optimized
        void clear_standard_method_map();
        bool get_builtin(builtin_object id, const tu_stringi& name, as_value* val) const;
        stringi_hash<as_value>* new_standard_method_map(builtin_object id);

        // library stuff, for sharing resources among different movies.
        stringi_hash< smart_ptr<character_def> >* get_chardef_library();
        void clear_library();

		as_object* get_global() const;
		void notify_key_object(key::code k, bool down);
		Uint64 get_start_time()	{ return m_start_time; }

		// the garbage manager
		void set_alive(as_object* obj);
		bool is_garbage(as_object* obj);
		void clear_heap();
		void set_as_garbage();
		void clear_garbage();
		

	};
}

#endif

