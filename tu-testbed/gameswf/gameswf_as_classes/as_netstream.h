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

#if TU_CONFIG_LINK_TO_FFMPEG == 1

#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>

#define VIDEO_QUEUE_SIZE 20

namespace gameswf
{

	void as_global_netstream_ctor(const fn_call& fn);
	void netstream_new(const fn_call& fn);
	void netstream_close(const fn_call& fn);
	void netstream_pause(const fn_call& fn);
	void netstream_play(const fn_call& fn);
	void netstream_seek(const fn_call& fn);
	void netstream_setbuffertime(const fn_call& fn);

	struct av_packet : public ref_counted
	{
		Uint8* m_data;
		int m_size;
		int m_stream_index;
		Sint64 m_dts;

		av_packet(const AVPacket& packet)
		{
			m_stream_index = packet.stream_index;
			m_size = packet.size;
			m_dts = packet.dts;

			m_data = new Uint8[m_size];
			memcpy(m_data, packet.data, m_size);
		}

		~av_packet()
		{
			delete [] m_data;
		}

		int get_stream_index() const
		{
			return m_stream_index;
		}

		Uint8* get_packet() const
		{
			return m_data;
		}

	};

	struct audio_queue
	{
		tu_mutex m_mutex;
		tu_queue< gc_ptr<av_packet> > m_queue;

		bool push(const gc_ptr<av_packet>& val)
		{
			tu_autolock locker(m_mutex);
			if (m_queue.size() < VIDEO_QUEUE_SIZE)
			{
				m_queue.push(val);
				return true;
			}
			return false;
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

	};

	struct video_queue
	{
		tu_queue< gc_ptr<av_packet> > m_queue;

		bool push(const gc_ptr<av_packet>& val)
		{
			if (m_queue.size() < VIDEO_QUEUE_SIZE)
			{
				m_queue.push(val);
				return true;
			}
			return false;
		}

		int size()
		{
			return int(m_queue.size());
		}

		bool pop(gc_ptr<av_packet>* val)
		{
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
			m_queue.clear();
		}

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

		as_netstream(player* player);
		~as_netstream();

		bool decode_audio(const gc_ptr<av_packet>& packet, Sint16** data, int* size);
		Uint8* decode_video(const gc_ptr<av_packet>& packet);
		void run();
		void pause(int mode);
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
		gc_ptr<decoded_sound> m_sound;

		double m_start_time;
		double m_video_clock;
		double m_current_clock;

		int m_video_index;
		int m_audio_index;

		tu_string m_url;
		volatile bool m_is_alive;
		volatile bool m_break;
		volatile bool m_pause;

		gc_ptr<av_packet> m_unqueued_data;

		// this is used in the decoder & sound threads
		audio_queue m_audio_queue;

		// this is used in  the decoder thread only
		video_queue m_video_queue;

		tu_thread* m_thread;
		tu_condition m_decoder;

		// decoded data
		Uint8* m_data;
		tu_mutex m_lock_data;

	};

} // end of gameswf namespace

#else	// ffmpeg is not present

#include "gameswf/gameswf_video_impl.h"

namespace gameswf
{
	void as_global_netstream_ctor(const fn_call& fn);
}

#endif

// GAMESWF_NETSTREAM_H
#endif

