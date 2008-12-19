// as_netstream.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf/gameswf_as_classes/as_netstream.h"
#include "gameswf/gameswf_render.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_root.h"
#include "base/tu_timer.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

//#define CLEAR_BLUE_VIDEO_BACKGROUND
#define AV_QUEUE_SIZE 20

namespace gameswf
{

	// it is running in decoder thread
	static int netstream_server(void* arg)
	{
		as_netstream* ns = (as_netstream*) arg;
		ns->run();
		return 0;
	}

	// audio callback is running in sound handler thread
	static void audio_streamer(as_object* netstream, Uint8* stream, int len)
	{
		as_netstream* ns = cast_to<as_netstream>(netstream);
		assert(ns);
		ns->audio_callback(stream, len);
	}

	as_netstream::as_netstream(player* player):
		as_object(player),
		m_FormatCtx(NULL),
		m_VCodecCtx(NULL),
		m_ACodecCtx(NULL),
		m_video_clock(0),
		m_video_index(-1),              
		m_audio_index(-1),
		m_is_alive(true),
		m_pause(false),
		m_thread(NULL),
		m_data(NULL)
	{
		av_register_all();
		m_thread = new tu_thread(netstream_server, this);
	}

	as_netstream::~as_netstream()
	{
		m_is_alive = false;
		m_decoder.signal();
		m_thread->wait();
		delete m_thread;

		set_video_data(NULL);

		m_aq.clear();
		m_vq.clear();
	}

	void as_netstream::play(const char* url)
	{
		// is path relative ?
		tu_string infile = get_player()->get_workdir();
		if (strstr(url, ":") || url[0] == '/')
		{
			infile = "";
		}
		infile += url;
		m_url = infile;

		m_break = true;
		m_decoder.signal();
	}

	void as_netstream::close()
	{
		m_break = true;
	}

	// it is running in sound mixer thread
	void as_netstream::audio_callback(Uint8* stream, int len)
	{
		if (m_pause)
		{
			return;
		}

		while (len > 0)
		{
			// decode sound
			if (m_sound == NULL)
			{
				gc_ptr<av_packet> audio;
				if (m_aq.pop(&audio))
				{
					Sint16* sample;
					int size;
					decode_audio(audio->get_packet(), &sample, &size);
					m_sound = new decoded_sound(sample, size);
					continue;
				}
				break;
			}
			else
			{
				m_sound->take(stream, len);
				if (m_sound->size() == 0)
				{
					m_sound = NULL;
				}
			}
		}

	}

	// it is running in decoder thread
	void as_netstream::set_status(const char* level, const char* code)
	{
		if (m_is_alive)
		{
			gameswf_engine_mutex().lock();

			as_value function;
			if (get_member("onStatus", &function))
			{
				gc_ptr<as_object> infoObject = new as_object(get_player());
				infoObject->set_member("level", level);
				infoObject->set_member("code", code);

				as_environment env(get_player());
				env.push(infoObject.get_ptr());
				call_method(function, &env, this, 1, env.get_top_index());
			}
			gameswf_engine_mutex().unlock();
		}
	}

	void as_netstream::pause(int mode)
	{
		if (mode == -1)
		{
			m_pause = ! m_pause;
		}
		else
		{
			m_pause = (mode == 0) ? true : false;
		}

		if (m_pause == false)
		{
			m_decoder.signal();
		}
	}

	// it is running in decoder thread
	void as_netstream::close_stream()
	{
		m_vq.clear();
		m_aq.clear();

		if (m_VCodecCtx) avcodec_close(m_VCodecCtx);
		m_VCodecCtx = NULL;

		if (m_ACodecCtx) avcodec_close(m_ACodecCtx);
		m_ACodecCtx = NULL;

		if (m_FormatCtx) av_close_input_file(m_FormatCtx);
		m_FormatCtx = NULL;
	}

