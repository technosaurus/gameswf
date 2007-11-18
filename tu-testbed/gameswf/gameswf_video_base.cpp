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
		m_data(NULL),
		m_is_updated(false)
	{
	}

	video_handler::~video_handler()
	{
	}

	void video_handler::update_frame(video_data* data)
	{
		m_mutex.lock();
		m_is_updated = true;
		m_data = data;
		m_mutex.unlock();
	}

	bool video_handler::is_updated() const
	{
		return m_is_updated;
	}

}  // namespace gameswf
