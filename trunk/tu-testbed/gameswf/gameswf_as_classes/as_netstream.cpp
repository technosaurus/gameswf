// as_netstream.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_FFMPEG

#include "../gameswf_as_classes/as_netstream.h"
#include "../gameswf_render.h"
#include "../gameswf_video_base.h"
#include "../gameswf_mutex.h"
#include "base/tu_timer.h"

namespace gameswf
{

	// it is running in decoder thread
	static int netstream_server(void* arg)
	{
		as_netstream* ns = (as_netstream*) arg;
		ns->run();
//		printf("~netstream_server\n");
		return 0;
	}

	// audio callback is running in sound handler thread
	static void audio_streamer(as_object_interface* netstream, Uint8* stream, int len)
	{
		as_netstream* ns = netstream->cast_to_as_netstream();
		assert(ns);
		ns->audio_callback(stream, len);
	}

	as_netstream::as_netstream():
		m_FormatCtx(NULL),
		m_VCodecCtx(NULL),
		m_ACodecCtx(NULL),
		m_Frame(NULL),
		m_video_clock(0),
		m_video_index(-1),              
		m_audio_index(-1),
		m_url(""),
		m_is_alive(true),
		m_pause(false),
		m_qaudio(20),
		m_qvideo(20)              
	{
		av_register_all();

		m_yuv = render::create_YUV_video();

		m_thread = tu_create_thread(netstream_server, this);
		assert(m_thread);
	}

	as_netstream::~as_netstream()
	{
		m_is_alive = false;
		m_decoder.signal();
		tu_wait_thread(m_thread);
		m_yuv = NULL;
//		printf("~as_netstream\n");
	}

	void as_netstream::play(const char* url)
	{
		m_url = url;
		m_break = true;
		m_decoder.signal();
	}

	void as_netstream::close()
	{
		m_break = true;
	}

	void as_netstream::audio_callback(Uint8* stream, int len)
	{
		if (m_pause)
		{
			return;
		}

		while (len > 0 && m_qaudio.size() > 0)
		{
			av_data* samples = m_qaudio.front();

			int n = imin(samples->m_size, len);
			memcpy(stream, samples->m_ptr, n);
			stream += n;
			samples->m_ptr += n;
			samples->m_size -= n;
			len -= n;

			if (samples->m_size == 0)
			{
				m_qaudio.pop();
				delete samples;
			}
		}
	}