	// it is running in decoder thread
	bool as_netstream::open_stream(const char* c_url)
	{
		// This registers all available file formats and codecs 
		// with the library so they will be used automatically when
		// a file with the corresponding format/codec is opened

		// Open video file
		// The last three parameters specify the file format, buffer size and format parameters;
		// by simply specifying NULL or 0 we ask libavformat to auto-detect the format 
		// and use a default buffer size

		if (av_open_input_file(&m_FormatCtx, c_url, NULL, 0, NULL) != 0)
		{
//			log_error("Couldn't open file '%s'\n", c_url);
			set_status("error", "NetStream.Play.StreamNotFound");
			return false;
		}

		// Next, we need to retrieve information about the streams contained in the file
		// This fills the streams field of the AVFormatContext with valid information
		if (av_find_stream_info(m_FormatCtx) < 0)
		{
			log_error("Couldn't find stream information from '%s'\n", c_url);
			return false;
		}

		// Find the first video & audio stream
		m_video_index = -1;
		m_audio_index = -1;
		for (int i = 0; i < m_FormatCtx->nb_streams; i++)
		{
			AVCodecContext* enc = m_FormatCtx->streams[i]->codec; 

			switch (enc->codec_type)
			{
				default:
					break;

				case CODEC_TYPE_AUDIO:

					if (m_audio_index < 0)
					{
						m_audio_index = i;
						m_audio_stream = m_FormatCtx->streams[i];
					}
					break;

				case CODEC_TYPE_VIDEO:

					if (m_video_index < 0) 
					{
						m_video_index = i;
						m_video_stream = m_FormatCtx->streams[i];
					}
					break;

				case CODEC_TYPE_DATA:
				case CODEC_TYPE_SUBTITLE:
				case CODEC_TYPE_UNKNOWN:
					break;
			}
		}

		if (m_video_index < 0)
		{
			log_error("Didn't find a video stream from '%s'\n", c_url);
			return false;
		}

		// Get a pointer to the codec context for the video stream
		m_VCodecCtx = m_FormatCtx->streams[m_video_index]->codec;

		// Find the decoder for the video stream
		AVCodec* pCodec = avcodec_find_decoder(m_VCodecCtx->codec_id);
		if (pCodec == NULL)
		{
			m_VCodecCtx = NULL;
			log_error("Decoder not found\n");
			return false;
		}

		// Open codec
		if (avcodec_open(m_VCodecCtx, pCodec) < 0)
		{
			m_VCodecCtx = NULL;
			log_error("Could not open codec\n");
			return false;
		}

		if (m_audio_index >= 0 && get_sound_handler() != NULL)
		{
			// Get a pointer to the audio codec context for the video stream
			m_ACodecCtx = m_FormatCtx->streams[m_audio_index]->codec;

			// Find the decoder for the audio stream
			AVCodec* pACodec = avcodec_find_decoder(m_ACodecCtx->codec_id);
			if (pACodec == NULL)
			{
				log_error("No available AUDIO decoder to process MPEG file: '%s'\n", c_url);
				return false;
			}

			// Open codec
			if (avcodec_open(m_ACodecCtx, pACodec) < 0)
			{
				log_error("Could not open AUDIO codec\n");
				return false;
			}
		}
		return true;
	}


