// gameswf_freetype.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Truetype font rasterizer based on freetype library

#ifndef GAMESWF_FREETYPE_H
#define GAMESWF_FREETYPE_H

#include "base/tu_config.h"

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_shape.h"

#if TU_CONFIG_LINK_TO_FREETYPE == 1

#include <ft2build.h>
#include FT_OUTLINE_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#define FT_CONST 

namespace gameswf
{

	struct tu_freetype : public ref_counted
	{
		tu_freetype(const char* fontname, bool is_bold, bool is_italic);
		~tu_freetype();
		
		static void init();
		static void close();
		static tu_freetype* create_face(const char* fontname, bool is_bold, bool is_italic);

		// callbacks
		static int move_to_callback(FT_CONST FT_Vector* vec, void* ptr);
		static int line_to_callback(FT_CONST FT_Vector* vec, void* ptr);
		static int conic_to_callback(FT_CONST FT_Vector* ctrl, FT_CONST FT_Vector* vec, 
			void* ptr);
		static int cubic_to_callback(FT_CONST FT_Vector* ctrl1, FT_CONST FT_Vector* ctrl2,
			FT_CONST FT_Vector* vec, void* ptr);

		image::alpha* draw_bitmap(const FT_Bitmap& bitmap);
		bitmap_info* get_char_image(Uint16 code, rect& box, float* advance);
		shape_character_def* get_char_def(Uint16 code, rect& box, float* advance);

		float get_advance_x(Uint16 code);

	private:
		
		static FT_Library	m_lib;
		FT_Face	m_face;
		float m_scale;
		smart_ptr<canvas> m_canvas;
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
		static tu_freetype* create_face(const char* fontname, bool is_bold, bool is_italic) { return NULL; }

		bitmap_info* get_char_image(Uint16 code, rect& box, float* advance)
		{
			return NULL;
		}
		float get_advance_x(Uint16 code) { return 0.0f; }

	};
}

#endif		// TU_CONFIG_LINK_TO_FREETYPE

#endif	// GAMESWF_FREETYPE_H
