// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.


#include "engine/image.h"
#include "engine/utility.h"
#include "engine/jpeg.h"
#include "engine/dlmalloc.h"
#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>
#include <stdlib.h>


namespace image
{
	//
	// image_base
	//
	image_base::image_base(Uint8* data, int width, int height, int pitch)
		:
		m_data(data),
		m_width(width),
		m_height(height),
		m_pitch(pitch)
	{
	}


	//
	// rgb
	//

	rgb::rgb(int width, int height)
		:
		image_base(
			0,
			width,
			height,
			(width * 3 + 3) & ~3)	// round pitch up to nearest 4-byte boundary
	{
		assert(width > 0);
		assert(height > 0);
		assert(m_pitch >= m_width * 3);
		assert((m_pitch & 3) == 0);

		m_data = (Uint8*) dlmalloc(m_pitch * m_height);
	}

	rgb::~rgb()
	{
		if (m_data) {
			dlfree(m_data);
			m_data = 0;
		}
	}


	rgb*	create_rgb(int width, int height)
	// Create an system-memory rgb surface.  The data order is
	// packed 24-bit, RGBRGB..., regardless of the endian-ness of
	// the CPU.
	{
		rgb*	s = new rgb(width, height);

		return s;
	}


	//
	// rgba
	//


	rgba::rgba(int width, int height)
		:
		image_base(0, width, height, width * 4)
	{
		assert(width > 0);
		assert(height > 0);
		assert(m_pitch >= m_width * 4);
		assert((m_pitch & 3) == 0);

		m_data = (Uint8*) dlmalloc(m_pitch * m_height);
	}

	rgba::~rgba()
	{
		if (m_data) {
			dlfree(m_data);
			m_data = 0;
		}
	}


	rgba*	create_rgba(int width, int height)
	// Create an system-memory rgb surface.  The data order is
	// packed 32-bit, RGBARGBA..., regardless of the endian-ness
	// of the CPU.
	{
		rgba*	s = new rgba(width, height);

		return s;
	}


	void	rgba::set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
	// Set the pixel at the given position.
	{
		assert(x >= 0 && x < m_width);
		assert(y >= 0 && y < m_height);

		Uint8*	data = scanline(this, y) + 4 * x;

		data[0] = r;
		data[1] = g;
		data[2] = b;
		data[3] = a;
	}


	void	write_jpeg(SDL_RWops* out, rgb* image, int quality)
	// Write the given image to the given out stream, in jpeg format.
	{
		jpeg::output*	j_out = jpeg::output::create(out, image->m_width, image->m_height, quality);

		for (int y = 0; y < image->m_height; y++) {
			j_out->write_scanline(scanline(image, y));
		}

		delete j_out;
	}


	rgb*	read_jpeg(const char* filename)
	// Create and read a new image from the given filename, if possible.
	{
		SDL_RWops*	in = SDL_RWFromFile(filename, "rb");
		if (in)
		{
			rgb*	im = read_jpeg(in);
			SDL_RWclose(in);
			return im;
		}
		else {
			return NULL;
		}
	}


	rgb*	read_jpeg(SDL_RWops* in)
	// Create and read a new image from the stream.
	{
		jpeg::input*	j_in = jpeg::input::create(in);
		if (j_in == NULL) return NULL;
		
		rgb*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++) {
			j_in->read_scanline(scanline(im, y));
		}

		delete j_in;

		return im;
	}


	rgb*	read_swf_jpeg2(SDL_RWops* in)
	// Create and read a new image from the stream.  Image is in
	// SWF JPEG2-style format (the encoding tables come first in a
	// separate "stream" -- otherwise it's just normal JPEG).  The
	// IJG documentation describes this as "abbreviated" format.
	{
		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2(in);
		if (j_in == NULL) return NULL;
		
		rgb*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++) {
			j_in->read_scanline(scanline(im, y));
		}

		delete j_in;

		return im;
	}


#if 0
	SDL_Surface*	create_SDL_Surface(rgb* image)
	// Steal *image's data to create an SDL_Surface.
	//
	// DELETES image!!!
	{
		assert(image->m_pitch < 65536);	// SDL_Surface only uses Uint16 for pitch!!!

		SDL_Surface*	s = SDL_CreateRGBSurfaceFrom(image->m_data,
							     image->m_width, image->m_height, 24, image->m_pitch,
							     SDL_SwapLE32(0x0FF),
							     SDL_SwapLE32(0x0FF00),
							     SDL_SwapLE32(0x0FF0000),
							     0);

		// s owns *image's data now -- invalidate *image.
		image->m_data = 0;
		image->m_height = 0;
		image->m_width = 0;
		image->m_pitch = 0;
		delete image;

		assert(s->pixels);
		assert(s->format->BytesPerPixel == 3);
		assert(s->format->BitsPerPixel == 24);

		return s;
	}
#endif // 0

	void	make_next_miplevel(rgb* image)
	// Fast, in-place resample.  For making mip-maps.  Munges the
	// input image to produce the output image.
	{
		assert(image->m_data);

		int	new_w = image->m_width >> 1;
		int	new_h = image->m_height >> 1;
		if (new_w < 1) new_w = 1;
		if (new_h < 1) new_h = 1;

		int	new_pitch = new_w * 3;
		// Round pitch up to the nearest 4-byte boundary.
		new_pitch = (new_pitch + 3) & ~3;

		if (new_w * 2 != image->m_width  || new_h * 2 != image->m_height)
		{
			// Image can't be shrunk along (at least) one
			// of its dimensions, so don't bother
			// resampling.  Technically we should, but
			// it's pretty useless at this point.  Just
			// change the image dimensions and leave the
			// existing pixels.
		}
		else
		{
			// Resample.  Simple average 2x2 --> 1, in-place.
			int	pitch = image->m_pitch;
			for (int j = 0; j < new_h; j++) {
				Uint8*	out = ((Uint8*) image->m_data) + j * new_pitch;
				Uint8*	in = ((Uint8*) image->m_data) + (j << 1) * pitch;
				for (int i = 0; i < new_w; i++) {
					int	r, g, b;
					r = (*(in + 0) + *(in + 3) + *(in + 0 + pitch) + *(in + 3 + pitch));
					g = (*(in + 1) + *(in + 4) + *(in + 1 + pitch) + *(in + 4 + pitch));
					b = (*(in + 2) + *(in + 5) + *(in + 2 + pitch) + *(in + 5 + pitch));
					*(out + 0) = r >> 2;
					*(out + 1) = g >> 2;
					*(out + 2) = b >> 2;
					out += 3;
					in += 6;
				}
			}
		}

		// Munge image's members to reflect the shrunken image.
		image->m_width = new_w;
		image->m_height = new_h;
		image->m_pitch = new_pitch;
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
