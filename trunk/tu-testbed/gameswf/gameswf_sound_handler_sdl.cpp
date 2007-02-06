// gameswf_sound_handler.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf_sound_handler_sdl.h"
#include <cmath>
#include <vector>
#include <SDL/SDL.h>   

namespace gameswf
{

	static void sdl_audio_callback(void *udata, Uint8 *stream, int len); // SDL C audio handler

	SDL_sound_handler::SDL_sound_handler():
		m_defvolume(100),
		m_mutex(NULL),
		soundOpened(true)
	{
		m_mutex = SDL_CreateMutex();

		// This is our sound settings
		audioSpec.freq = 44100;
		audioSpec.format = AUDIO_S16SYS;
		audioSpec.channels = 2;
		audioSpec.callback = sdl_audio_callback;
		audioSpec.userdata = this;
		audioSpec.samples = 4096;

		if (SDL_OpenAudio(&audioSpec, NULL) < 0 )
		{
			printf("Unable to START SOUND: %s\n", SDL_GetError());
			soundOpened = false;
			return;
		}

#ifdef USE_FFMPEG
		avcodec_init();
		avcodec_register_all();
#endif

		SDL_PauseAudio(1);
	}

	SDL_sound_handler::~SDL_sound_handler()
	{
		if (soundOpened) SDL_CloseAudio();
		m_sound.clear();
		SDL_DestroyMutex(m_mutex);
	}

	int	SDL_sound_handler::create_sound(
		void* data,
		int data_bytes,
		int sample_count,
		format_type format,
		int sample_rate,
		bool stereo)
		// Called to create a sample.  We'll return a sample ID that
		// can be use for playing it.
	{
		assert(data_bytes > 0);
		locker lock(m_mutex);

		int sound_id = m_sound.size();
		m_sound[sound_id] = new sound(data_bytes, (Uint8*) data, format, sample_count, sample_rate, 
			stereo, m_defvolume);

		return sound_id;
	}

	void	SDL_sound_handler::set_default_volume(int vol)
	{
		if (m_defvolume >= 0 && m_defvolume <= 100)
		{
			m_defvolume = vol;
		}
	}

	void	SDL_sound_handler::play_sound(int sound_handle, int loops)
	// Play the index'd sample.
	{
		locker lock(m_mutex);

		it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->play(loops, this);
		}
	}

	void	SDL_sound_handler::stop_sound(int sound_handle)
	{
		locker lock(m_mutex);

		it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->clear_playlist();
		}
	}


	void	SDL_sound_handler::delete_sound(int sound_handle)
	// this gets called when it's done with a sample.
	{
		locker lock(m_mutex);

		it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			//		stop_sound(sound_handle);
			m_sound.erase(sound_handle);
		}
	}

	void	SDL_sound_handler::stop_all_sounds()
	{
		locker lock(m_mutex);

		for (it = m_sound.begin(); it != m_sound.end(); ++it)
		{
			it->second->clear_playlist();
		}
	}

	//	returns the sound volume level as an integer from 0 to 100,
	//	where 0 is off and 100 is full volume. The default setting is 100.
	int	SDL_sound_handler::get_volume(int sound_handle)
	{
		locker lock(m_mutex);

		int vol = 0;
		it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			vol = it->second->get_volume();
		}
		return vol;
	}


	//	A number from 0 to 100 representing a volume level.
	//	100 is full volume and 0 is no volume. The default setting is 100.
	void	SDL_sound_handler::set_volume(int sound_handle, int volume)
	{
		locker lock(m_mutex);

		it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->set_volume(volume);
		}
	}

	void	SDL_sound_handler::attach_aux_streamer(aux_streamer_ptr ptr, void* owner)
	{
		assert(owner);
		assert(ptr);
		locker lock(m_mutex);

		hash_map< Uint32, aux_streamer_ptr >::const_iterator it = m_aux_streamer.find((Uint32) owner);
		if (it == m_aux_streamer.end())
		{
			m_aux_streamer[(Uint32) owner] = ptr;
			SDL_PauseAudio(0);
		}
	}

	void SDL_sound_handler::detach_aux_streamer(void* owner)
	{
		locker lock(m_mutex);
		m_aux_streamer.erase((Uint32) owner);
	}

	void SDL_sound_handler::cvt(short int** adjusted_data, int* adjusted_size, unsigned char* data, 
		int size, int channels, int freq)
	{
		*adjusted_data = NULL;
		*adjusted_size = 0;

		SDL_AudioCVT  wav_cvt;
		int rc = SDL_BuildAudioCVT(&wav_cvt, AUDIO_S16SYS, channels, freq,
			audioSpec.format, audioSpec.channels, audioSpec.freq);
		if (rc == -1)
		{
			printf("Couldn't build converter!\n");
			return;
		}

		// Setup for conversion 
		wav_cvt.buf = (Uint8*) malloc(size * wav_cvt.len_mult);
		wav_cvt.len = size;
		memcpy(wav_cvt.buf, data, size);

		// We can delete to original WAV data now 
		// free(data);

		// And now we're ready to convert
		SDL_ConvertAudio(&wav_cvt);

		*adjusted_data = (int16_t*) wav_cvt.buf;
		*adjusted_size = size * wav_cvt.len_mult;
	}

	sound::sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
		int sample_rate, bool stereo, int vol):
		m_data(data),
		m_size(size),
		m_volume(vol),
		m_format(format),
		m_sample_count(sample_count),
		m_sample_rate(sample_rate),
		m_stereo(stereo)
	{
		assert(data);
		assert(size > 0);
		m_data = new Uint8[size];
		memcpy(m_data, data, size);
	}

	sound::~sound()
	{
		delete [] m_data;
	}

