// gameswf_video_base.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// base class for video


#ifndef GAMESWF_VIDEO_BASE_H
#define GAMESWF_VIDEO_BASE_H

#include "gameswf/gameswf.h"

namespace gameswf
{
	struct video : public ref_counted
	{
		enum {Y, U, V, T, NB_TEXS};

		video();
		~video();

		void resize(int w, int h);
		void update(Uint8* data);
		virtual void display(const matrix* m, const rect* bounds, const rgba& cx);
		int size() const;
		bool is_updated() const;

	protected:

		Uint8* m_data;
		int m_width;
		int m_height;
		int m_size;
		bool m_is_updated;

		struct plane
		{
			Uint32 w, h, p2w, p2h, offset, size;
			int unit;
			int id;
			float coords[4][2];
		} planes[4];	

		const matrix* m;
		const rect* m_bounds;

	private:

		Uint32 video_nlpo2(Uint32 x) const;

	};
}

#endif