	// it is running in sound mixer thread
	bool as_netstream::decode_audio(const AVPacket& pkt, Sint16** data, int* size)
	{
		bool ok = false;
		int frame_size;
		Uint8* decoder_buf = (Uint8*) malloc((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
		if (avcodec_decode_audio(m_ACodecCtx, (Sint16*) decoder_buf, &frame_size,	pkt.data, pkt.size) >= 0)
		{
			sound_handler* sh = get_sound_handler();
			if (sh)
			{
				sh->cvt(data, size, decoder_buf, frame_size, m_ACodecCtx->channels, m_ACodecCtx->sample_rate);
				ok = true;
			}
		}
		free(decoder_buf);
		return ok;
	}

	// it is running in decoder thread
	Uint8* as_netstream::decode_video(const AVPacket& pkt)
	{
		int w = m_VCodecCtx->width;
		int h = m_VCodecCtx->height;
		Uint8* data = NULL;
		int got = 0;
		AVFrame frame;

		avcodec_decode_video(m_VCodecCtx, &frame, &got, pkt.data, pkt.size);
		if (got)
		{
			// get buf for rgba picture
			// will be freed later in update_video() 
			data = (Uint8*) malloc(avpicture_get_size(PIX_FMT_RGBA32, w, h));

			AVPicture picture1;
			avpicture_fill(&picture1, data, PIX_FMT_RGBA32, w, h);

			struct SwsContext* toRGB_convert_ctx = sws_getContext(
				w, h, m_VCodecCtx->pix_fmt, w, h, PIX_FMT_RGBA32,
				SWS_FAST_BILINEAR, NULL, NULL, NULL);

			assert(toRGB_convert_ctx);

			sws_scale(toRGB_convert_ctx, frame.data, frame.linesize, 0, 0,
				picture1.data, picture1.linesize);

			sws_freeContext(toRGB_convert_ctx);

			// clear video background
			Uint8* ptr = data;
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					swap(ptr, ptr + 2);	// BGR ==> RGB

#ifdef CLEAR_BLUE_VIDEO_BACKGROUND

					Uint16 rg = *(Uint16*) ptr;	// abgr
					if (ptr[3] >= 0xF0 && rg == 0)
					{
						ptr[3] = 0;
					}
#endif

					ptr +=4;
				}
			}
			//printf("video advance time: %d\n", SDL_GetTicks()-t);
		}
		return data;
	}

