// utility.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#ifndef UTILITY_H
#define UTILITY_H

#include <assert.h>
#include "base/tu_math.h"
#include "base/tu_types.h"

#ifdef _WIN32
#ifndef NDEBUG

// On windows, replace ANSI assert with our own, for a less annoying
// debugging experience.
//int	tu_testbed_assert_break(const char* filename, int linenum, const char* expression);
#undef assert
#define assert(x)	if (!(x)) { __asm { int 3 } }	// tu_testbed_assert_break(__FILE__, __LINE__, #x))

#endif // not NDEBUG
#endif // _WIN32


// Compile-time assert.  Thanks to Jon Jagger
// (http://www.jaggersoft.com) for this trick.
#define compiler_assert(x)	switch(0){case 0: case x:;}


//
// new/delete wackiness -- if USE_DL_MALLOC is defined, we're going to
// try to use Doug Lea's malloc as much as possible by overriding the
// default operator new/delete.
//
#ifdef USE_DL_MALLOC

void*	operator new(size_t size);
void	operator delete(void* ptr);
void*	operator new[](size_t size);
void	operator delete[](void* ptr);

#else	// not USE_DL_MALLOC

// If we're not using DL_MALLOC, then *really* don't use it: #define
// away dlmalloc(), dlfree(), etc, back to the platform defaults.
#define dlmalloc	malloc
#define dlfree	free
#define dlrealloc	realloc
#define dlcalloc	calloc
#define dlmemalign	memalign
#define dlvalloc	valloc
#define dlpvalloc	pvalloc
#define dlmalloc_trim	malloc_trim
#define dlmalloc_stats	malloc_stats

#endif	// not USE_DL_MALLOC


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

inline float flerp(float a, float b, float f) { return (b - a) * f + a; }

const float LN_2 = 0.693147180559945f;
inline float	log2(float f) { return logf(f) / LN_2; }

inline int	fchop( float f ) { return (int) f; }	// replace w/ inline asm if desired
inline int	frnd(float f) { return fchop(f + 0.5f); }	// replace with inline asm if desired


template<class T>
void	swap(T& a, T& b)
// Convenient swap function.  Normally I don't approve of non-const
// references, but I think this is a special case.
{
	T	temp(a);
	a = b;
	b = temp;
}


//
// endian conversions
//


inline Uint16 swap16(Uint16 u)
{ 
	return ((u & 0x00FF) << 8) | 
		((u & 0xFF00) >> 8);
}

inline Uint32 swap32(Uint32 u)
{ 
	return ((u & 0x000000FF) << 24) | 
		((u & 0x0000FF00) << 8)  |
		((u & 0x00FF0000) >> 8)  |
		((u & 0xFF000000) >> 24);
}

inline Uint64 swap64(Uint64 u)
{
#ifdef __GNUC__
	return ((u & 0x00000000000000FFLL) << 56) |
		((u & 0x000000000000FF00LL) << 40)  |
		((u & 0x0000000000FF0000LL) << 24)  |
		((u & 0x00000000FF000000LL) << 8) |
		((u & 0x000000FF00000000LL) >> 8) |
		((u & 0x0000FF0000000000LL) >> 24) |
		((u & 0x00FF000000000000LL) >> 40) |
		((u & 0xFF00000000000000LL) >> 56);
#else
	return ((u & 0x00000000000000FF) << 56) | 
		((u & 0x000000000000FF00) << 40)  |
		((u & 0x0000000000FF0000) << 24)  |
		((u & 0x00000000FF000000) << 8) |
		((u & 0x000000FF00000000) >> 8) |
		((u & 0x0000FF0000000000) >> 24) |
		((u & 0x00FF000000000000) >> 40) |
		((u & 0xFF00000000000000) >> 56);
#endif
}


inline Uint32	swap_le32(Uint32 le_32)
// Given a 32-bit little-endian piece of data, return it as a 32-bit
// integer in native endian-ness.  I.e. on a little-endian machine,
// this just returns the input; on a big-endian machine, this swaps
// the bytes around first.
{
#ifdef _TU_LITTLE_ENDIAN_
	return le_32;
#else	// not _TU_LITTLE_ENDIAN_
	return swap32(le_32);	// convert to big-endian.
#endif	// not _TU_LITTLE_ENDIAN_
}


inline Uint16	swap_le16(Uint16 le_16)
// Given a 16-bit little-endian piece of data, return it as a 16-bit
// integer in native endianness.
{
#ifdef _TU_LITTLE_ENDIAN_
	return le_16;
#else	// not _TU_LITTLE_ENDIAN_
	return swap16(le_16);	// convert to big-endian.
#endif	// not _TU_LITTLE_ENDIAN_
}


inline Uint32	swap_be32(Uint32 le_32)
// Given a 32-bit big-endian piece of data, return it as a 32-bit
// integer in native endian-ness.  I.e. on a little-endian machine,
// this swaps the bytes around; on a big-endian machine, it just
// returns the input.
{
#ifdef _TU_LITTLE_ENDIAN_
	return swap32(le_32);	// convert to little-endian.
#else	// not _TU_LITTLE_ENDIAN_
	return le_32;
#endif	// not _TU_LITTLE_ENDIAN_
}


inline Uint16	swap_be16(Uint16 le_16)
// Given a 16-bit big-endian piece of data, return it as a 16-bit
// integer in native endianness.
{
#ifdef _TU_LITTLE_ENDIAN_
	return swap16(le_16);	// convert to little-endian.
#else	// not _TU_LITTLE_ENDIAN_
	return le_16;
#endif	// not _TU_LITTLE_ENDIAN_
}


// Handy macro to quiet compiler warnings about unused parameters/variables.
#define UNUSED(x) (x) = (x)

#endif // UTILITY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
