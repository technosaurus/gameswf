// gameswf_mutex.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// auto locker/unlocker

#ifndef GAMESWF_MUTEX_H
#define GAMESWF_MUTEX_H

#include <SDL.h>

namespace gameswf
{

	struct locker
	{
		locker(SDL_mutex* mutex):
			m_mutex(mutex)
		{
			SDL_LockMutex(m_mutex);
		}

		~locker()
		{
			SDL_UnlockMutex(m_mutex);
		}

		private:
			SDL_mutex* m_mutex;
	};

	// auto mutex
	struct gameswf_mutex
	{
		gameswf_mutex()
		{
			m_mutex = SDL_CreateMutex();
		}

		~gameswf_mutex()
		{
			SDL_DestroyMutex(m_mutex);
		}

		SDL_mutex* get_mutex()
		{
			return m_mutex;
		}

		private:
			SDL_mutex* m_mutex;

	};

	SDL_mutex* get_gameswf_mutex();

}

#endif
