// utility.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#ifndef UTILITY_H
#define UTILITY_H

#include <assert.h>
#include <math.h>
#include <SDL/SDL_endian.h>


#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI

//
// some misc handy math functions
//
inline int	iabs(int i) { if (i < 0) return -i; else return i; }
inline int	imax(int a, int b) { if (a < b) return b; else return a; }
inline float	fmax(float a, float b) { if (a < b) return b; else return a; }
inline int	imin(int a, int b) { if (a < b) return a; else return b; }
inline float	fmin(float a, float b) { if (a < b) return b; else return a; }

inline int	iclamp(int i, int min, int max) {
	assert( min <= max );
	return imax(min, imin(i, max));
}

inline float	fclamp(float f, float min, float max) {
	assert( min <= max );
	return fmax(min, fmin(f, max));
}

const float LN_2 = 0.693147180559945;
inline float	log2(float f) { return log(f) / LN_2; }

inline int	fchop( float f ) { return (int) f; }	// replace w/ inline asm if desired
inline int	frnd(float f) { return fchop(f + 0.5); }	// replace with inline asm if desired


//
// Read/write 32-bit little-endian floats, and 64-bit little-endian doubles.
//


inline void	WriteFloat32(SDL_RWops* dst, float f) {
	SDL_WriteLE32(dst, *((Uint32*) &f));
}


inline float	ReadFloat32(SDL_RWops* src) {
	float	f;
	*((Uint32*) &f) = SDL_ReadLE32(src);
	return f;
}


inline void	WriteDouble64(SDL_RWops* dst, double d) {
	SDL_WriteLE64(dst, *((Uint64*) &d));
}


inline double	ReadDouble64(SDL_RWops* src) {
	double	d;
	*((Uint64*) &d) = SDL_ReadLE64(src);
	return d;
}


#endif // UTILITY_H
