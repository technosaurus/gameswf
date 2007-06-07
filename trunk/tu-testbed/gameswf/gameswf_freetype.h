// gameswf_freetype.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Truetype font rasterizer based on freetype library

#ifndef GAMESWF_FREETYPE_H
#define GAMESWF_FREETYPE_H

#include "gameswf/gameswf.h"
#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_FREETYPE == 1

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace gameswf
{

	struct tu_freetype : public ref_counted
	{
		tu_freetype(const char* fontname);
		~tu_freetype();
		
		static void init();
		static void close();
		static tu_freetype* create_face(const char* fontname);


		image::alpha* draw_bitmap(const FT_Bitmap& bitmap);
		bitmap_info* get_char_image(Uint16 code, rect& box, float* advance);
		float get_advance_x(Uint16 code);

	private:
		
		static FT_Library	m_lib;
		FT_Face	m_face;

	};

}

#else	// TU_CONFIG_LINK_TO_FREETYPE == 0

namespace gameswf
{

	struct tu_freetype : public ref_counted
	{
		tu_freetype(const char* fontname) {}
		~tu_freetype() {}
		
		static void init() {}
		static void close() {}
		static tu_freetype* create_face(const char* fontname) { return NULL; }

		bitmap_info* get_char_image(Uint16 code, rect& box, float* advance)
		{
			return NULL;
		}

	};
}

#endif		// TU_CONFIG_LINK_TO_FREETYPE

#endif	// GAMESWF_FREETYPE_H
