// gameswf_netstream.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf_netstream.h"
#include "gameswf_render.h"
#include "gameswf_video_base.h"
#include "base/tu_timer.h"

namespace gameswf {
 
NetStream::NetStream():
	m_video_index(-1),
	m_audio_index(-1),
	m_VCodecCtx(NULL),
	m_ACodecCtx(NULL),
	m_FormatCtx(NULL),
	m_Frame(NULL),
	m_go(false),
	m_yuv(NULL),
	m_video_clock(0),
	m_pause(false),
	m_ns(NULL),
	m_qaudio(20),
	m_qvideo(20)
{
}

NetStream::~NetStream()
{
	close();
}

void NetStream::set_status(const char* code)
{
	netstream_as_object* ns = (netstream_as_object*) m_ns;
	if (ns)
	{
		ns->set_member("onStatus_Code", code);
//		push_video_event(ns);
	}
}

void NetStream::pause(int mode)
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

void NetStream::close()
{
	if (m_go)
	{
		// terminate thread
		m_go = false;

		// wait till thread is complete before main continues
		SDL_WaitThread(m_thread, NULL);
	}

	sound_handler* s = get_sound_handler();
	if (s)
	{
		s->detach_aux_streamer((void*) this);
	}

	if (m_Frame) av_free(m_Frame);
	m_Frame = NULL;

	if (m_VCodecCtx) avcodec_close(m_VCodecCtx);
	m_VCodecCtx = NULL;

	if (m_ACodecCtx) avcodec_close(m_ACodecCtx);
	m_ACodecCtx = NULL;

	if (m_FormatCtx) av_close_input_file(m_FormatCtx);
	m_FormatCtx = NULL;

	render::delete_YUV_video(m_yuv);
	m_yuv = NULL;

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

int
NetStream::play(const char* c_url)
{
	// This registers all available file formats and codecs 
	// with the library so they will be used automatically when
	// a file with the corresponding format/codec is opened

	// Is it already playing ?
	if (m_go)
	{
		return 0;
	}

	av_register_all();

	// Open video file
	// The last three parameters specify the file format, buffer size and format parameters;
	// by simply specifying NULL or 0 we ask libavformat to auto-detect the format 
	// and use a default buffer size

	if (av_open_input_file(&m_FormatCtx, c_url, NULL, 0, NULL) != 0)
	{
	  log_error("Couldn't open file '%s'\n", c_url);
		set_status("NetStream.Play.StreamNotFound");
		return -1;
	}

	// Next, we need to retrieve information about the streams contained in the file
	// This fills the streams field of the AVFormatContext with valid information
	if (av_find_stream_info(m_FormatCtx) < 0)
	{
    log_error("Couldn't find stream information from '%s'\n", c_url);
		return -1;
	}

//	m_FormatCtx->pb.eof_reached = 0;
//	av_read_play(m_FormatCtx);

	// Find the first video & audio stream
	m_video_index = -1;
	m_audio_index = -1;
	for (int i = 0; i < m_FormatCtx->nb_streams; i++)
	{
		AVCodecContext* enc = m_FormatCtx->streams[i]->codec; 

		switch (enc->codec_type)
		{
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
		return -1;
	}

	// Get a pointer to the codec context for the video stream
  m_VCodecCtx = m_FormatCtx->streams[m_video_index]->codec;

	// Find the decoder for the video stream
	AVCodec* pCodec = avcodec_find_decoder(m_VCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		m_VCodecCtx = NULL;
		log_error("Decoder not found\n");
		return -1;
	}

	// Open codec
	if (avcodec_open(m_VCodecCtx, pCodec) < 0)
	{
		log_error("Could not open codec\n");
	}

	// Allocate a frame to store the decoded frame in
	m_Frame = avcodec_alloc_frame();
	
	// Determine required buffer size and allocate buffer
	m_yuv = render::create_YUV_video(m_VCodecCtx->width,	m_VCodecCtx->height);

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
			return -1;
		}
        
    // Open codec
    if (avcodec_open(m_ACodecCtx, pACodec) < 0)
		{
			log_error("Could not open AUDIO codec\n");
			return -1;
		}
	
		s->attach_aux_streamer(audio_streamer, (void*) this);

	}

	m_pause = false;

	m_thread = SDL_CreateThread(NetStream::av_streamer, this);
	if (m_thread == NULL)
	{
		return -1;
	}

	return 0;
}

// decoder thread
int NetStream::av_streamer(void* arg)
{
	
	NetStream* ns = static_cast<NetStream*>(arg);
	ns->set_status("NetStream.Play.Start");

	raw_videodata_t* unqueued_data = NULL;
	raw_videodata_t* video = NULL;

	ns->m_video_clock = 0;

	int delay = 0;
	ns->m_start_clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks());
	ns->m_go = true;
	while (ns->m_go)
	{
		if (ns->m_pause)
		{
			double t = tu_timer::ticks_to_seconds(tu_timer::get_ticks());
			SDL_Delay(100);
			ns->m_start_clock += tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - t;
			continue;
		}

		unqueued_data = ns->read_frame(unqueued_data);
		if ((int) unqueued_data < 0)
		{
			unqueued_data = NULL;
			if (ns->m_qvideo.size() == 0)
			{
				break;
			}
		}

		if (ns->m_qvideo.size() > 0)
		{
			video = ns->m_qvideo.front();
			double clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - ns->m_start_clock;
			double video_clock = video->m_pts;

			if (clock >= video_clock)
			{
				ns->m_yuv->update(video->m_data);
				ns->m_qvideo.pop();
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
			if (unqueued_data && delay > 0)
			{
				SDL_Delay(delay);
			}
		}
	}

	ns->set_status("NetStream.Play.Stop");
	return 0;
}

// audio callback is running in sound handler thread
void NetStream::audio_streamer(void *owner, Uint8 *stream, int len)
{
	NetStream* ns = static_cast<NetStream*>(owner);

	if (ns->m_pause)
	{
		return;
	}

	while (len > 0 && ns->m_qaudio.size() > 0)
	{
		raw_videodata_t* samples = ns->m_qaudio.front();

		int n = imin(samples->m_size, len);
		memcpy(stream, samples->m_ptr, n);
		stream += n;
		samples->m_ptr += n;
		samples->m_size -= n;
		len -= n;

		if (samples->m_size == 0)
		{
			ns->m_qaudio.pop();
			delete samples;
		}
	}
}

raw_videodata_t* NetStream::read_frame(raw_videodata_t* unqueued_data)
{
	raw_videodata_t* ret = NULL;
	if (unqueued_data)
	{
		if (unqueued_data->m_stream_index == m_audio_index)
		{
			sound_handler* s = get_sound_handler();
			if (s)
			{
				ret = m_qaudio.push(unqueued_data) ? NULL : unqueued_data;
			}
		}
		else
		if (unqueued_data->m_stream_index == m_video_index)
		{
			ret = m_qvideo.push(unqueued_data) ? NULL : unqueued_data;
		}
		else
		{
			printf("read_frame: not audio & video stream\n");
		}

		return ret;
	}

	AVPacket packet;
	int rc = av_read_frame(m_FormatCtx, &packet);
	if (rc >= 0)
	{
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

			    raw_videodata_t* raw = new raw_videodata_t;
					raw->m_data = (uint8_t*) adjusted_data;
					raw->m_ptr = raw->m_data;
					raw->m_size = n;
					raw->m_stream_index = m_audio_index;

					ret = m_qaudio.push(raw) ? NULL : raw;
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

				raw_videodata_t* video = new raw_videodata_t;
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
				ret = m_qvideo.push(video) ? NULL : video;
			}
		}
		av_free_packet(&packet);
	}
	else
	{
		return (raw_videodata_t*) rc;
	}

	return ret;
}


