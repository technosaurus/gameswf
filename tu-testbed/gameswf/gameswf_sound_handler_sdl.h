// gameswf_sound_handler_sdl.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef SOUND_HANDLER_SDL_H
#define SOUND_HANDLER_SDL_H

#ifdef USE_FFMPEG
#include <ffmpeg/avformat.h>
#endif

#include <SDL_audio.h>
#include <SDL_thread.h>

#include "gameswf.h"
#include "base/container.h"
#include "base/smart_ptr.h"
#include "gameswf_mutex.h"
#include "gameswf_log.h"

// Used to hold the info about active sounds

namespace gameswf
{

	struct sound;
	struct active_sound;

	// Use SDL and ffmpeg to handle sounds.
	struct SDL_sound_handler : public sound_handler
	{

		// NetStream audio callbacks
		hash< as_object_interface* /* netstream */, aux_streamer_ptr /* callback */> m_aux_streamer;
		hash< int, smart_ptr<sound> > m_sound;
		int m_defvolume;
		tu_mutex* m_mutex;

#ifdef USE_FFMPEG
		AVCodec* m_MP3_codec;
#endif

		// SDL_audio specs
		SDL_AudioSpec m_audioSpec;

		// Is sound device opened?
		bool soundOpened;

		SDL_sound_handler();
		virtual ~SDL_sound_handler();

		// Called to create a sample.
		virtual int	create_sound(void* data, int data_bytes,
			int sample_count, format_type format,
			int sample_rate, bool stereo);

		virtual void append_sound(int sound_handle, void* data, int data_bytes);

		// Play the index'd sample.
		virtual void	play_sound(int sound_handle, int loop_count);

		virtual void	set_default_volume(int vol);

		virtual void	stop_sound(int sound_handle);

		// this gets called when it's done with a sample.
		virtual void	delete_sound(int sound_handle);

		// this will stop all sounds playing.
		virtual void	stop_all_sounds();

		// returns the sound volume level as an integer from 0 to 100.
		virtual int	get_volume(int sound_handle);

		virtual void	set_volume(int sound_handle, int volume);

		virtual void	attach_aux_streamer(gameswf::sound_handler::aux_streamer_ptr ptr,
			as_object_interface* netstream);
		virtual void	detach_aux_streamer(as_object_interface* netstream);

		// Converts input data to the SDL output format.
		virtual void cvt(short int** adjusted_data, int* adjusted_size, unsigned char* data, int size, 
			int channels, int freq);

	};

	// Used to hold the sounddata
	struct sound: public gameswf::ref_counted
	{
		sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
			int sample_rate, bool stereo, int vol);
		~sound();

		inline void clear_playlist()
		{
			m_playlist.clear();
		}

		inline int get_volume() const
		{
			return m_volume;
		}

		inline void set_volume(int vol)
		{
			m_volume = vol;
		}

		void append(void* data, int size, SDL_sound_handler* handler);
		void play(int loops, SDL_sound_handler* handler);
		bool mix(Uint8* stream, int len);

//	private:

