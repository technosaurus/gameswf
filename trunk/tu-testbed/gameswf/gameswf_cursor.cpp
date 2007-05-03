// gameswf_cursor.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Used code from SDL help

#include "gameswf/gameswf_cursor.h"
#include <assert.h>

// TODO(tulrich): REMOVE ALL SDL DEPENDENCIES FROM CORE GAMESWF.
// (Embedders probably aren't using SDL; don't make their job harder.)
#include <SDL.h>

namespace gameswf
{

	// XPM 
	static const char *image[] =
	{
		// width height num_colors chars_per_pixel
		"    32    32        3            1",
		// colors
		"X c #000000",
		". c #ffffff",
		"  c None",
		// pixels
		"   XX                           ",
		"  X..X                          ",
		"  X..X                          ",
		"  X..X                          ",
		"  X..X                          ",
		"  X..XXX                        ",
		"  X..X..XXX                     ",
		"XXX..X..X..XXX                  ",
		"X.X..X..X..X..X                 ",
		"X.X..X..X..X..X                 ",
		"X....X..X..X..X                 ",
		"X..........X..X                 ",
		" X............X                 ",
		" X...........X                  ",
		" X...........X                  ",
		" X...........X                  ",
		" XXXXXXXXXXXXX                  ",
		" XXXXXXXXXXXXX                  ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"0,0"
	};


	static SDL_Cursor* s_system_cursor = NULL;
	static SDL_Cursor* s_active_cursor = NULL;

	void create_cursor()
	{
		int i, row, col;
		Uint8 data[4 * 32];
		Uint8 mask[4 * 32];
		int hot_x, hot_y;

		i = -1;
		for (row=0; row<32; ++row)
		{
			for (col=0; col<32; ++col)
			{
				if (col % 8)
				{
					data[i] <<= 1;
					mask[i] <<= 1;
				} 
				else
				{
					++i;
					data[i] = mask[i] = 0;
				}

				switch (image[4 + row][col])
				{
					case 'X':
						// black
						data[i] |= 0x01;
						mask[i] |= 0x01;
						break;

					case '.':
						// white
						mask[i] |= 0x01;
						break;

					case ' ':
						// transparent
						break;
				}
			}
		}

		// store system cursor
		s_system_cursor = SDL_GetCursor();

		sscanf(image[4 + row], "%d,%d", &hot_x, &hot_y);
		s_active_cursor = SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
	}

	void clear_cursor()
	{
		if (s_system_cursor)
		{
			SDL_SetCursor(s_system_cursor);
			if (s_active_cursor)
			{
				SDL_FreeCursor(s_active_cursor);
				s_active_cursor = NULL;
			}
		}
		s_system_cursor = NULL;
	}

	void set_cursor(cursor_type cursor)
	{
		if (s_system_cursor && s_active_cursor)
		{
			switch (cursor)
			{
				case SYSTEM_CURSOR:
					SDL_SetCursor(s_system_cursor);
					break;

				case ACTIVE_CURSOR:
					SDL_SetCursor(s_active_cursor);
					break;

				default:
					assert(0);
			}
		}
	}
}
