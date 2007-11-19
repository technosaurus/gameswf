// gameswf_video_base.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// base class for video


#ifndef GAMESWF_VIDEO_BASE_H
#define GAMESWF_VIDEO_BASE_H

#include "base/smart_ptr.h"
#include "gameswf/gameswf.h"
#include "gameswf/gameswf_mutex.h"

namespace gameswf
{

	struct audio_data;
	struct video_data;
	struct audio_video_data : public ref_counted
	{
		Uint8* m_data;

		// base class for audio & video data
		audio_video_data() :
			m_data(NULL)
		{
		};

		~audio_video_data()
		{
			free(m_data);
			m_data = NULL;
		};

		virtual audio_data* cast_to_audio() { return 0; }
		virtual video_data* cast_to_video() { return 0; }
	};


	// Data for audio queue. 
	struct audio_data : public audio_video_data
	{
		Uint32 m_size;
		Uint8* m_ptr;

		audio_data(Uint8* data, int size) :
			m_size(size),
			m_ptr(data)
		{
			assert(data);
			m_data = data;
		};

		virtual audio_data* cast_to_audio() { return this; }
	};

	// Data for video queue. 
	struct video_data : public audio_video_data
	{
		int m_width;
		int m_height;
		double m_pts;	// presentation timestamp in sec

		video_data(Uint8* data, int width, int height) :
			m_width(width),
			m_height(height),
			m_pts(0)
		{
			assert(data);
			m_data = data;
		};

		virtual video_data* cast_to_video() { return this; }
	};

	struct video_handler : public ref_counted
	{

		video_handler();
		~video_handler();

		virtual void display(const matrix* m, const rect* bounds, const rgba& cx) = 0;
		bool is_video_data();
		void get_video(smart_ptr<video_data>* vd);
		void update_video(video_data* data);

		volatile bool m_is_drawn;

	protected:

		tu_mutex m_mutex;

		// multithread !!!
		smart_ptr<video_data> m_data;

		const matrix* m;
		const rect* m_bounds;
	};
}

#endif
