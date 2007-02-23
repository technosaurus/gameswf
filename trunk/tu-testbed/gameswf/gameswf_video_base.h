// gameswf_video_base.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// base class for video


#ifndef GAMESWF_VIDEO_BASE_H
#define GAMESWF_VIDEO_BASE_H

#include "gameswf.h"

namespace gameswf
{
	struct YUV_video : public ref_counted
	{
		enum {Y, U, V, T, NB_TEXS};

		YUV_video(int w, int h);
		~YUV_video();

		void update(Uint8* data);
		virtual void display(const matrix* m, const rect* bounds, const rgba& cx);
		int size() const;
		bool video_in_place() const;

	protected:

		Uint8* m_data;
		int m_width;
		int m_height;
		int m_size;

		struct plane
		{
			Uint32 w, h, p2w, p2h, offset, size;
			int unit;
			int id;
			float coords[4][2];
		} planes[4];	

		const matrix* m;
		const rect* m_bounds;
		bool m_video_in_place;

	private:

		Uint32 video_nlpo2(Uint32 x) const;

	};
}

#endif
