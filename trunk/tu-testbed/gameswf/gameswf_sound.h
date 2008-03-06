// gameswf_sound.h   -- Thatcher Ulrich, Vitaly Alexeev

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_SOUND_H
#define GAMESWF_SOUND_H


#include "gameswf/gameswf_impl.h"


namespace gameswf
{
	int get_sample_rate(int index);

	struct sound_sample : public as_object_interface
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_SOUND_SAMPLE };
		virtual bool is(int class_id)
		{
			return m_class_id == class_id;
		}

		int	m_sound_handler_id;

		sound_sample(int id) : m_sound_handler_id(id)
		{
		}

		virtual ~sound_sample();
	};
}


#endif // GAMESWF_SOUND_H
