// gameswf_video_base.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// base class for video


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory.h>
#include "gameswf/gameswf.h"
#include "base/tu_types.h"
#include "gameswf/gameswf_video_base.h"

namespace gameswf
{

	video::video():
		m_data(NULL),
		m_width(0),
		m_height(0),
		m_size(0),
		m_is_updated(false)
	{
	}

	// We should save planes[i].id & planes[i].unit
	// because they are already inited
	void video::resize(int w, int h)
	{
		delete [] m_data;

		m_width = w;
		m_height = h;
		m_is_updated = false;
		m_size = m_width * m_height;
		m_size += m_size >> 1;

		m_data = new Uint8[m_size];

		planes[Y].w = m_width;
		planes[Y].h = m_height;
		planes[Y].size = m_width * m_height;
		planes[Y].offset = 0;

		planes[U].w = m_width >> 1;
		planes[U].h = m_height >> 1;
		planes[U].size = planes[Y].size >> 2;
		planes[U].offset = planes[Y].size;

		planes[V].w = planes[U].w;
		planes[V].h = planes[U].h;
		planes[V].size = planes[U].size;
		planes[V].offset = planes[U].offset + planes[U].size;

		for (int i = 0; i < 3; ++i)
		{
			Uint32 ww = planes[i].w;
			Uint32 hh = planes[i].h;
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

	}	

	video::~video()
	{
		delete [] m_data;
	}

	Uint32 video::video_nlpo2(Uint32 x) const
	{
		x |= (x >> 1);
		x |= (x >> 2);
		x |= (x >> 4);
		x |= (x >> 8);
		x |= (x >> 16);
		return x + 1;
	}

	void video::update(Uint8* data)
	{
		m_is_updated = true;
		memcpy(m_data, data, m_size);
	}

	bool video::is_updated() const
	{
		return m_is_updated;
	}

	int video::size() const
	{
		return m_size;
	}

	void video::display(const matrix* m, const rect* bounds, const rgba& cx)
	{
	}

}  // namespace gameswf
