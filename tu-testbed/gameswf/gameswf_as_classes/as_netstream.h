// as_netstream.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_NETSTREAM_H
#define GAMESWF_NETSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../gameswf_video_impl.h"

#ifdef USE_FFMPEG
#include <ffmpeg/avformat.h>
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include "base/tu_queue.h"

namespace gameswf
{

	template<class T>
	class multithread_queue
	{
	public:

		multithread_queue(const int size):m_size(size)
		{
			assert(m_size > 0);
			m_mutex = SDL_CreateMutex();
		};

		~multithread_queue()
		{
			SDL_DestroyMutex(m_mutex);
		}

		size_t size()
		{
			lock();
			size_t n = m_queue.size();
			unlock();
			return n;
		}

		void clear()
		{
			lock();
			while (m_queue.size() > 0)
			{
				T x = m_queue.front();
				m_queue.pop();
				//				delete x;
			}
			unlock();
		}

		bool push(T member)
		{
			bool rc = false;
			lock();
			if (m_queue.size() < m_size)	// hack
			{
				m_queue.push(member);
				rc = true;
			}
			unlock();
			return rc;
		}

		bool push_roll(T member)
		{
			bool rc = false;
			lock();
			if (m_queue.size() < m_size)
			{
				m_queue.push(member);
				rc = true;
			}
			else
			{
				m_queue.pop();	// remove the oldest member
				m_queue.push(member);
			}
			unlock();
			return rc;
		}

		T front()
		{
			lock();
			T member = NULL;
			if (m_queue.size() > 0)
			{
				member = m_queue.front();
			}
			unlock();
			return member;
		}

		void pop()
		{
			lock();
			if (m_queue.size() > 0)
			{
				m_queue.pop();
			}
			unlock();
		}

		void extract(T* var, int count)
		{
			lock();
			int i = 0;
			while (m_queue.size() > 0 && i < count)
			{
				var[i++] = m_queue.front();
				m_queue.pop();
			}
			unlock();
		}

	private:

		inline void lock()
		{
			SDL_LockMutex(m_mutex);
		}

		inline void unlock()
		{
			SDL_UnlockMutex(m_mutex);
		}

		SDL_mutex* m_mutex;
		tu_queue<T> m_queue;
		size_t m_size;
	};

	struct raw_videodata_t
	{
		raw_videodata_t():
		m_stream_index(-1),
		m_size(0),
		m_data(NULL),
		m_ptr(NULL),
		m_pts(0)
	{
	};

	~raw_videodata_t()
	{
		if (m_data)
		{
			delete m_data;
		}
	};

	int m_stream_index;
	Uint32 m_size;
	Uint8* m_data;
	Uint8* m_ptr;
	double m_pts;	// presentation timestamp in sec
	};

	struct netstream
	{

		netstream();
		~netstream();

		void set_status(const char* code);
		void close();
		void pause(int mode);
		int play(const char* source);
		void seek(double seek_time);
		void setBufferTime();

		raw_videodata_t* read_frame(raw_videodata_t* vd);
		YUV_video* get_video();

#ifdef USE_FFMPEG
		inline double as_double(AVRational time)
		{
			return time.num / (double) time.den;
		}
#endif
		static int av_streamer(void* arg);
		static void audio_streamer(void *udata, Uint8 *stream, int len);

		void set_ns(void* ns)
		{
			m_ns = ns;
		}

	private:

#ifdef USE_FFMPEG
		AVFormatContext *m_FormatCtx;

		// video
		AVCodecContext* m_VCodecCtx;
		AVStream* m_video_stream;

		// audio
		AVCodecContext *m_ACodecCtx;
		AVStream* m_audio_stream;

		AVFrame* m_Frame;
#endif

		int m_video_index;
		int m_audio_index;
		volatile bool m_go;

		YUV_video* m_yuv;
		double m_video_clock;

		SDL_Thread* m_thread;
		multithread_queue<raw_videodata_t*> m_qaudio;
		multithread_queue<raw_videodata_t*> m_qvideo;
		bool m_pause;
		void* m_ns;
		double m_start_clock;
	};

	struct as_netstream : public as_object
	{
		as_netstream()
		{
			obj.set_ns(this);
		}

		~as_netstream()
		{
		}

		netstream obj;

		//	virtual void set_member(const tu_stringi& name, const as_value& val);
		//	virtual bool get_member(const tu_stringi& name, as_value* val);

	};

	void netstream_new(const fn_call& fn);
	void netstream_close(const fn_call& fn);
	void netstream_pause(const fn_call& fn);
	void netstream_play(const fn_call& fn);
	void netstream_seek(const fn_call& fn);
	void netstream_setbuffertime(const fn_call& fn);

} // end of gameswf namespace

// GAMESWF_NETSTREAM_H
#endif

