// gameswf_mutex.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// auto locker/unlocker
// We have redefined SDL mutex functions that
// there was an opportunity to use other libraries (pthread, ...)

#ifndef GAMESWF_MUTEX_H
#define GAMESWF_MUTEX_H

#include <SDL.h>

namespace gameswf
{

	typedef SDL_mutex tu_mutex;
	
	inline int tu_mutex_lock(tu_mutex* mutex)
	{
		return SDL_LockMutex(mutex);
	}

	inline int tu_mutex_unlock(tu_mutex* mutex)
	{
		return SDL_UnlockMutex(mutex);
	}

	inline tu_mutex* tu_mutex_create()
	{
		return SDL_CreateMutex();
	}

	inline void tu_mutex_destroy(tu_mutex* mutex)
	{
		SDL_DestroyMutex(mutex);
	}


	struct locker
	{
		locker(tu_mutex* mutex):
			m_mutex(mutex)
		{
			tu_mutex_lock(m_mutex);
		}

		~locker()
		{
			tu_mutex_unlock(m_mutex);
		}

		private:

			tu_mutex* m_mutex;
	};

	// auto mutex
	struct gameswf_mutex
	{
		gameswf_mutex()
		{
			m_mutex = tu_mutex_create();
		}

		~gameswf_mutex()
		{
			tu_mutex_destroy(m_mutex);
		}

		tu_mutex* get_mutex()
		{
			return m_mutex;
		}

		private:

			tu_mutex* m_mutex;

	};

	tu_mutex* get_gameswf_mutex();

}

#endif
