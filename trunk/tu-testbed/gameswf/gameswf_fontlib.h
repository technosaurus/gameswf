// gameswf_fontlib.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Internal interfaces to fontlib.


#ifndef GAMESWF_FONTLIB_H
#define GAMESWF_FONTLIB_H

#include "base/tu_config.h"

#include "base/container.h"
#include "gameswf/gameswf_types.h"

namespace gameswf
{

	// For adding fonts.
	void	add_font(font* f);

	// For drawing a textured glyph w/ current render transforms.
	void	draw_glyph(const matrix& m, const glyph& g, rgba color, int nominal_glyph_height);

	// Return the pixel height of text, such that the
	// texture glyphs are sampled 1-to-1 texels-to-pixels.
	// I.e. the height of the glyph box, in texels.
	float	get_glyph_max_height(const font* f);

	// Builds cached glyph textures from shape info.
	void	generate_font_bitmaps(const array<font*>& fonts, movie_definition_sub* owner);

	// Save cached font data, including glyph textures, to a
	// stream.
	void	output_cached_data(
		tu_file* out,
		const array<font*>& fonts,
		movie_definition_sub* owner,
		const cache_options& options);

	// Load a stream containing previously-saved cachded font
	// data, including glyph texture info.
	void	input_cached_data(tu_file* in, const array<font*>& fonts, movie_definition_sub* owner);

	typedef hash<int, glyph_entity> glyph_array;

	struct glyph_provider_tu : public glyph_provider
	{
		glyph_provider_tu();
		~glyph_provider_tu();

		virtual bitmap_info* get_char_image(character_def* shape_glyph, Uint16 code, 
			const tu_string& fontname, bool is_bold, bool is_italic, int fontsize,
			rect* bounds, float* advance);

	private:
		
		stringi_hash<glyph_array*> m_glyph;	// fontame-glyphs-glyph
	};

}	// end namespace gameswf



#endif // GAMESWF_FONTLIB_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
