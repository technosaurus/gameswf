// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.

#ifndef IMAGE_H
#define IMAGE_H


#include "engine/utility.h"
#include <SDL/SDL.h>
struct SDL_RWops;


namespace image
{
	// Make a system-memory 24-bit bitmap surface.  24-bit packed
	// data, red byte first.
	SDL_Surface*	create_rgb(int width, int height);
	
	inline Uint8*	scanline(SDL_Surface* surf, int y)
	{
		assert(surf);
		assert(y < surf->h);
		return ((Uint8*) surf->pixels) + surf->pitch * y;
	}

	void	resample(SDL_Surface* out, int out_x0, int out_y0, int out_x1, int out_y1,
			 SDL_Surface* in, float in_x0, float in_y0, float in_x1, float in_y1);

	void	write_jpeg(SDL_RWops* out, SDL_Surface* image, int quality);

	SDL_Surface*	read_jpeg(SDL_RWops* in);
};


#endif // IMAGE_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
