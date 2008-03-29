// gameswf.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameswf player implementation

#include "gameswf/gameswf_player.h"
#include "gameswf/gameswf_object.h"
#include "base/tu_timer.h"

namespace gameswf
{

	smart_ptr<as_object>	s_global;
	as_object* get_global()
	{
		return s_global.get_ptr();
	}

	gameswf_player::gameswf_player()
	{
		s_global = new as_object();
		action_init();
	}

	gameswf_player::~gameswf_player()
	{
		// Clean up gameswf as much as possible, so valgrind will help find actual leaks.
		s_global = NULL;
		clear_gameswf();
		action_clear();
	}

	void gameswf_player::set_bootup_options(const tu_string& param)
	// gameSWF extension
	// Allow pass the user bootup options to Flash (through _global._bootup)
	{
		s_global->set_member("_bootup", param.c_str());
	}

	void gameswf_player::verbose_action(bool val)
	{
		set_verbose_action(val);
	}

	void gameswf_player::verbose_parse(bool val)
	{
		set_verbose_parse(val);
	}

}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

