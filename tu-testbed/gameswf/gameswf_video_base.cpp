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

	video_handler::video_handler():
		m_is_drawn(true),
		m_data(NULL)
	{
	}

	video_handler::~video_handler()
	{
	}

	void video_handler::update_video(video_data* data)
	{
		m_mutex.lock();
		m_data = data;
		m_is_drawn = false;
		m_mutex.unlock();
	}

	bool video_handler::is_video_data()
	{
		m_mutex.lock();
		bool ret = m_data == NULL ? false : true;
		m_mutex.unlock();
		return ret;
	}

	void video_handler::get_video(smart_ptr<video_data>* vd)
	{
		m_mutex.lock();
		*vd = m_data;
		m_is_drawn = true;
		m_mutex.unlock();
	}

}  // namespace gameswf
