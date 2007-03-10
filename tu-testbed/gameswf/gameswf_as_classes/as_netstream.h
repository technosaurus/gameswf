// as_netstream.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_NETSTREAM_H
#define GAMESWF_NETSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_FFMPEG

#include <ffmpeg/avformat.h>

#include "base/tu_queue.h"
#include "../gameswf_mutex.h"
#include "../gameswf_video_impl.h"

namespace gameswf
{

	void as_global_netstream_ctor(const fn_call& fn);
	void netstream_new(const fn_call& fn);
	void netstream_close(const fn_call& fn);
	void netstream_pause(const fn_call& fn);
	void netstream_play(const fn_call& fn);
	void netstream_seek(const fn_call& fn);
	void netstream_setbuffertime(const fn_call& fn);

	template<class T>
	class multithread_queue
	{
	public:

		multithread_queue(const int size):m_size(size)
		{
			assert(m_size > 0);
			m_mutex = tu_mutex_create();
		};

		~multithread_queue()
		{
			tu_mutex_destroy(m_mutex);
		}

		size_t size()
		{
			locker lock(m_mutex);
			return m_queue.size();
		}

		void clear()
		{
			locker lock(m_mutex);
			while (m_queue.size() > 0)
			{
				T x = m_queue.front();
				m_queue.pop();
				//				delete x;
			}
		}

		bool push(T member)
		{
			locker lock(m_mutex);
			if (m_queue.size() < m_size)	// hack
			{
				m_queue.push(member);
				return true;
			}
			return false;
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
			locker lock(m_mutex);
			if (m_queue.size() > 0)
			{
				return m_queue.front();
			}
			return NULL;
		}

		void pop()
		{
			locker lock(m_mutex);
			if (m_queue.size() > 0)
			{
				m_queue.pop();
			}
		}

		void extract(T* var, int count)
		{
			locker lock(m_mutex);
			int i = 0;
			while (m_queue.size() > 0 && i < count)
			{
				var[i++] = m_queue.front();
				m_queue.pop();
			}
		}

	private:

		tu_mutex* m_mutex;
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

	struct as_netstream : public as_object
	{
		as_netstream();
		~as_netstream();

		virtual as_netstream* cast_to_as_netstream() { return this; }

		void set_status(const char* level, const char* code);
		void pause(int mode);

		void run();
		bool open_stream(const char* source);
		void close_stream();
		void seek(double seek_time);
		void setBufferTime();

		raw_videodata_t* read_frame(raw_videodata_t* vd);
		YUV_video* get_video();
		void audio_callback(Uint8* stream, int len);
		void close();
		void play(const char* url);

		inline double as_double(AVRational time)
		{
			return time.num / (double) time.den;
		}

	private:

		AVFormatContext *m_FormatCtx;

		// video
		AVCodecContext* m_VCodecCtx;
		AVStream* m_video_stream;

		// audio
		AVCodecContext *m_ACodecCtx;
		AVStream* m_audio_stream;

		AVFrame* m_Frame;

		double m_video_clock;
		double m_start_clock;

		int m_video_index;
		int m_audio_index;

		tu_string m_url;
		volatile bool m_is_alive;
		volatile bool m_break;
		volatile bool m_pause;

		YUV_video* m_yuv;

		multithread_queue<raw_videodata_t*> m_qaudio;
		multithread_queue<raw_videodata_t*> m_qvideo;

		tu_thread* m_thread;
		tu_condition m_cond;
	};

} // end of gameswf namespace

#else	// ffmpeg is not present

#include "../gameswf_video_impl.h"

namespace gameswf
{
	void as_global_netstream_ctor(const fn_call& fn);

	struct as_netstream : public as_object
	{
		YUV_video* get_video() { return NULL; }
	};

}

#endif

// GAMESWF_NETSTREAM_H
#endif

