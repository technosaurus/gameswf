// as_netstream.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_NETSTREAM_H
#define GAMESWF_NETSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/tu_queue.h"
#include "gameswf/gameswf_object.h"

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

	// helper, for memory management
	struct av_packet : public ref_counted
	{
		av_packet(const AVPacket& pkt)
		{
			av_dup_packet(&(AVPacket&) pkt);
			m_pkt = pkt;
		}

		~av_packet()
		{
			av_free_packet(&m_pkt);
		}

		const AVPacket& get_packet() const
		{
			return m_pkt;
		}

	private:

		AVPacket m_pkt;

	};

	struct av_queue
	{
		void push(const AVPacket& pkt)
		{
			tu_autolock locker(m_mutex);
			m_queue.push(new av_packet(pkt));
		}

		int size()
		{
			tu_autolock locker(m_mutex);
			return int(m_queue.size());
		}

		bool pop(gc_ptr<av_packet>* val)
		{
			tu_autolock locker(m_mutex);
			if (m_queue.size() > 0)
			{
				*val = m_queue.front();
				m_queue.pop(); 
				return true;
			}
			return false;
		}

		void clear()
		{
			tu_autolock locker(m_mutex);
			m_queue.clear();
		}

	private:

		tu_mutex m_mutex;
		tu_queue< gc_ptr<av_packet> > m_queue;
	};

	// container for the decoded sound
	struct decoded_sound : public ref_counted
	{
		int m_size;
		Sint16* m_data;
		Uint8* m_ptr;

		decoded_sound(Sint16* sample, int size) :
			m_size(size),
			m_data(sample)
		{
			m_ptr = (Uint8*) sample;
		}

		~decoded_sound()
		{
			// We can delete to original WAV data now (created in cvt())
			free(m_data);
		}

		int size() const
		{
			return m_size;
		}

		void take(Uint8*& stream, int& len)
		{
			if (m_ptr)
			{
				int n = imin(m_size, len);
				memcpy(stream, m_ptr, n);
				stream += n;
				m_ptr += n;
				m_size -= n;
				len -= n;
				assert(m_size >= 0 &&	len >= 0);
			}
		}
	};

	struct as_netstream : public as_object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_NETSTREAM };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		// netstream status
		enum netstream_status
		{
			STOP,
			PLAY,
			PAUSE
		};

		as_netstream(player* player);
		~as_netstream();

		bool decode_audio(const AVPacket& pkt, Sint16** data, int* size);
		Uint8* decode_video(const AVPacket& pkt);
		void run();
		void seek(double seek_time);
		void setBufferTime();
		void audio_callback(Uint8* stream, int len);
		void close();
		void play(const char* url);
		double time() const;

		int get_width() const;
		int get_height() const;

		Uint8* get_video_data();
		void set_video_data(Uint8* data);

		volatile netstream_status m_status;
		volatile double m_seek_time;

	private:

		void set_status(const char* level, const char* code);
		bool open_stream(const char* url);
		void close_stream();
		double get_duration() const;

		// return current time in sec
		double now() const;

		AVFormatContext* m_FormatCtx;

		// video
		AVCodecContext* m_VCodecCtx;
		AVStream* m_video_stream;

		// audio
		AVCodecContext *m_ACodecCtx;
		AVStream* m_audio_stream;
		gc_ptr<decoded_sound> m_sound;

		double m_start_time;	// sec
		double m_video_time;	// sec

		int m_video_index;
		int m_audio_index;

		tu_string m_url;

		av_queue m_aq;	// audio queue
		av_queue m_vq;	// video queue

		gc_ptr<tu_thread> m_thread;
		tu_condition m_decoder;

		// RGBA video frame 
		Uint8* m_video_data;
		tu_mutex m_lock_video;

		// for converting video frame in RGBA
		SwsContext* m_convert_ctx;
	};

} // end of gameswf namespace

#else	// ffmpeg is not present

namespace gameswf
{
	struct as_netstream : public as_object
	{
		int get_width() const {
			return 0;
		}
		int get_height() const {
			return 0;
		}

		Uint8* get_video_data() {
			return NULL;
		}
	};

	void as_global_netstream_ctor(const fn_call& fn);
}

#endif

// GAMESWF_NETSTREAM_H
#endif

