// as_textformat.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The TextFormat class represents character formatting information.
// Use the TextFormat class to create specific text formatting for text fields. 
// You can apply text formatting to both static and dynamic text fields. 
// Some properties of the TextFormat class are not available for both
// embedded and device fonts. 

#ifndef GAMESWF_AS_TEXTFORMAT_H
#define GAMESWF_AS_TEXTFORMAT_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_character.h"

namespace gameswf
{

	void	as_global_textformat_ctor(const fn_call& fn);

	struct as_textformat : public as_object
	{
		as_textformat();
		virtual as_textformat* cast_to_as_textformat() { return this; }

	};

}	// end namespace gameswf


#endif // GAMESWF_AS_TEXTFORMAT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
