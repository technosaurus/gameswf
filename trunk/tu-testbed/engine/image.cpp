// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.


#include "engine/image.h"
#include "engine/utility.h"
#include "engine/jpeg.h"
#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>


namespace image
{
	SDL_Surface*	create_rgb(int width, int height)
	// Create an system-memory rgb surface.  The data order is packed
	// 24-bit, RGBRGB..., regardless of the endian-ness of the CPU.
	{
		SDL_Surface*	s = SDL_CreateRGBSurface(SDL_SWSURFACE,
												 width, height, 24,
												 SDL_SwapLE32(0x0FF),
												 SDL_SwapLE32(0x0FF00),
												 SDL_SwapLE32(0x0FF0000),
												 0);
		assert(s->format->BytesPerPixel == 3);
		assert(s->format->BitsPerPixel == 24);

		return s;
	}


#if 0
	void	resample(SDL_Surface* dest, int out_x0, int out_y0, int out_x1, int out_y1,
			 SDL_Surface* src, float in_x0, float in_y0, float in_x1, float in_y1)
	// Resample the specified rectangle of the src surface into the
	// specified rectangle of the destination surface.  Output coords
	// are inclusive.
	{
		// Make sure output is within bounds.
		assert(out_x0 >= 0 && out_x0 < dest->w);
		assert(out_x1 > out_x0 && out_x1 < dest->w);
		assert(out_y0 >= 0 && out_y0 < dest->h);
		assert(out_y1 > out_y0 && out_y1 < dest->h);

		int	dxo = (out_x1 - out_x0);
		int	dyo = (out_y1 - out_y0);

		// @@ check input...

		float	dxi = in_x1 - in_x0;
		float	dyi = in_y1 - in_y0;
		assert(dxi > 0.001f);
		assert(dyi > 0.001f);

		float	x_factor = dxi / dxo;
		float	y_factor = dyi / dyo;

		// @@ not optimized.

		for (int j = 0; j <= dyo; j++) {
			for (int i = 0; i <= dxo; i++) {
				// @@ simple nearest-neighbor point-sample.
				float	x = i * x_factor + in_x0;
				float	y = j * y_factor + in_y0;
				x = fclamp(x, 0.f, float(src->w - 1));
				y = fclamp(y, 0.f, float(src->h - 1));

				Uint8*	p = scanline(src, frnd(y)) + 3 * frnd(x);
				Uint8*	q = scanline(dest, out_y0 + j) + 3 * (out_x0 + i);

				*q++ = *p++;	// red
				*q++ = *p++;	// green
				*q++ = *p++;	// blue
			}
		}
	}
#endif // 0

	void	write_jpeg(SDL_RWops* out, SDL_Surface* image, int quality)
	// Write the given image to the given out stream, in jpeg format.
	{
		jpeg::output*	j_out = jpeg::output::create(out, image->w, image->h, quality);

		for (int y = 0; y < image->h; y++) {
			j_out->write_scanline(scanline(image, y));
		}

		delete j_out;
	}


	SDL_Surface*	read_jpeg(SDL_RWops* in)
	// Create and read a new image from the stream.
	{
		jpeg::input*	j_in = jpeg::input::create(in);
		if (j_in == NULL) return NULL;
		
		SDL_Surface*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++) {
			j_in->read_scanline(scanline(im, y));
		}

		delete j_in;

		return im;
	}

};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
