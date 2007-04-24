// gameswf_mutex.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// auto locker/unlocker

#include "gameswf_mutex.h"
#include "base/smart_ptr.h"

namespace gameswf
{

	static gameswf_mutex s_gameswf_mutex;

	tu_mutex* get_gameswf_mutex()
	{
		return s_gameswf_mutex.get_mutex();
	}

}