#ifdef USE_FFMPEG
	int sound::decode_mp3_data(int data_size, Uint8* data,	int* newsize, Uint8** newdata)
	{
		AVCodec* m_MP3_codec = avcodec_find_decoder(CODEC_ID_MP3);
		if (m_MP3_codec == NULL)
		{
			printf("FFMPEG can't decode MP3\n");
			return -1;
		}

		AVCodecContext *cc;
		AVCodecParserContext* parser;

		// Init the parser
		parser = av_parser_init(CODEC_ID_MP3);

		cc = avcodec_alloc_context();
		if (cc == NULL)
		{
			printf("Could not alloc MP3 codec context\n");
			return -1;
		}

		if (avcodec_open(cc, m_MP3_codec) < 0)
		{
			printf("Could not open MP3 codec\n");
			avcodec_close(cc);
			return -1;
		}

		int asize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;

		int bufsize = 2 * asize;
		Uint8* buf = (Uint8*) malloc(bufsize);
		int buflen = 0;

		while (data_size > 0)
		{
			uint8_t* frame;
			int framesize;
			int decoded = av_parser_parse(parser, cc, &frame, &framesize, data, data_size, 0, 0);

			if (decoded < 0)
			{
				printf("Error while decoding MP3-stream\n");
				break;
			}

			data += decoded;
			data_size -= decoded;

			if (framesize > 0)
			{
				int len = 0;
				if (avcodec_decode_audio(cc, (int16_t*) (buf + buflen), &len, frame, framesize) >= 0)
				{
					buflen += len;

					if (buflen > bufsize - asize)
					{
						bufsize += asize;
						buf = (Uint8*) realloc(buf, bufsize);
					}
				}
			}
		}

		*newdata = buf;
		*newsize = buflen;

		avcodec_close(cc);
		av_parser_close(parser);

		return 0;
	}
#endif

	void sound::play(int loops, SDL_sound_handler* handler)
	{
		int16_t*	adjusted_data = NULL;
		int	adjusted_size = 0;	

		switch (m_format)
		{
		case sound_handler::FORMAT_NATIVE16:
			handler->cvt(&adjusted_data, &adjusted_size, m_data, m_size, m_stereo ? 2 : 1, m_sample_rate);
			break;

		case sound_handler::FORMAT_MP3:
			{
#ifdef USE_FFMPEG
				int mp3size = 0;
				Uint8* mp3data = NULL;
				if (decode_mp3_data(m_size, m_data, &mp3size, &mp3data) == 0)
				{
					handler->cvt(&adjusted_data, &adjusted_size, mp3data, mp3size, m_stereo ? 2 : 1, m_sample_rate);
					free(mp3data);
				}
				else
				{
					printf("gameswf does not handle MP3 yet\n");
				}
#else
				printf("MP3 requires FFMPEG library\n");
#endif
				break;
			}

		default:
			printf("Sound format #%d is not supported\n", m_format);
			break;
		}

		if (adjusted_data)
		{
			m_playlist.push_back(new active_sound(adjusted_size, (Uint8*) adjusted_data, loops));
			SDL_PauseAudio(0);
		}

	}

	// called from audio callback
	bool sound::mix(Uint8* stream, int len)
	{
		Uint8* buf = new Uint8[len];

		bool play = false;
		for (vector< smart_ptr<active_sound> >::iterator it = m_playlist.begin(); it != m_playlist.end(); )
		{
			play = true;
			bool next_it = (*it->get_ptr()).mix(buf, len);

			int volume = (int) floorf(SDL_MIX_MAXVOLUME / 100.0f * m_volume);
			SDL_MixAudio(stream, buf, len, volume);

			if (next_it)
			{
				it++;
			}
			else
			{
				it = m_playlist.erase(it);
			}
		}

		delete [] buf;
		return play;
	}

	gameswf::sound_handler*	gameswf::create_sound_handler_sdl()
		// Factory.
	{
		return new SDL_sound_handler;
	}

	/// The callback function which refills the buffer with data
	/// We run through all of the sounds, and mix all of the active sounds 
	/// into the stream given by the callback.
	/// If sound is compresssed (mp3) a mp3-frame is decoded into a buffer,
	/// and resampled if needed. When the buffer has been sampled, another
	/// frame is decoded until all frames has been decoded.
	/// If a sound is looping it will be decoded from the beginning again.

	static void
		sdl_audio_callback (void *udata, Uint8 *stream, int len)
	{
		// Get the soundhandler
		SDL_sound_handler* handler = static_cast<SDL_sound_handler*>(udata);
		locker lock(handler->m_mutex);

		int pause = 1;
		memset(stream, 0, len);

		// mix Flash audio
		for (hash_map< int, smart_ptr<sound> >::iterator snd = handler->m_sound.begin();
			snd != handler->m_sound.end(); ++snd)
		{
			bool play = snd->second->mix(stream, len);
			pause = play ? 0 : pause;
		}

		// mix audio of video 
		if (handler->m_aux_streamer.size() > 0)
		{
			Uint8* mix_buf = new Uint8[len];
			hash_map< Uint32, gameswf::sound_handler::aux_streamer_ptr>::const_iterator it;
			for (it = handler->m_aux_streamer.begin(); it != handler->m_aux_streamer.end(); ++it)
			{
				memset(mix_buf, 0, len); 

				gameswf::sound_handler::aux_streamer_ptr aux_streamer = it->second;
				void* owner = (void*) it->first;
				(aux_streamer)(owner, mix_buf, len);

				SDL_MixAudio(stream, mix_buf, len, SDL_MIX_MAXVOLUME);
			}
			pause = 0;
			delete [] mix_buf;
		}
		SDL_PauseAudio(pause);
	}

}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
