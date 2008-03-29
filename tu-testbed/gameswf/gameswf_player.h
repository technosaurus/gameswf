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

		// Mouse state.
		int	mouse_x;
		int	mouse_y;
		int	mouse_buttons;

		Uint32	start_ticks;
		Uint32	last_ticks;
		int	frame_counter;
		int	last_logged_fps;
		bool	do_render;

		exported_module  gameswf_player();
		exported_module  ~gameswf_player();

		void verbose_action(bool val)
		{
			set_verbose_action(val);
		}

		void verbose_parse(bool val)
		{
			set_verbose_parse(val);
		}

	};
}

#endif