	// it is running in decoder thread
	void as_netstream::set_status(const char* level, const char* code)
	{
		if (m_is_alive)
		{
			locker lock(get_gameswf_mutex());

			as_value function;
			if (get_member("onStatus", &function))
			{
				smart_ptr<as_object> infoObject = new as_object();
				infoObject->set_member("level", level);
				infoObject->set_member("code", code);

				as_environment* env = function.to_as_function()->m_env;
				assert(env);

				env->push_val(infoObject.get_ptr());
				call_method(function, env, NULL, 1, env->get_top_index());
				env->drop(1);
			}
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
	}

	// it is running in decoder thread
	void as_netstream::close_stream()
	{
		if (m_Frame) av_free(m_Frame);
		m_Frame = NULL;

		if (m_VCodecCtx) avcodec_close(m_VCodecCtx);
		m_VCodecCtx = NULL;

		if (m_ACodecCtx) avcodec_close(m_ACodecCtx);
		m_ACodecCtx = NULL;

		if (m_FormatCtx) av_close_input_file(m_FormatCtx);
		m_FormatCtx = NULL;

		while (m_qvideo.size() > 0)
		{
			delete m_qvideo.front();
			m_qvideo.pop();
		}

		while (m_qaudio.size() > 0)
		{
			delete m_qaudio.front();
			m_qaudio.pop();
		}
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
			case CODEC_TYPE_AUDIO:
				if (m_audio_index < 0) {
					m_audio_index = i;
					m_audio_stream = m_FormatCtx->streams[i];
				}
				break;

			case CODEC_TYPE_VIDEO:
				if (m_video_index < 0) {
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
			log_error("Could not open codec\n");
		}

		// Allocate a frame to store the decoded frame in
		m_Frame = avcodec_alloc_frame();

		// Determine required buffer size and allocate buffer
		m_yuv_mutex.lock();
		m_yuv->resize(m_VCodecCtx->width,	m_VCodecCtx->height);
		m_yuv_mutex.unlock();
		if (m_yuv == NULL)
		{
			log_error("No available video render\n");
			return false;
		}

		sound_handler* s = get_sound_handler();
		if (m_audio_index >= 0 && s != NULL)
		{
			// Get a pointer to the audio codec context for the video stream
			m_ACodecCtx = m_FormatCtx->streams[m_audio_index]->codec;

			// Find the decoder for the audio stream
			AVCodec* pACodec = avcodec_find_decoder(m_ACodecCtx->codec_id);
			if(pACodec == NULL)
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

	// it is running in decoder thread
	void as_netstream::run()
	{
		while (m_is_alive)
		{
			if (m_url == "")
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

				m_unqueued_data = NULL;
				av_data* video = NULL;

				m_video_clock = 0;

				int delay = 0;
				m_start_clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks());

				m_break = false;
				m_pause = false;
				while (m_break == false && m_is_alive)
				{
					if (m_pause)
					{
						double t = tu_timer::ticks_to_seconds(tu_timer::get_ticks());
						tu_delay(100);
						m_start_clock += tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - t;
						continue;
					}

					bool rc = read_frame();
					if (rc == false)
					{
						if (m_qvideo.size() == 0)
						{
							set_status("status", "NetStream.Play.Stop");
							break;
						}
					}

					if (m_qvideo.size() > 0)
					{
						video = m_qvideo.front();
						double clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - m_start_clock;
						double video_clock = video->m_pts;

						if (clock >= video_clock)
						{
							m_yuv->update(video->m_data);
							m_qvideo.pop();
							delete video;
							delay = 0;
						}
						else
						{
							delay = int(1000 * (video_clock - clock));
						}

						// Don't hog the CPU.
						// Queues have filled, video frame have shown
						// now it is possible and to have a rest
						if (m_unqueued_data && delay > 0)
						{
							tu_delay(delay);
						}
					}
				}

				{
					sound_handler* sound = get_sound_handler();
					if (sound)
					{
						sound->detach_aux_streamer(this);
					}
				}
	
				close_stream();
			}
		}
	}

	// it is running in decoder thread
	bool as_netstream::read_frame()
	{
		if (m_unqueued_data)
		{
			if (m_unqueued_data->m_stream_index == m_audio_index)
			{
				sound_handler* s = get_sound_handler();
				if (s)
				{
					if (m_qaudio.push(m_unqueued_data))
					{
						m_unqueued_data = NULL;
					}
				}
			}
			else
				if (m_unqueued_data->m_stream_index == m_video_index)
				{
					if (m_qvideo.push(m_unqueued_data))
					{
						m_unqueued_data = NULL;
					}
				}
				else
				{
					log_error("read_frame: not audio & video stream\n");
				}

				return true;
		}

		AVPacket packet;
		int rc = av_read_frame(m_FormatCtx, &packet);
		if (rc < 0)
		{
			return false;
		}

		if (packet.stream_index == m_audio_index)
		{
			sound_handler* s = get_sound_handler();
			if (s)
			{
				int frame_size;
				uint8_t* ptr = (uint8_t*) malloc((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
				if (avcodec_decode_audio(m_ACodecCtx, (int16_t*) ptr, &frame_size, packet.data, packet.size) >= 0)
				{

					int16_t*	adjusted_data = 0;
					int n = 0;
					s->cvt(&adjusted_data, &n, ptr, frame_size, m_ACodecCtx->channels, m_ACodecCtx->sample_rate);

					av_data* raw = new av_data;
					raw->m_data = (uint8_t*) adjusted_data;
					raw->m_ptr = raw->m_data;
					raw->m_size = n;
					raw->m_stream_index = m_audio_index;

					if (m_qaudio.push(raw) == false)
					{
						m_unqueued_data = raw;			
					}
				}
				free(ptr);
			}
		}
		else
		if (packet.stream_index == m_video_index)
		{
			int got = 0;
			avcodec_decode_video(m_VCodecCtx, m_Frame, &got, packet.data, packet.size);
			if (got)
			{
				if (m_VCodecCtx->pix_fmt != PIX_FMT_YUV420P)
				{
					//				img_convert((AVPicture*) pFrameYUV, PIX_FMT_YUV420P, (AVPicture*) pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
					assert(0);	// TODO
				}

				av_data* video = new av_data;
				video->m_data = (uint8_t*) malloc(m_yuv->size());
				video->m_ptr = video->m_data;
				video->m_stream_index = m_video_index;

				// set presentation timestamp
				if (packet.dts != AV_NOPTS_VALUE)
				{
					video->m_pts = as_double(m_video_stream->time_base) * packet.dts;
				}

				if (video->m_pts != 0)
				{	
					// update video clock with pts, if present
					m_video_clock = video->m_pts;
				}
				else
				{
					video->m_pts = m_video_clock;
				}

				// update video clock for next frame
				double frame_delay = as_double(m_video_stream->codec->time_base);

				// for MPEG2, the frame can be repeated, so we update the clock accordingly
				frame_delay += m_Frame->repeat_pict * (frame_delay * 0.5);

				m_video_clock += frame_delay;

				int copied = 0;
				uint8_t* ptr = video->m_data;
				for (int i = 0; i < 3 ; i++)
				{
					int shift = (i == 0 ? 0 : 1);
					uint8_t* yuv_factor = m_Frame->data[i];
					int h = m_VCodecCtx->height >> shift;
					int w = m_VCodecCtx->width >> shift;
					for (int j = 0; j < h; j++)
					{
						copied += w;
						assert(copied <= m_yuv->size());
						memcpy(ptr, yuv_factor, w);
						yuv_factor += m_Frame->linesize[i];
						ptr += w;
					}
				}
				video->m_size = copied;
				if (m_qvideo.push(video) == false)
				{
					m_unqueued_data = video;
				}
			}
		}
		av_free_packet(&packet);

		return true;
	}

	YUV_video* as_netstream::get_video()
	{
		YUV_video* tmp = NULL;
		m_yuv_mutex.lock();
		if (m_yuv != NULL)
		{
			if (m_yuv->is_updated())
			{
				tmp = m_yuv;
			}
		}
		m_yuv_mutex.unlock();
		return tmp;
	}

	void as_netstream::seek(double seek_time)
	{
		double clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - m_start_clock;
		clock += seek_time;
//		int64_t timestamp = (int64_t) clock * AV_TIME_BASE;

//		int flg = seek_time < 0 ? AVSEEK_FLAG_BACKWARD : 0;
		int rc = av_seek_frame(m_FormatCtx, -1, AV_TIME_BASE, 0);
		if (rc < 0)
		{
			log_error("could not seek to position rc=%d\n", rc);
		}
	}

	void as_netstream::setBufferTime()
	{
		log_msg("%s:unimplemented \n", __FUNCTION__);
	}

	void netstream_close(const fn_call& fn)
	{
		as_netstream* ns = fn.this_ptr->cast_to_as_netstream();
		assert(ns);
		ns->close();
	}

	void netstream_pause(const fn_call& fn)
	{
		as_netstream* ns = fn.this_ptr->cast_to_as_netstream();
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
		as_netstream* ns = fn.this_ptr->cast_to_as_netstream();
		assert(ns);

		if (fn.nargs < 1)
		{
			log_error("NetStream play needs args\n");
			return;
		}

		ns->play(fn.arg(0).to_tu_string());
	}

	void netstream_seek(const fn_call& fn)
	{
		as_netstream* ns = fn.this_ptr->cast_to_as_netstream();
		assert(ns);

		if (fn.nargs < 1)
		{
			log_error("NetStream seek needs args\n");
			return;
		}

		ns->seek(fn.arg(0).to_number());
	}

	void netstream_setbuffertime(const fn_call& /*fn*/) {
		log_msg("%s:unimplemented \n", __FUNCTION__);
	}

	void	as_global_netstream_ctor(const fn_call& fn)
	// Constructor for ActionScript class NetStream.
	{
		smart_ptr<as_object>	netstream_obj(new as_netstream);

		// methods
		netstream_obj->set_member("close", &netstream_close);
		netstream_obj->set_member("pause", &netstream_pause);
		netstream_obj->set_member("play", &netstream_play);
		netstream_obj->set_member("seek", &netstream_seek);
		netstream_obj->set_member("setbuffertime", &netstream_setbuffertime);

		fn.result->set_as_object_interface(netstream_obj.get_ptr());
	}

} // end of gameswf namespace

#else

#include "../gameswf_action.h"
#include "../gameswf_log.h"

namespace gameswf
{
	void as_global_netstream_ctor(const fn_call& fn)
	{
		log_error("video requires FFMPEG library\n");
	}
}

#endif