	// it is running in decoder thread
	void as_netstream::run()
	{
		while (m_is_alive)
		{
			if (m_url.size() == 0)
			{
//				printf("waiting a job...\n");
				m_decoder.wait();
			}

			bool is_open = open_stream(m_url.c_str());
			m_url = "";

			if (is_open)
			{
				set_status("status", "NetStream.Play.Start");

				{
					sound_handler* sound = get_sound_handler();
					if (sound)
					{
						sound->attach_aux_streamer(audio_streamer, this);
					}
				}

				m_start_time = tu_timer::ticks_to_seconds(tu_timer::get_ticks());
				m_video_clock = m_start_time;
				int delay = 0;
				m_break = false;
				m_pause = false;

				while (m_break == false && m_is_alive)
				{
					if (m_pause)
					{
						m_decoder.wait();
						m_video_clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks());
						continue;
					}

					if (m_vq.size() < AV_QUEUE_SIZE && m_aq.size() < AV_QUEUE_SIZE)
					{
						AVPacket pkt;
						int rc = av_read_frame(m_FormatCtx, &pkt);
						if (rc < 0)
						{
							if (m_vq.size() == 0)
							{
								set_status("status", "NetStream.Play.Stop");
								break;
							}
						}

						if (pkt.stream_index == m_video_index)
						{
							m_vq.push(pkt);
						}
						else
						if (pkt.stream_index == m_audio_index)
						{
							if (get_sound_handler())
							{
								m_aq.push(pkt);
							}
						}
						else
						{
							continue;
						}
					}

					m_current_clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks());

					// it is time
					if (m_current_clock >= m_video_clock)
					{
						delay = 0;
						gc_ptr<av_packet> packet;
						if (m_vq.pop(&packet))
						{
							const AVPacket& pkt = packet->get_packet();

							// update video clock with pts, if present
							if (pkt.dts != AV_NOPTS_VALUE)
							{
								double pts = as_double(m_video_stream->codec->time_base) * pkt.dts;
								m_video_clock = m_start_time + pts;
							}

							// update video clock for next frame
							double frame_delay = as_double(m_video_stream->codec->time_base);
							m_video_clock += frame_delay;

							set_video_data(decode_video(packet->get_packet()));
						}
					}
					else
					{
						delay = int(1000 * (m_video_clock - m_current_clock));
					}

					// Don't hog the CPU.
					// Queues have filled, video frame have shown
					// now it is possible and to have a rest
					if (delay > 0 && (m_vq.size() >= AV_QUEUE_SIZE || m_aq.size() >= AV_QUEUE_SIZE))
					{
						//	printf("video delay=%d\n", delay);
						tu_timer::sleep(delay);
					}
				}

				sound_handler* sound = get_sound_handler();
				if (sound)
				{
					sound->detach_aux_streamer(this);
				}

				close_stream();
			}
		}
	}

	int as_netstream::get_width() const
	{
		if (m_VCodecCtx)
		{
			return m_VCodecCtx->width;
		}
		return 0;
	}

	int as_netstream::get_height() const
	{
		if (m_VCodecCtx)
		{
			return m_VCodecCtx->height;
		}
		return 0;
	}

	Uint8* as_netstream::get_video_data()
	{
		tu_autolock locker(m_lock_data);
		Uint8* video_data = m_data;
		m_data = NULL;
		return video_data;
	}

	void as_netstream::set_video_data(Uint8* data)
	{
		tu_autolock locker(m_lock_data);
		if (m_data)
		{
			free(m_data);
		}
		m_data = data;
	}

	void as_netstream::seek(double seek_time)
	{
		return;
		//TODO
/*		double clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - m_start_clock;
		clock += seek_time;
//		int64_t timestamp = (int64_t) clock * AV_TIME_BASE;

//		int flg = seek_time < 0 ? AVSEEK_FLAG_BACKWARD : 0;
		int rc = av_seek_frame(m_FormatCtx, -1, AV_TIME_BASE, 0);
		if (rc < 0)
		{
			log_error("could not seek to position rc=%d\n", rc);
		}*/
	}

	// Specifies how long to buffer messages before starting to display the stream.
	void as_netstream::setBufferTime()
	{
		log_msg("%s:unimplemented \n", __FUNCTION__);
	}

	//	The position of the playhead, in seconds.
	double as_netstream::time() const
	{
		return (double) m_video_clock;
	}

	void netstream_close(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		ns->close();
	}

	void netstream_pause(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		// mode: -1 ==> toogle, 0==> pause, 1==> play
		int mode = -1;
		if (fn.nargs > 0)
		{
			mode = fn.arg(0).to_bool() ? 0 : 1;
		}
		ns->pause(mode);	// toggle mode
	}

	void netstream_play(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs < 1)
		{
			log_error("NetStream play needs args\n");
			return;
		}

		ns->play(fn.arg(0).to_tu_string());
	}

	// Seeks the keyframe closest to the specified number of seconds from the beginning
	// of the stream.
	void netstream_seek(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs < 1)
		{
			log_error("NetStream seek needs args\n");
			return;
		}

		ns->seek(fn.arg(0).to_number());
	}

	void netstream_setbuffertime(const fn_call& /*fn*/)
	{
		log_msg("%s:unimplemented \n", __FUNCTION__);
	}

	void netstream_time(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_double(ns->time());
	}


	void	as_global_netstream_ctor(const fn_call& fn)
	// Constructor for ActionScript class NetStream.
	{
		as_object* netstream = new as_netstream(fn.get_player());

		// properties
		netstream->builtin_member("time", as_value(netstream_time, as_value()));

		// methods
		netstream->builtin_member("close", netstream_close);
		netstream->builtin_member("pause", netstream_pause);
		netstream->builtin_member("play", netstream_play);
		netstream->builtin_member("seek", netstream_seek);
		netstream->builtin_member("setbuffertime", netstream_setbuffertime);

		fn.result->set_as_object(netstream);
	}


} // end of gameswf namespace

#else

#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_log.h"

namespace gameswf
{
	void as_global_netstream_ctor(const fn_call& fn)
	{
		log_error("video requires FFMPEG library\n");
	}
}

#endif