YUV_video* NetStream::get_video()
{
	return m_yuv;
}

void NetStream::seek(double seek_time)
{
	return;
	double clock = tu_timer::ticks_to_seconds(tu_timer::get_ticks()) - m_start_clock;
	clock += seek_time;
	int64_t timestamp = (int64_t) clock * AV_TIME_BASE;

	int flg = seek_time < 0 ? AVSEEK_FLAG_BACKWARD : 0;
	int rc = av_seek_frame(m_FormatCtx, -1, AV_TIME_BASE, 0);
	if (rc < 0)
	{
		printf("could not seek to position rc=%d\n", rc);
	}
}

void
NetStream::setBufferTime()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

/*
void
netstream_new(const fn_call& fn)
{
    netstream_as_object *netstream_obj = new netstream_as_object;

    netstream_obj->set_member("close", &netstream_close);
    netstream_obj->set_member("pause", &netstream_pause);
    netstream_obj->set_member("play", &netstream_play);
    netstream_obj->set_member("seek", &netstream_seek);
    netstream_obj->set_member("setbuffertime", &netstream_setbuffertime);

    fn.result->set(netstream_obj);
}


void	netstream_as_object::set_member(const tu_stringi& name, const as_value& val)
{
		log_error("error: video_stream_instance::set_member('%s', '%s') not implemented yet\n",
				name.c_str(),
				val.to_string());
}

bool netstream_as_object::get_member(const tu_stringi& name, as_value* val)
{
	if (name == "attachVideo")
	{
//			val->set(attach_video);
		return true;
	}
	return false;
}
*/

void netstream_close(const fn_call& fn)
{
	assert(dynamic_cast<netstream_as_object*>(fn.this_ptr));
	netstream_as_object* ns = static_cast<netstream_as_object*>(fn.this_ptr);
	ns->obj.close();
}

void netstream_pause(const fn_call& fn)
{
	assert(dynamic_cast<netstream_as_object*>(fn.this_ptr));
	netstream_as_object* ns = static_cast<netstream_as_object*>(fn.this_ptr);
	
	// mode: -1 ==> toogle, 0==> pause, 1==> play
	int mode = -1;
	if (fn.nargs > 0)
	{
		mode = fn.arg(0).to_bool() ? 0 : 1;
	}
	ns->obj.pause(mode);	// toggle mode
}

void netstream_play(const fn_call& fn)
{
	assert(dynamic_cast<netstream_as_object*>(fn.this_ptr));
	netstream_as_object* ns = static_cast<netstream_as_object*>(fn.this_ptr);
	
	if (fn.nargs < 1)
	{
    log_error("NetStream play needs args\n");
    return;
	}

	if (ns->obj.play(fn.arg(0).to_string()) != 0)
	{
		ns->obj.close();
	};
}

void netstream_seek(const fn_call& fn)
{
	assert(dynamic_cast<netstream_as_object*>(fn.this_ptr));
	netstream_as_object* ns = static_cast<netstream_as_object*>(fn.this_ptr);
	
	if (fn.nargs < 1)
	{
    log_error("NetStream seek needs args\n");
    return;
	}

	ns->obj.seek(fn.arg(0).to_number());
}

void netstream_setbuffertime(const fn_call& /*fn*/) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

} // end of gameswf namespace

