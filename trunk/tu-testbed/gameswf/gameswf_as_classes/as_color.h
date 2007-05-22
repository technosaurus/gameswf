// as_color.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Color class lets you set the RGB color value and color transform
// of movie clips and retrieve those values once they have been set. 


#ifndef GAMESWF_AS_COLOR_H
#define GAMESWF_AS_COLOR_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_character.h"

namespace gameswf
{

	void	as_global_color_ctor(const fn_call& fn);

	struct as_color : public as_object
	{
		as_color();
		virtual as_color* cast_to_as_color() { return this; }

		smart_ptr<character> m_target;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_COLOR_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
