// gameswf_fontlib.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Internal interfaces to fontlib.


#ifndef GAMESWF_FONTLIB_H
#define GAMESWF_FONTLIB_H


#include "base/container.h"
#include "gameswf_types.h"
class tu_file;


namespace gameswf
{
	struct movie_def_impl;

	namespace fontlib
	{
		// Builds cached glyph textures from shape info.
		void	generate_font_bitmaps(const array<font*>& fonts, movie_definition_sub* owner);
		
		// Save cached font data, including glyph textures, to a
		// stream.
		void	output_cached_data(tu_file* out, const array<font*>& fonts, movie_definition_sub* owner);
		
		// Load a stream containing previously-saved cachded font
		// data, including glyph texture info.
		void	input_cached_data(tu_file* in, const array<font*>& fonts, movie_definition_sub* owner);
		
	}	// end namespace fontlib
}	// end namespace gameswf



#endif // GAMESWF_FONTLIB_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
