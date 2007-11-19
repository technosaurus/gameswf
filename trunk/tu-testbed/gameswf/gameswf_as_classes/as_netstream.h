// as_netstream.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_NETSTREAM_H
#define GAMESWF_NETSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/tu_queue.h"
#include "gameswf/gameswf_video_impl.h"
#include "gameswf/gameswf_video_base.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>

namespace gameswf
{

	void as_global_netstream_ctor(const fn_call& fn);
	void netstream_new(const fn_call& fn);
	void netstream_close(const fn_call& fn);
	void netstream_pause(const fn_call& fn);
	void netstream_play(const fn_call& fn);
	void netstream_seek(const fn_call& fn);
	void netstream_setbuffertime(const fn_call& fn);

	struct as_netstream : public as_object
	{
		as_netstream();
		~as_netstream();

		virtual as_netstream* cast_to_as_netstream() { return this; }

		video_handler* get_video_handler();
		void run();
		void pause(int mode);
		void seek(double seek_time);
		void setBufferTime();
		void audio_callback(Uint8* stream, int len);
		void close();
		void play(const char* url);
		float time() const;

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

		volatile double m_video_clock;
		double m_start_clock;

		int m_video_index;
		int m_audio_index;

		tu_string m_url;
		volatile bool m_is_alive;
		volatile bool m_break;
		volatile bool m_pause;

		// this is used only in decoder thread
		tu_queue< smart_ptr<video_data> > m_qvideo;

		// this is used in decoder & sound threads
		tu_mutex m_audio_mutex;
		tu_queue< smart_ptr<audio_data> > m_qaudio;

		tu_thread* m_thread;
		tu_condition m_decoder;

		smart_ptr<video_handler> m_video_handler;

		smart_ptr<audio_video_data> m_unqueued_data;

		size_t m_queue_size;
	};

} // end of gameswf namespace

#else	// ffmpeg is not present

#include "gameswf/gameswf_video_impl.h"

namespace gameswf
{
	void as_global_netstream_ctor(const fn_call& fn);

	struct as_netstream : public as_object
	{
		video_handler* get_video_handler() { return NULL; }
	};

}

#endif

// GAMESWF_NETSTREAM_H
#endif

