// as_sound.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_SOUND_H
#define GAMESWF_AS_SOUND_H

#include "../gameswf_action.h"	// for as_object

namespace gameswf
{

	void	as_global_sound_ctor(const fn_call& fn);

	struct as_sound : public as_object
	{
		tu_string sound;
		int sound_id;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_SOUND_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
