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

// Used to hold the info about active sounds

namespace gameswf
{

	struct sound;

	// Use SDL and ffmpeg to handle sounds.
	struct SDL_sound_handler : public sound_handler
	{

		// NetStream audio callbacks
		hash< as_object_interface* /* netstream */, aux_streamer_ptr /* callback */> m_aux_streamer;
		hash< int, smart_ptr<sound> > m_sound;
		int m_defvolume;
		tu_mutex* m_mutex;

		// SDL_audio specs
		SDL_AudioSpec audioSpec;

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

	struct active_sound: public gameswf::ref_counted
	{
		active_sound(int size, Uint8* data, int loops):
		m_pos(0),
		m_loops(loops),
		m_size(size),
		m_data(data)
	{
		assert(data);
		assert(size > 0);
	}

	~active_sound()
	{
		// memory was allocated in cvt()
		delete [] m_data;
	}

	bool mix(Uint8* buf , int len)
	{
		memset(buf, 0, len);
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

			if (m_pos == 0)
			{
				if (m_loops == 0)
				{
					next_it = false;
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

		int decode_mp3_data(int data_size, Uint8* data,	int* newsize, Uint8** newdata);
		void append(void* data, int size, SDL_sound_handler* handler);
		void play(int loops, SDL_sound_handler* handler);
		bool mix(Uint8* stream, int len);

	private:

		Uint8* m_data;
		int m_size;
		int m_volume;
		gameswf::sound_handler::format_type m_format;
		int m_sample_count;
		int m_sample_rate;
		bool m_stereo;
		array < smart_ptr<active_sound> > m_playlist;
	};

}

#endif // SOUND_HANDLER_SDL_H



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
