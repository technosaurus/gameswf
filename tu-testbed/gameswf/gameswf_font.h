// gameswf_font.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A font type for gameswf.


#ifndef GAMESWF_FONT_H


#include "engine/container.h"


namespace gameswf
{
	struct movie;
	struct shape_character;
	struct stream;

	struct font
	{
		font();
		~font();
		shape_character*	get_glyph(int index) const;
		void	read(stream* in, int tag_type, movie* m);

	private:
		array<shape_character*>	m_glyphs;
		char*	m_name;
		// etc
	};

};	// end namespace gameswf



#endif // GAMESWF_FONT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
