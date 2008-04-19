// gameswf.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// here are collected all statics of gameswf library and also
// init_library and clear_library implementation

#include "base/tu_timer.h"
#include "gameswf/gameswf_player.h"
#include "gameswf/gameswf_object.h"
#include "gameswf/gameswf_action.h"

// action script classes
#include "gameswf/gameswf_as_sprite.h"
#include "gameswf/gameswf_as_classes/as_array.h"
#include "gameswf/gameswf_as_classes/as_sound.h"
#include "gameswf/gameswf_as_classes/as_key.h"
#include "gameswf/gameswf_as_classes/as_math.h"
#include "gameswf/gameswf_as_classes/as_mcloader.h"
#include "gameswf/gameswf_as_classes/as_netstream.h"
#include "gameswf/gameswf_as_classes/as_netconnection.h"
#include "gameswf/gameswf_as_classes/as_textformat.h"
#include "gameswf/gameswf_as_classes/as_string.h"
#include "gameswf/gameswf_as_classes/as_color.h"
#include "gameswf/gameswf_as_classes/as_date.h"
#include "gameswf/gameswf_as_classes/as_xmlsocket.h"
#include "gameswf/gameswf_as_classes/as_loadvars.h"
#include "gameswf/gameswf_as_classes/as_flash.h"
#include "gameswf/gameswf_as_classes/as_broadcaster.h"
#include "gameswf/gameswf_as_classes/as_selection.h"
#include "gameswf/gameswf_as_classes/as_number.h"
#include "gameswf/gameswf_as_classes/as_boolean.h"
#include "gameswf/gameswf_as_classes/as_global.h"

namespace gameswf
{

	//
	//	gameswf's statics
	//

	// hack, temporary to avoid memory leak on exit from gameswf
//	smart_ptr<gameswf_player>	s_current_player;
	gameswf_player*	s_current_player = NULL;

	gameswf_player * get_current_player()
	{
		return s_current_player;
	}

	void set_current_player(gameswf_player* p)
	{
		s_current_player = p;
	}
	
	as_object* get_global()
	{
		return s_current_player->m_global.get_ptr();
	}

	heap* get_heap() { return &s_current_player->m_heap; }

	Uint64 get_start_time()
	{
		return s_current_player->m_start_time;
	}

	// standard method map, this stuff should be high optimized

	static stringi_hash<as_value>*	s_standard_method_map[BUILTIN_COUNT];
	void clear_standard_method_map()
	{
		for (int i = 0; i < BUILTIN_COUNT; i++)
		{
			if (s_standard_method_map[i])
			{
				delete s_standard_method_map[i];
			}
		}
	}

	bool get_builtin(builtin_object id, const tu_stringi& name, as_value* val)
	{
		if (s_standard_method_map[id])
		{
			return s_standard_method_map[id]->get(name, val);
		}
		return false;
	}

	stringi_hash<as_value>* new_standard_method_map(builtin_object id)
	{
		if (s_standard_method_map[id] == NULL)
		{
			s_standard_method_map[id] = new stringi_hash<as_value>;
		}
		return s_standard_method_map[id];
	}

	// Standard property lookup.

	static stringi_hash<as_standard_member>	s_standard_property_map;
	void clear_standard_property_map()
	{
		s_standard_property_map.clear();
	}

#ifdef WIN32
	static tu_string s_gameswf_version("WIN");
#else
	static tu_string s_gameswf_version("LINUX");
#endif

	const char* get_gameswf_version() {	return s_gameswf_version.c_str(); }

	// dynamic library stuff, for sharing DLL/shared library among different movies.

	static string_hash<tu_loadlib*> s_shared_libs;

	string_hash<tu_loadlib*>* get_shared_libs()
	{
		return &s_shared_libs;
	}

	void clear_shared_libs()
	{
		for (string_hash<tu_loadlib*>::iterator it = s_shared_libs.begin();
			it != s_shared_libs.end(); ++it)
		{
			delete it->second;
		}
		s_shared_libs.clear();
	}

	// External interface.

	static fscommand_callback	s_fscommand_handler = NULL;

	fscommand_callback	get_fscommand_callback()
	{
		return s_fscommand_handler;
	}

	void	register_fscommand_callback(fscommand_callback handler)
	{
		s_fscommand_handler = handler;
	}


	// library stuff, for sharing resources among different movies.

	static stringi_hash< smart_ptr<character_def> >	s_chardef_library;
	
	static tu_string s_workdir;

