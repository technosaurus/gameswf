// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.

#ifndef IMAGE_H
#define IMAGE_H


#include "engine/utility.h"
class tu_file;
namespace jpeg { struct input; };


namespace image
{
	struct image_base
	{
		Uint8*	m_data;
		int	m_width;
		int	m_height;
		int	m_pitch;	// byte offset from one row to the next

		image_base(Uint8* data, int width, int height, int pitch);
	};

	// 24-bit RGB image.  Packed data, red byte first (RGBRGB...)
	struct rgb : public image_base
	{
		rgb(int width, int height);
		~rgb();
	};

	// 32-bit RGBA image.  Packed data, red byte first (RGBARGBA...)
	struct rgba : public image_base
	{
		rgba(int width, int height);
		~rgba();

		void	set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	};


	// Make a system-memory 24-bit bitmap surface.  24-bit packed
	// data, red byte first.
	rgb*	create_rgb(int width, int height);


	// Make a system-memory 32-bit bitmap surface.  Packed data,
	// red byte first.
	rgba*	create_rgba(int width, int height);
	
	inline Uint8*	scanline(image_base* surf, int y)
	{
		assert(surf);
		assert(y >= 0 && y < surf->m_height);
		return ((Uint8*) surf->m_data) + surf->m_pitch * y;
	}

	void	resample(rgb* out, int out_x0, int out_y0, int out_x1, int out_y1,
			 rgb* in, float in_x0, float in_y0, float in_x1, float in_y1);

	void	resample(rgba* out, int out_x0, int out_y0, int out_x1, int out_y1,
			 rgba* in, float in_x0, float in_y0, float in_x1, float in_y1);

	void	write_jpeg(tu_file* out, rgb* image, int quality);

	rgb*	read_jpeg(const char* filename);
	rgb*	read_jpeg(tu_file* in);

	// For reading SWF JPEG2-style image data (slight variation on
	// ordinary JPEG).
	rgb*	read_swf_jpeg2(tu_file* in);

	// For reading SWF JPEG2-style image data, using pre-loaded
	// headers stored in the given jpeg::input object.
	rgb*	read_swf_jpeg2_with_tables(jpeg::input* loader);

	// For reading SWF JPEG3-style image data, like ordinary JPEG, 
	// but stores the data in rgba format.
	rgba*	read_swf_jpeg3(tu_file* in);

	// Fast, in-place, DESTRUCTIVE resample.  For making mip-maps.
	// Munges the input image to produce the output image.
	void	make_next_miplevel(rgb* image);
};


#endif // IMAGE_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
