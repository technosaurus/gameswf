// gameswf_freetype.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// TrueType font rasterizer based on freetype library,
// used code from demos/font_output/font_output.cpp

#include "base/tu_config.h"

#include "gameswf/gameswf_render.h"
#include "gameswf_freetype.h"
#include "base/utility.h"

#if TU_CONFIG_LINK_TO_FREETYPE == 1

namespace gameswf
{
	// static
	FT_Library	tu_freetype::m_lib;

	// static
	void tu_freetype::init()
	{
		int	error = FT_Init_FreeType(&m_lib);
		if (error)
		{
			fprintf(stderr, "can't init FreeType!  error = %d\n", error);
			exit(1);
		}
	}

	// static
	void tu_freetype::close()
	{
		int error = FT_Done_FreeType(m_lib);
		if (error)
		{
			fprintf(stderr, "can't close FreeType!  error = %d\n", error);
		}
	}

	tu_freetype* tu_freetype::create_face(const char* fontname)
	{
		return new tu_freetype(fontname);
	}

	tu_freetype::tu_freetype(const char* fontname)
	{
		if (m_lib == NULL)
		{
			init();
		}

		const char* font_filename = "/windows/fonts/times.ttf";	//vv fixme
		int error = FT_New_Face(m_lib, font_filename, 0, &m_face);
		if (error == FT_Err_Unknown_File_Format)
		{
			fprintf(stderr, "file '%s' has bad format\n", font_filename);
			exit(1);
		}
		else
			if (error)
			{
				fprintf(stderr, "some error opening font '%s'\n", font_filename);
				exit(1);
			}

			// quality of image
			error = FT_Set_Char_Size(
				m_face,
				0,	//width
				12 * 64,	//height*64 in points!
				300,	//horiz device dpi
				300	//vert device dpi
				);
			assert(error == 0);
	}

	tu_freetype::~tu_freetype()
	{
	}

	image::alpha* tu_freetype::draw_bitmap(const FT_Bitmap& bitmap)
	{
		image::alpha* alpha = image::create_alpha(fontlib::get_glyph_texture_size(), fontlib::get_glyph_texture_size());
		memset(alpha->m_data, 0, alpha->m_width * alpha->m_height);

		// copy image to alpha
		for (int i = 0; i < bitmap.rows; i++)
		{
			uint8*	src = bitmap.buffer + bitmap.pitch * i;
			uint8*	dst = alpha->m_data + alpha->m_pitch * i;
			int	x = bitmap.width;
			while (x-- > 0)
			{
				*dst++ = *src++;
			}
		}

		return alpha;
	}

	bitmap_info* tu_freetype::get_char_image(Uint16 code, float* x_max, float* y_max, float* advance)
	{
		if (FT_Load_Char(m_face, code, FT_LOAD_RENDER))
		{
			return NULL;
		}

//		bool use_kerning = static_cast<FT_Bool>(FT_HAS_KERNING(m_face));

		image::alpha* im = draw_bitmap(m_face->glyph->bitmap);
		bitmap_info* bi = render::create_bitmap_info_alpha(im->m_width, im->m_height, im->m_data);
		delete im;;
		
		*x_max = float(m_face->glyph->bitmap.width) / float(bi->m_suspended_image->m_width);
		*y_max = float(m_face->glyph->bitmap.rows) / float(bi->m_suspended_image->m_height);
		*advance = m_face->glyph->advance.x / 3.2f; //vv fixme
		return bi;
	}

}

#endif