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

	// With these data are filled audio & video queues. 
	// They the common both for audio and for video
	struct av_data
	{
		av_data():
			m_stream_index(-1),
			m_size(0),
			m_data(NULL),
			m_ptr(NULL),
			m_pts(0)
		{
		};

		~av_data()
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

	// audio_queue is filled in decoder thread, 
	// and read in sound handler thread
	// therefore mutex is required
	struct audio_queue
	{

		audio_queue(size_t size) : m_size(size)
		{
			assert(m_size > 0);
			m_mutex = tu_mutex_create();
		};

		~audio_queue()
		{
			tu_mutex_destroy(m_mutex);
		}

		size_t size()
		{
			locker lock(m_mutex);
			return m_queue.size();
		}

		bool push(av_data* member)
		{
			locker lock(m_mutex);
			if (m_queue.size() < m_size)	// hack
			{
				m_queue.push(member);
				return true;
			}
			return false;
		}

		av_data* front()
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

	private:

		tu_mutex* m_mutex;
		tu_queue<av_data*> m_queue;
		size_t m_size;
	};

	// video_queue is used only in decoder thread,
	// therefore mutex is not required
	struct video_queue
	{

		video_queue(size_t size) : m_size(size)
		{
			assert(m_size > 0);
		};

		~video_queue()
		{
		}

		size_t size()
		{
			return m_queue.size();
		}

		bool push(av_data* member)
		{
			if (m_queue.size() < m_size)	// hack
			{
				m_queue.push(member);
				return true;
			}
			return false;
		}

		av_data* front()
		{
			if (m_queue.size() > 0)
			{
				return m_queue.front();
			}
			return NULL;
		}

		void pop()
		{
			if (m_queue.size() > 0)
			{
				m_queue.pop();
			}
		}

	private:

		tu_queue<av_data*> m_queue;
		size_t m_size;
	};

	struct as_netstream : public as_object
	{
		as_netstream();
		~as_netstream();

		virtual as_netstream* cast_to_as_netstream() { return this; }

		YUV_video* get_video();
		void run();
		void pause(int mode);
		void seek(double seek_time);
		void setBufferTime();
		void audio_callback(Uint8* stream, int len);
		void close();
		void play(const char* url);

	private:

		inline double as_double(AVRational time){	return time.num / (double) time.den; }
		void set_status(const char* level, const char* code);
		bool open_stream(const char* url);
		void close_stream();
		av_data* read_frame(av_data* vd);

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

		audio_queue m_qaudio;
		video_queue m_qvideo;

		tu_thread* m_thread;
		tu_condition m_decoder;

		smart_ptr<YUV_video> m_yuv;
		gameswf_mutex m_yuv_mutex;
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

