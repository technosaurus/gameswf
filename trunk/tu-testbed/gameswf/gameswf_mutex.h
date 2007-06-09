// gameswf_mutex.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// auto locker/unlocker
// We have redefined SDL mutex functions that
// there was an opportunity to use other libraries (pthread, ...)

#ifndef GAMESWF_MUTEX_H
#define GAMESWF_MUTEX_H

#include <SDL.h>
#include <SDL_thread.h>

namespace gameswf
{

	typedef SDL_mutex tu_mutex;
	typedef SDL_Thread tu_thread;

	inline void tu_wait_thread(tu_thread* thread)
	{
		SDL_WaitThread(thread, NULL);
	}

	inline tu_thread* tu_create_thread(int (*fn)(void *), void* data)
	{
		return SDL_CreateThread(fn, data);
	}

	inline void tu_kill_thread(tu_thread* thread)
	{
		return SDL_KillThread(thread);
	}

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

		inline tu_mutex* get_mutex() const
		{
			return m_mutex;
		}

		void lock()
		{
			tu_mutex_lock(m_mutex);
		}

		void unlock()
		{
			tu_mutex_unlock(m_mutex);
		}

		private:

			tu_mutex* m_mutex;

	};

	struct tu_condition
	{
		tu_condition()
		{
			m_cond = SDL_CreateCond();
		}

		~tu_condition()
		{
			SDL_DestroyCond(m_cond);
		}

		// Wait on the condition variable cond and unlock the provided mutex.
		// The mutex must the locked before entering this function.
		void wait()
		{
			m_cond_mutex.lock();
			SDL_CondWait(m_cond, m_cond_mutex.get_mutex());
		}

		void signal()
		{
			SDL_CondSignal(m_cond);
		}

		SDL_cond *m_cond;
		gameswf_mutex m_cond_mutex;
	};

	tu_mutex* get_gameswf_mutex();

}

#endif
