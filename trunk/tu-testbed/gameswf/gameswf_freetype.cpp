// gameswf_freetype.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// TrueType font rasterizer based on freetype library,
// used code from demos/font_output/font_output.cpp

#include "base/tu_config.h"

#include "gameswf/gameswf_render.h"
#include "gameswf_freetype.h"
#include "gameswf_log.h"
#include "gameswf_canvas.h"
#include "base/utility.h"
#include "base/container.h"

#ifdef _WIN32
	#include <Windows.h>
#else
#endif


#if TU_CONFIG_LINK_TO_FREETYPE == 1

#define FREETYPE_MAX_FONTSIZE 96

namespace gameswf
{

	static float s_advance_scale = 0.16666666f;	//vv hack

	bool get_fontfile(const char* font_name, tu_string& file_name, bool is_bold, bool is_italic)
	// gets font file name by font name
	{

		if (font_name == NULL)
		{
			return false;
		}

#ifdef _WIN32

		//Vitaly: I'm not sure that this code works on all versions of Windows

		tu_stringi fontname = font_name;
		if (is_bold)
		{
			fontname += " Bold";
		}
		if (is_italic)
		{
			fontname +=  " Italic";
		}
		fontname += " (TrueType)";

		HKEY hKey;

		// try WinNT
		DWORD retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 
			0, 
			KEY_ALL_ACCESS,
			&hKey);

		if (retCode != ERROR_SUCCESS)
		{
			// try Windows
			retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts", 
				0, 
				KEY_ALL_ACCESS,
				&hKey);

			if (retCode != ERROR_SUCCESS)
			{
				return false;
			}
		}

		// Get the value of the 'windir' environment variable.
		tu_string windir(getenv("WINDIR"));

		// Get value count. 
		DWORD    cValues;              // number of values for key 
		retCode = RegQueryInfoKey(
			hKey,	// key handle 
			NULL,	// buffer for class name 
			NULL,	// size of class string 
			NULL,	// reserved 
			NULL,	// number of subkeys 
			NULL,	// longest subkey size 
			NULL,	// longest class string 
			&cValues,	// number of values for this key 
			NULL,	// longest value name 
			NULL,	// longest value data 
			NULL,	// security descriptor 
			NULL);	// last write time 

		// Enumerate the key values. 
		BYTE szValueData[MAX_PATH];
		TCHAR  achValue[MAX_PATH]; 
		for (DWORD i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) 
		{ 
			DWORD cchValue = MAX_PATH; 
			DWORD dwValueDataSize = sizeof(szValueData) - 1;
			achValue[0] = '\0'; 
			retCode = RegEnumValueA(hKey, i, 
				achValue, 
				&cchValue, 
				NULL, 
				NULL,
				szValueData,
				&dwValueDataSize);

			if (retCode == ERROR_SUCCESS) 
			{ 
				if (fontname == (char*) achValue)
				{
					file_name = windir + tu_string("\\Fonts\\") + (char*) szValueData;
					RegCloseKey(hKey);
					return true;
				}
			} 
		}

		// font file is not found, take default value - 'Times New Roman'
		file_name = windir + tu_string("\\Fonts\\Times");
		if (is_bold && is_italic)
		{
			file_name += "BI";
		}
		else
		if (is_bold)
		{
			file_name +=  "B";
		}
		else
		if (is_italic)
		{
			file_name +=  "I";
		}
		file_name += ".ttf";
		log_error("can't find font file for '%s'\nit's used '%s' file\n", fontname.c_str(), file_name.c_str());
		RegCloseKey(hKey);
		return true;

#else

	//TODO for Linux

	// hack
	file_name = "/usr/share/fonts/truetype/times";
	if (is_bold && is_italic)
	{
		file_name += "bi";
	}
	else
	if (is_bold)
	{
		file_name +=  "b";
	}
	else
	if (is_italic)
	{
		file_name +=  "b";
	}
	file_name += ".ttf";

	return true;