	stringi_hash< smart_ptr<character_def> >* get_chardef_library()
	{
		return &s_chardef_library;
	}

	root* get_current_root()
	{
		assert(s_current_player->m_current_root.get_ptr() != NULL);
		return s_current_player->m_current_root.get_ptr();
	}

	void set_current_root(root* m)
	{
		assert(m != NULL);
		s_current_player->m_current_root = m;
	}

	const char* get_workdir()
	{
		return s_workdir.c_str();
	}

	void set_workdir(const char* dir)
	{
		assert(dir != NULL);
		s_workdir = dir;
	}

	void	clear_library()
	// Drop all library references to movie_definitions, so they
	// can be cleaned up.
	{
		for (stringi_hash< smart_ptr<character_def> >::iterator it = 
			s_chardef_library.begin(); it != s_chardef_library.end(); ++it)
		{
			if (it->second->get_ref_count() > 1)
			{
				printf("memory leaks is found out: on exit movie_definition_sub ref_count > 1\n");
				printf("this = 0x%p, ref_count = %d\n", it->second.get_ptr(),
					it->second->get_ref_count());

				// to detect memory leaks
				while (it->second->get_ref_count() > 1)	it->second->drop_ref();
			}
		}
		s_chardef_library.clear();
		s_workdir.clear();
	}



	as_standard_member	get_standard_member(const tu_stringi& name)
	{
		if (s_standard_property_map.size() == 0)
		{
			s_standard_property_map.set_capacity(int(AS_STANDARD_MEMBER_COUNT));

			s_standard_property_map.add("_x", M_X);
			s_standard_property_map.add("_y", M_Y);
			s_standard_property_map.add("_xscale", M_XSCALE);
			s_standard_property_map.add("_yscale", M_YSCALE);
			s_standard_property_map.add("_currentframe", M_CURRENTFRAME);
			s_standard_property_map.add("_totalframes", M_TOTALFRAMES);
			s_standard_property_map.add("_alpha", M_ALPHA);
			s_standard_property_map.add("_visible", M_VISIBLE);
			s_standard_property_map.add("_width", M_WIDTH);
			s_standard_property_map.add("_height", M_HEIGHT);
			s_standard_property_map.add("_rotation", M_ROTATION);
			s_standard_property_map.add("_target", M_TARGET);
			s_standard_property_map.add("_framesloaded", M_FRAMESLOADED);
			s_standard_property_map.add("_name", M_NAME);
			s_standard_property_map.add("_droptarget", M_DROPTARGET);
			s_standard_property_map.add("_url", M_URL);
			s_standard_property_map.add("_highquality", M_HIGHQUALITY);
			s_standard_property_map.add("_focusrect", M_FOCUSRECT);
			s_standard_property_map.add("_soundbuftime", M_SOUNDBUFTIME);
			s_standard_property_map.add("_xmouse", M_XMOUSE);
			s_standard_property_map.add("_ymouse", M_YMOUSE);
			s_standard_property_map.add("_parent", M_PARENT);
			s_standard_property_map.add("text", M_TEXT);
			s_standard_property_map.add("textWidth", M_TEXTWIDTH);
			s_standard_property_map.add("textColor", M_TEXTCOLOR);
			s_standard_property_map.add("border", M_BORDER);
			s_standard_property_map.add("multiline", M_MULTILINE);
			s_standard_property_map.add("wordWrap", M_WORDWRAP);
			s_standard_property_map.add("type", M_TYPE);
			s_standard_property_map.add("backgroundColor", M_BACKGROUNDCOLOR);
			s_standard_property_map.add("_this", M_THIS);
			s_standard_property_map.add("this", MTHIS);
			s_standard_property_map.add("_root", M_ROOT);
			s_standard_property_map.add(".", MDOT);
			s_standard_property_map.add("..", MDOT2);
			s_standard_property_map.add("_level0", M_LEVEL0);
			s_standard_property_map.add("_global", M_GLOBAL);
			s_standard_property_map.add("enabled", M_ENABLED);
		}

		as_standard_member	result = M_INVALID_MEMBER;
		s_standard_property_map.get(name, &result);

		return result;
	}

	//
	// properties by number
	//

