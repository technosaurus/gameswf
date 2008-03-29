// gameswf.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameswf player implementation
// Here are collected all static variables,
// init_action() and clear_libray()


#ifndef GAMESWF_PLAYER_H
#define GAMESWF_PLAYER_H

#include "base/utility.h"
#include "gameswf/gameswf.h"


namespace gameswf
{
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

