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
	struct av_data : public ref_counted
	{
		av_data(int stream, Uint8* data, int size) :
			m_stream_index(stream),
			m_size(size),
			m_data(data),
			m_ptr(data),
			m_pts(0)
		{
			assert(data);
		};

		~av_data()
		{
			delete m_data;
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
		bool read_frame();

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

		tu_queue< smart_ptr<av_data> > m_qvideo;

		gameswf_mutex m_audio_mutex;
		tu_queue< smart_ptr<av_data> > m_qaudio;

		tu_thread* m_thread;
		tu_condition m_decoder;

		YUV_video* m_yuv;
		gameswf_mutex m_yuv_mutex;
		smart_ptr<av_data> m_unqueued_data;

		size_t m_queue_size;
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

