// tu_mutex.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_MUTEX_H
#define GAMESWF_MUTEX_H

#include "base/tu_config.h"
#include "gameswf/gameswf_types.h"
#include "gameswf/gameswf_log.h"

typedef void (*thread_start_func)(void* arg);

#if TU_CONFIG_LINK_TO_THREAD == 1

#include <SDL.h>
#include <SDL_thread.h>

namespace gameswf
{
	int sdl_thread_start_func(void* ptr);
	struct tu_thread : public ref_counted
	{
		tu_thread(void (*fn)(void *), void* data);
		~tu_thread();

		void wait();
		void kill();
		void start();

	private:
		SDL_Thread* m_thread;
		thread_start_func m_func;
		void* m_arg;
	};

	struct tu_mutex
	{
		exported_module tu_mutex();
		exported_module ~tu_mutex(); 

		exported_module void lock();
		exported_module void unlock();

		SDL_mutex* m_mutex;
	};

	// like autoptr
	struct tu_autolock
	{
		tu_mutex& m_mutex;
		tu_autolock(tu_mutex& mutex) : m_mutex(mutex)
		{
			m_mutex.lock();
		}

		~tu_autolock()
		{
			m_mutex.unlock();
		}
	};

	struct tu_condition
	{
		tu_condition();
		~tu_condition();

		// Wait on the condition variable cond and unlock the provided mutex.
		// The mutex must the locked before entering this function.
		void wait();
		void signal();

		SDL_cond* m_cond;
		tu_mutex m_cond_mutex;
	};

}

#elif TU_CONFIG_LINK_TO_THREAD == 2	// libpthread

#include <pthread.h>

namespace gameswf
{
	void* pthread_start_func(void* ptr);
	struct tu_thread : public ref_counted
	{
		tu_thread(void (*fn)(void *), void* data);
		~tu_thread();

		void wait();
		void kill();
		void start();

	private:
		pthread_t m_thread;
		thread_start_func m_func;
		void* m_arg;
	};

	struct tu_mutex
	{
		exported_module tu_mutex();
		exported_module ~tu_mutex(); 

		exported_module void lock();
		exported_module void unlock();

		pthread_mutex_t m_mutex;
	};

	// like autoptr
	struct tu_autolock
	{
		tu_mutex& m_mutex;
		tu_autolock(tu_mutex& mutex) :
		m_mutex(mutex)
		{
			m_mutex.lock();
		}

		~tu_autolock()
		{
			m_mutex.unlock();
		}
	};

	struct tu_condition
	{
		tu_condition();
		~tu_condition();

		// Wait on the condition variable cond and unlock the provided mutex.
		// The mutex must the locked before entering this function.
		void wait();
		void signal();

		pthread_cond_t m_cond;
		tu_mutex m_cond_mutex;
	};
}

#else

namespace gameswf
{
	struct tu_thread : public ref_counted
	{
		tu_thread(void (*fn)(void *), void* data)
		{
			IF_VERBOSE_ACTION(log_msg("gameswf is in single thread mode\n"));
			(fn)(data);
		}

		~tu_thread() {}

		void wait() {}
		void kill() {}
	};

	struct tu_mutex
	{
		exported_module tu_mutex() {}
		exported_module ~tu_mutex() {}
		exported_module void lock() {}
		exported_module void unlock() {}
	};

	struct tu_condition
	{
		tu_condition() {}
		~tu_condition() {}

		void wait() {}
		void signal() {}
	};

	struct tu_autolock
	{
		tu_mutex& m_mutex;
		tu_autolock(tu_mutex& mutex) :
		m_mutex(mutex)
		{
			m_mutex.lock();
		}

		~tu_autolock()
		{
			m_mutex.unlock();
		}
	};

}

#endif	// TU_CONFIG_LINK_TO_THREAD

#endif	// GAMESWF_MUTEX_H
