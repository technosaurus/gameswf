// utility.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#ifndef UTILITY_H
#define UTILITY_H

#include <assert.h>
#include <math.h>
#include <SDL/SDL_endian.h>


// assert_else, for asserting with associated recovery code.  Follow
// "assert_else( predicate )" with a block of code to execute when the
// predicate is false.  In debug builds this code will be executed if
// you step past the assert.  In release builds this code there's no
// assert, but the recovery block will be executed when predicate is
// false.
//
// usage:
//	assert_else( p ) {
//		block of recovery code for when p is false;
//	}
//
//	assert_else( p );	// no recovery code.
//
//	assert_else( p ) return;	// recovery code doesn't need to be a block.
//
#define assert_else(p)	\
	if ( !(p) && (assert(0), 1) )


#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI

//
// some misc handy math functions
//
inline int	iabs(int i) { if (i < 0) return -i; else return i; }
#ifdef __GNUC__
	// use the builtin (gcc) operator. ugly, but not my call.
	#define imax _max
	#define fmax _max
	#define _max(a,b) ((a)>?(b))
	#define imin _min
	#define fmin _min
	#define _min(a,b) ((a)<?(b))
#else // not GCC
	inline int	imax(int a, int b) { if (a < b) return b; else return a; }
	inline float	fmax(float a, float b) { if (a < b) return b; else return a; }
	inline int	imin(int a, int b) { if (a < b) return a; else return b; }
	inline float	fmin(float a, float b) { if (a < b) return a; else return b; }
#endif // not GCC

inline int	iclamp(int i, int min, int max) {
	assert( min <= max );
	return imax(min, imin(i, max));
}

inline float	fclamp(float f, float min, float max) {
	assert( min <= max );
	return fmax(min, fmin(f, max));
}

const float LN_2 = 0.693147180559945f;
inline float	log2(float f) { return logf(f) / LN_2; }

inline int	fchop( float f ) { return (int) f; }	// replace w/ inline asm if desired
inline int	frnd(float f) { return fchop(f + 0.5f); }	// replace with inline asm if desired


//
// Read/write bytes to SDL_RWops streams.
//


inline void	WriteByte(SDL_RWops* dst, Uint8 b) {
	dst->write(dst, &b, sizeof(b), 1);
}


inline Uint8	ReadByte(SDL_RWops* src) {
	Uint8	b;
	src->read(src, &b, sizeof(b), 1);	// @@ check for error
	return b;
}


//
// Read/write 32-bit little-endian floats, and 64-bit little-endian doubles.
//


inline void	WriteFloat32(SDL_RWops* dst, float value) {
	union {
		float	f;
		int	i;
	} u;
	u.f = value;
	SDL_WriteLE32(dst, u.i);
}


inline float	ReadFloat32(SDL_RWops* src) {
	union {
		float	f;
		int	i;
	} u;
	u.i = SDL_ReadLE32(src);
	return u.f;
}


inline void	WriteDouble64(SDL_RWops* dst, double value) {
	union {
		double	d;
		Uint64	l;
	} u;
	u.d = value;
	SDL_WriteLE64(dst, u.l);
}


inline double	ReadDouble64(SDL_RWops* src) {
	union {
		double	d;
		Uint64	l;
	} u;
	u.l = SDL_ReadLE64(src);
	return u.d;
}


#endif // UTILITY_H
