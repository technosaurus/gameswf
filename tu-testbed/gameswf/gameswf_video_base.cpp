// gameswf_video_base.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// base class for video


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory.h>
#include "gameswf.h"
#include "base/tu_types.h"
#include "gameswf_video_base.h"

namespace gameswf
{

	YUV_video::YUV_video(int w, int h):
		m_width(w),
		m_height(h),
		m_video_in_place(false)
	{
		planes[Y].w = m_width;
		planes[Y].h = m_height;
		planes[Y].size = m_width * m_height;
		planes[Y].offset = 0;

		planes[U] = planes[Y];
		planes[U].w >>= 1;
		planes[U].h >>= 1;
		planes[U].size >>= 2;
		planes[U].offset = planes[Y].size;

		planes[V] = planes[U];
		planes[V].offset += planes[U].size;

		m_size = planes[Y].size + (planes[U].size << 1);

		for (int i = 0; i < 3; ++i)
		{
			planes[i].id = 0;	//texids[i];

			Uint32 ww = planes[i].w;
			Uint32 hh = planes[i].h;
			planes[i].unit = 0; // i[units];
			planes[i].p2w = (ww & (ww - 1)) ? video_nlpo2(ww) : ww;
			planes[i].p2h = (hh & (hh - 1)) ? video_nlpo2(hh) : hh;
			float tw = (float) ww / planes[i].p2w;
			float th = (float) hh / planes[i].p2h;

			planes[i].coords[0][0] = 0.0;
			planes[i].coords[0][1] = 0.0;
			planes[i].coords[1][0] = tw;
			planes[i].coords[1][1] = 0.0;
			planes[i].coords[2][0] = tw; 
			planes[i].coords[2][1] = th;
			planes[i].coords[3][0] = 0.0;
			planes[i].coords[3][1] = th;
		}

		m_data = new Uint8[m_size];
	}	

	YUV_video::~YUV_video()
	{
		delete [] m_data;
	}

	Uint32 YUV_video::video_nlpo2(Uint32 x) const
	{
		x |= (x >> 1);
		x |= (x >> 2);
		x |= (x >> 4);
		x |= (x >> 8);
		x |= (x >> 16);
		return x + 1;
	}

	void YUV_video::update(Uint8* data)
	{
		m_video_in_place = true;
		memcpy(m_data, data, m_size);
	}

	int YUV_video::size() const
	{
		return m_size;
	}

	bool YUV_video::video_in_place() const
	{
		return m_video_in_place;
	}

	void YUV_video::display(const matrix* m, const rect* bounds, const rgba& cx)
	{
	}

}  // namespace gameswf