	static const tu_string	s_property_names[] =
	{
		tu_string("_x"),
		tu_string("_y"),
		tu_string("_xscale"),
		tu_string("_yscale"),
		tu_string("_currentframe"),
		tu_string("_totalframes"),
		tu_string("_alpha"),
		tu_string("_visible"),
		tu_string("_width"),
		tu_string("_height"),
		tu_string("_rotation"),
		tu_string("_target"),
		tu_string("_framesloaded"),
		tu_string("_name"),
		tu_string("_droptarget"),
		tu_string("_url"),
		tu_string("_highquality"),
		tu_string("_focusrect"),
		tu_string("_soundbuftime"),
		tu_string("@@ mystery quality member"),
		tu_string("_xmouse"),
		tu_string("_ymouse"),
	};

	as_value	get_property(as_object* obj, int prop_number)
	{
		as_value	val;
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->get_member(s_property_names[prop_number], &val);
		}
		else
		{
			log_error("error: invalid property query, property number %d\n", prop_number);
		}
		return val;
	}

	void	set_property(as_object* obj, int prop_number, const as_value& val)
	{
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->set_member(s_property_names[prop_number], val);
		}
		else
		{
			log_error("error: invalid set_property, property number %d\n", prop_number);
		}
	}


	//
	//	gameswf_player
	//

	gameswf_player::gameswf_player()
	{
		m_global = new as_object();
		action_init();
	}

	gameswf_player::~gameswf_player()
	{
		// Clean up gameswf as much as possible, so valgrind will help find actual leaks.
		m_global = NULL;
		clear_gameswf();
		action_clear();
	}

	void gameswf_player::set_bootup_options(const tu_string& param)
	// gameSWF extension
	// Allow pass the user bootup options to Flash (through _global._bootup)
	{
		m_global->set_member("_bootup", param.c_str());
	}

	void gameswf_player::verbose_action(bool val)
	{
		set_verbose_action(val);
	}

	void gameswf_player::verbose_parse(bool val)
	{
		set_verbose_parse(val);
	}

	void	as_global_trace(const fn_call& fn);
	void	gameswf_player::action_init()
	// Create/hook built-ins.
	{

		// setup builtin methods
		stringi_hash<as_value>* map;

		// as_object builtins
		map = new_standard_method_map(BUILTIN_OBJECT_METHOD);
		map->add("addProperty", as_object_addproperty);
		map->add("registerClass", as_object_registerclass);
		map->add("hasOwnProperty", as_object_hasownproperty);
		map->add("watch", as_object_watch);
		map->add("unwatch", as_object_unwatch);

		// for debugging
#ifdef _DEBUG
		map->add("dump", as_object_dump);
#endif

		// as_number builtins
		map = new_standard_method_map(BUILTIN_NUMBER_METHOD);
		map->add("toString", as_number_to_string);
		map->add("valueOf", as_number_valueof);

		// as_boolean builtins
		map = new_standard_method_map(BUILTIN_BOOLEAN_METHOD);
		map->add("toString", as_boolean_to_string);
		map->add("valueOf", as_boolean_valueof);

		// as_string builtins
		map = new_standard_method_map(BUILTIN_STRING_METHOD);
		map->add("toString", string_to_string);
		map->add("fromCharCode", string_from_char_code);
		map->add("charCodeAt", string_char_code_at);
		map->add("concat", string_concat);
		map->add("indexOf", string_index_of);
		map->add("lastIndexOf", string_last_index_of);
		map->add("slice", string_slice);
		map->add("split", string_split);
		map->add("substring", string_substring);
		map->add("substr", string_substr);
		map->add("toLowerCase", string_to_lowercase);
		map->add("toUpperCase", string_to_uppercase);
		map->add("charAt", string_char_at);
		map->add("length", as_value(string_length, NULL));

		// sprite_instance builtins
		map = new_standard_method_map(BUILTIN_SPRITE_METHOD);
		map->add("play", sprite_play);
		map->add("stop", sprite_stop);
		map->add("gotoAndStop", sprite_goto_and_stop);
		map->add("gotoAndPlay", sprite_goto_and_play);
		map->add("nextFrame", sprite_next_frame);
		map->add("prevFrame", sprite_prev_frame);
		map->add("getBytesLoaded", sprite_get_bytes_loaded);
		map->add("getBytesTotal", sprite_get_bytes_total);
		map->add("swapDepths", sprite_swap_depths);
		map->add("duplicateMovieClip", sprite_duplicate_movieclip);
		map->add("getDepth", sprite_get_depth);
		map->add("createEmptyMovieClip", sprite_create_empty_movieclip);
		map->add("removeMovieClip", sprite_remove_movieclip);
		map->add("hitTest", sprite_hit_test);
		map->add("startDrag", sprite_start_drag);
		map->add("stopDrag", sprite_stop_drag);
		map->add("loadMovie", sprite_loadmovie);
		map->add("unloadMovie", sprite_unloadmovie);
		map->add("getNextHighestDepth", sprite_getnexthighestdepth);
		map->add("createTextField", sprite_create_text_field);
		map->add("attachMovie", sprite_attach_movie);

		// drawing API
		map->add("beginFill", sprite_begin_fill);
		map->add("endFill", sprite_end_fill);
		map->add("lineTo", sprite_line_to);
		map->add("moveTo", sprite_move_to);
		map->add("curveTo", sprite_curve_to);
		map->add("clear", sprite_clear);
		map->add("lineStyle", sprite_line_style);

		// gameSWF extension
		// reset root FPS
		map->add("setFPS", sprite_set_fps);


		m_start_time = tu_timer::get_ticks();

		//
		// global init
		//

		m_heap.set(m_global.get_ptr(), false);
		m_global->builtin_member("trace", as_global_trace);
		m_global->builtin_member("Object", as_global_object_ctor);
		m_global->builtin_member("Sound", as_global_sound_ctor);
		m_global->builtin_member("Array", as_global_array_ctor);
		m_global->builtin_member("MovieClip", as_global_movieclip_ctor);
		m_global->builtin_member("TextFormat", as_global_textformat_ctor);

		//			m_global->set_member("XML", as_value(xml_new));
		m_global->builtin_member("XMLSocket", as_global_xmlsock_ctor);
		m_global->builtin_member("MovieClipLoader", as_global_mcloader_ctor);
		m_global->builtin_member("String", string_ctor);
		m_global->builtin_member("Number", as_global_number_ctor);
		m_global->builtin_member("Boolean", as_global_boolean_ctor);
		m_global->builtin_member("Color", as_global_color_ctor);
		m_global->builtin_member("Date", as_global_date_ctor);
		m_global->builtin_member("Selection", selection_init());
		m_global->builtin_member("LoadVars", as_global_loadvars_ctor);

		// ASSetPropFlags
		m_global->builtin_member("ASSetPropFlags", as_global_assetpropflags);

		// for video
		m_global->builtin_member("NetStream", as_global_netstream_ctor);
		m_global->builtin_member("NetConnection", as_global_netconnection_ctor);

		m_global->builtin_member("math", math_init());
		m_global->builtin_member("Key", key_init());
		m_global->builtin_member("AsBroadcaster", broadcaster_init());
		m_global->builtin_member( "flash", flash_init());

		// global builtins functions
		m_global->builtin_member("setInterval",  as_global_setinterval);
		m_global->builtin_member("clearInterval",  as_global_clearinterval);
		m_global->builtin_member("getVersion",  as_global_get_version);
		m_global->builtin_member("parseFloat",  as_global_parse_float);
		m_global->builtin_member("parseInt",  as_global_parse_int);
		m_global->builtin_member("isNaN",  as_global_isnan);
		m_global->builtin_member("$version",  "gameSWF");

	}


	void	gameswf_player::action_clear()
	{
		clear_standard_property_map();
		clear_standard_method_map();
	}


	// garbage collector

	void heap::clear()
	{
		for (hash<smart_ptr<as_object>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object* obj = it->first.get_ptr();
			if (obj)
			{
				if (obj->get_ref_count() > 1)
				{
					hash<as_object*, bool> visited_objects;
					obj->clear_refs(&visited_objects, obj);
				}
			}
		}
		m_heap.clear();
	}

	bool heap::is_garbage(as_object* obj)
	{
		bool is_garbage = false;
		m_heap.get(obj, &is_garbage);
		return is_garbage;
	}

	void heap::set(as_object* obj, bool is_garbage)
	{
		m_heap.set(obj, is_garbage);
	}

	void heap::set_as_garbage()
	{
		for (hash<smart_ptr<as_object>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object* obj = it->first.get_ptr();
			if (obj)
			{
				m_heap.set(obj, true);
			}
		}
	}

	void heap::clear_garbage()
	{
		as_object* global = get_global();
		global->this_alive();
		for (hash<smart_ptr<as_object>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object* obj = it->first.get_ptr();
			if (obj)
			{
				if (it->second)	// is garbage ?
				{
					if (obj->get_ref_count() > 1)	// is in heap only ?
					{
						hash<as_object*, bool> visited_objects;
						obj->clear_refs(&visited_objects, obj);
					}
					m_heap.erase(obj);
				}
			}
		}
	}


}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