		Uint8* m_data;
		int m_size;
		int m_volume;
		gameswf::sound_handler::format_type m_format;
		int m_sample_count;
		int m_sample_rate;
		bool m_stereo;
		array < smart_ptr<active_sound> > m_playlist;
	};


	struct active_sound: public gameswf::ref_counted
	{
		active_sound(sound* parent, int loops):
		m_pos(0),
		m_loops(loops),
		m_parent(parent),
		m_size(0),
		m_data(NULL),
		m_decoded(0)
	{
		m_handler = (SDL_sound_handler*) get_sound_handler();
		assert(m_handler);
		
		if (m_parent->m_format == sound_handler::FORMAT_MP3)
		{
#ifdef USE_FFMPEG
			// Init MP3 the parser
			m_parser = av_parser_init(CODEC_ID_MP3);
			m_cc = avcodec_alloc_context();
			if (m_cc == NULL)
			{
				log_error("Could not alloc MP3 codec context\n");
				return;
			}

			if (avcodec_open(m_cc, m_handler->m_MP3_codec) < 0)
			{
				log_error("Could not open MP3 codec\n");
				avcodec_close(m_cc);
				return;
			}
#endif
		}
	}

	~active_sound()
	{

		// memory was allocated in decode()
		free(m_data);

		if (m_parent->m_format == sound_handler::FORMAT_MP3)
		{
#ifdef USE_FFMPEG
			avcodec_close(m_cc);
			av_parser_close(m_parser);
#endif
		}
	}

	int decode(int mix_buflen, int* newsize, Uint8** newdata)
	{
		*newdata = NULL;
		*newsize = 0;
		switch (m_parent->m_format)
		{
			default:
				log_error("unknown sound format %d\n", m_parent->m_format);
				break;

			case sound_handler::FORMAT_NATIVE16:
			{
				float	dup = float(m_handler->m_audioSpec.freq) / float(m_parent->m_sample_rate);
				int samples = int(float(mix_buflen) / dup / (m_parent->m_stereo ? 1.0f : 2.0f));
				samples = imin(samples, m_parent->m_size - m_decoded);
				*newsize = samples;
				*newdata = (Uint8*) malloc(samples);

				memcpy(*newdata, m_parent->m_data + m_decoded, samples);
				m_decoded += samples;
				return true;
			}

			case sound_handler::FORMAT_MP3:
			{
#ifdef USE_FFMPEG
				int asize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;

				int bufsize = 2 * asize;
				Uint8* buf = (Uint8*) malloc(bufsize);
				int buflen = 0;

				while (m_decoded < m_parent->m_size)
				{
					uint8_t* frame;
					int framesize;
					int decoded = av_parser_parse(m_parser, m_cc, &frame, &framesize,
						m_parent->m_data + m_decoded , m_parent->m_size - m_decoded, 0, 0);

					if (decoded < 0)
					{
						log_error("Error while decoding MP3-stream\n");
						free(buf);
						buf = NULL;
						buflen = 0;
						break;
					}

					m_decoded += decoded;

					if (framesize > 0)
					{
						int len = 0;
						if (avcodec_decode_audio(m_cc, (int16_t*) (buf + buflen), &len, frame, framesize) >= 0)
						{
							buflen += len;

							if (buflen > bufsize - asize)
							{
								bufsize += asize;
								buf = (Uint8*) realloc(buf, bufsize);
							}

							// Whether is it time to stop ?
							float	dup = float(m_handler->m_audioSpec.freq) / float(m_parent->m_sample_rate);
							int	output_sample_count = int(buflen * dup * (m_parent->m_stereo ? 1 : 2));
							if (output_sample_count >= mix_buflen)
							{
								break;
							}
						}
					}
				}

				*newdata = buf;
				*newsize = buflen;
#endif
			}
		}
		return true;
	}

	bool mix(Uint8* buf , int len)
	{
		memset(buf, 0, len);

		// m_pos ==> current position of sound which should be played
		// len ==> mix buffer size
		// m_size==> current sound size
		// (size of stream sound may be be increased)
		// If is not enough sound then try decode it
		assert(m_pos <= m_size);
		assert(m_decoded <= m_parent->m_size);

		if (m_decoded < m_parent->m_size)
		{

			int decoded_size = 0;
			Uint8* decoded_data = NULL;
			decode(len, &decoded_size, &decoded_data);
			if (decoded_size > 0)
			{

				int16* cvt_data;
				int cvt_size;
				m_handler->cvt(&cvt_data, &cvt_size, decoded_data, decoded_size, 
					m_parent->m_stereo ? 2 : 1, m_parent->m_sample_rate);

				free(decoded_data);

				// copy the rest of buf to the beginning
				// then copy newdata
				int rest = m_size - m_pos;
				m_size = cvt_size + rest;
				Uint8* new_m_data = (Uint8*) malloc(m_size);

				memcpy(new_m_data, m_data + m_pos,  rest);	// the rest
				memcpy(new_m_data + rest, (Uint8*) cvt_data, cvt_size);	// cvt data

				free(cvt_data);
				free(m_data);
				m_data = new_m_data;

				m_pos = 0;
			}
		}

		bool next_it = true;
		Uint8* ptr = buf;

		while (ptr - buf < len)
		{
			int left = len - (ptr - buf);
			int n = imin(m_size - m_pos, left);
			assert(n > 0);
			memcpy(ptr, m_data + m_pos, n);

			ptr += n;
			m_pos = (m_pos + n)  % m_size;
			if (m_pos == 0)	// is buf played ?
			{
				if (m_loops == 0)
				{
					if (m_decoded >= m_parent->m_size)
					{
						next_it = false;
					}
					else
					{
						m_size = 0;
					}
					break;
				}
				if (m_loops > 0)
				{
					m_loops--;
				}
			}
		}
		return next_it;
	}

	private:

		int m_pos;
		int m_loops;
		int m_size;
		Uint8* m_data;
		sound* m_parent;
		int m_decoded_size;

#ifdef USE_FFMPEG
		AVCodecContext *m_cc;
		AVCodecParserContext* m_parser;
#endif

		int m_bufsize;

		int m_decoded;
		SDL_sound_handler* m_handler;

	};

}

#endif // SOUND_HANDLER_SDL_H



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