#endif
	}

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

	tu_freetype* tu_freetype::create_face(const char* fontname, bool is_bold, bool is_italic)
	{
		return new tu_freetype(fontname, is_bold, is_italic);
	}

	tu_freetype::tu_freetype(const char* fontname, bool is_bold, bool is_italic) :
		m_face(NULL)
	{
		if (m_lib == NULL)
		{
			init();
		}

		tu_string font_filename;
		if (get_fontfile(fontname, font_filename, is_bold, is_italic) == false)
		{
			log_error("can't found font file for font '%s'\n", fontname);
			return;
		}

		int error = FT_New_Face(m_lib, font_filename.c_str(), 0, &m_face);
		switch (error)
		{
			case 0:
				break;

			case FT_Err_Unknown_File_Format:
				log_error("file '%s' has bad format\n", font_filename.c_str());
				return;
				break;

			default:
				log_error("some error opening font '%s'\n", font_filename.c_str());
				return;
				break;
		}
	}

	tu_freetype::~tu_freetype()
	{
		if (m_face)
		{
			// Discards a given face object, as well as all of its child slots and sizes.
			FT_Done_Face(m_face);
		}
	}

	image::alpha* tu_freetype::draw_bitmap(const FT_Bitmap& bitmap)
	{
		// You must use power-of-two dimensions!!
		int	w = 1; while (w < bitmap.pitch) { w <<= 1; }
		int	h = 1; while (h < bitmap.rows) { h <<= 1; }

		image::alpha* alpha = image::create_alpha(w, h);
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

	// static
	int tu_freetype::move_to_callback(FT_CONST FT_Vector* to, void* user)
	{
		tu_freetype* _this = static_cast<tu_freetype*>(user);
		_this->m_canvas->move_to(to->x * _this->m_scale, - to->y * _this->m_scale);
		return 0;
	}
	
	// static 
	int tu_freetype::line_to_callback(FT_CONST FT_Vector* to, void* user)
	{
		tu_freetype* _this = static_cast<tu_freetype*>(user);
		_this->m_canvas->line_to(to->x * _this->m_scale, - to->y * _this->m_scale);
		return 0;
	}

	//static
	int tu_freetype::conic_to_callback(FT_CONST FT_Vector* ctrl, FT_CONST FT_Vector* to,
		void* user)
	{
		tu_freetype* _this = static_cast<tu_freetype*>(user);
		_this->m_canvas->curve_to(ctrl->x * _this->m_scale, - ctrl->y * _this->m_scale, 
				to->x * _this->m_scale, - to->y * _this->m_scale);
		return 0;
	}

	//static
	int tu_freetype::cubic_to_callback(FT_CONST FT_Vector* ctrl1, FT_CONST FT_Vector* ctrl2,
			FT_CONST FT_Vector* to, void* user)
	{
		tu_freetype* _this = static_cast<tu_freetype*>(user);
		float x = ctrl1->x + ((ctrl2->x - ctrl1->x) * 0.5);
		float y = ctrl1->y + ((ctrl2->y - ctrl1->y) * 0.5);
		_this->m_canvas->curve_to(x * _this->m_scale, - y * _this->m_scale, 
				to->x * _this->m_scale, - to->y * _this->m_scale);
		return 0;
	}

	// Get image of character as bitmap
	bitmap_info* tu_freetype::get_char_image(Uint16 code, rect& box, float* advance)
	{
		FT_Set_Pixel_Sizes(m_face, 0, FREETYPE_MAX_FONTSIZE);
		if (FT_Load_Char(m_face, code, FT_LOAD_RENDER))
		{
			return NULL;
		}

//		bool use_kerning = static_cast<FT_Bool>(FT_HAS_KERNING(m_face));

		image::alpha* im = draw_bitmap(m_face->glyph->bitmap);
		bitmap_info* bi = render::create_bitmap_info_alpha(im->m_width, im->m_height, im->m_data);
		delete im;;

		box.m_x_max = float(m_face->glyph->bitmap.width) / float(bi->m_suspended_image->m_width);
		box.m_y_max = float(m_face->glyph->bitmap.rows) / float(bi->m_suspended_image->m_height);

		box.m_x_min = float(m_face->glyph->metrics.horiBearingX) / float(m_face->glyph->metrics.width);
		box.m_y_min = float(m_face->glyph->metrics.horiBearingY) / float(m_face->glyph->metrics.height);
		box.m_x_min *= -box.m_x_max;
		box.m_y_min *= box.m_y_max;
		
		*advance = (float) m_face->glyph->metrics.horiAdvance * s_advance_scale;
		return bi;
	}

	// Get image of character as shape
	// FIXME
	shape_character_def* tu_freetype::get_char_def(Uint16 code, rect& box, float* advance)
	{
		if (FT_Load_Char(m_face, code, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE))
		{
			return NULL;
		}

		if (m_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
		{
			return NULL;
		}


		m_canvas = new canvas();
		m_canvas->begin_fill(rgba(255, 255, 255, 255));

		FT_Outline_Funcs callback_func;
 		callback_func.move_to = move_to_callback;
		callback_func.line_to = line_to_callback;
		callback_func.conic_to = conic_to_callback;
		callback_func.cubic_to = cubic_to_callback;

		m_scale = 1024.0f / m_face->units_per_EM;
		*advance = (float) m_face->glyph->metrics.horiAdvance * m_scale;

		FT_Outline_Decompose(&m_face->glyph->outline, &callback_func, this);

		m_canvas->end_fill();

		return m_canvas.get_ptr();
	}

	float tu_freetype::get_advance_x(Uint16 code)
	{
		FT_Set_Pixel_Sizes(m_face, 0, FREETYPE_MAX_FONTSIZE);
		if (FT_Load_Char(m_face, code, FT_LOAD_RENDER))
		{
			return 0;
		}
		return (float) m_face->glyph->metrics.horiAdvance * s_advance_scale;
	}

}

#endif