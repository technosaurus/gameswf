// gameswf_mutex.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// auto locker/unlocker
// We have redefined SDL mutex functions that
// there was an opportunity to use other libraries (pthread, ...)

#ifndef GAMESWF_CURSOR_H
#define GAMESWF_CURSOR_H

#include <SDL.h>

namespace gameswf
{
	enum cursor_type
	{
		SYSTEM_CURSOR,
		ACTIVE_CURSOR
	};

	void create_cursor();
	void clear_cursor();
	void set_cursor(cursor_type cursor);
}

#endif