// tu_file.h	-- Ignacio Castaño, Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A very generic file class that can be customized with callbacks.


#ifndef TU_FILE_H
#define TU_FILE_H


#include <stdio.h>
#include "engine/tu_types.h"
#include "engine/utility.h"

struct SDL_RWops;


// a file abstraction that can be customized with callbacks.
// Designed to be easy to hook up to FILE*, SDL_RWops*, or
// whatever stream type(s) you might use in your game or
// libraries.
class tu_file
{
public:
	typedef int (* read_func)(void* dst, int bytes, void* appdata);
	typedef int (* write_func)(const void* src, int bytes, void* appdata);
	typedef int (* seek_func)(int pos, void *appdata);
	typedef int (* tell_func)(const void *appdata);
	typedef int (* close_func)(const void *appdata);

	enum 
	{
		NO_ERROR = 0,
		OPEN_ERROR,
		READ_ERROR,
		WRITE_ERROR,
		SEEK_ERROR,
		CLOSE_ERROR
	};


	// The generic constructor; supply functions for the implementation.
	tu_file(void * appdata, read_func rf, write_func wf, seek_func sf, tell_func tf, close_func cf=NULL);

	// Make a file from an ordinary FILE*.
	tu_file(FILE* fp, bool autoclose);

	// Optional: if you're using SDL, this is a constructor to create
	// a tu_file from an SDL_RWops* stream.
	tu_file(SDL_RWops* sdl_stream, bool autoclose);

	// Open a file using ordinary fopen().  Automatically closes the
	// file when we are destroyed.
	tu_file(const char* name, const char* mode);

	~tu_file();

	Uint64	read_le64();
	Uint32 	read_le32();
	Uint16 	read_le16();
	Uint64 	read_be64();
	Uint32 	read_be32();
	Uint16 	read_be16();
	void 	write_le64(Uint64 u);
	void 	write_le32(Uint32 u);
	void 	write_le16(Uint16 u);
	void	write_be64(Uint64 u);
	void 	write_be32(Uint32 u);
	void 	write_be16(Uint16 u);

	// read/write a single byte
	Uint8 	read_byte() { return read8(); }
	void	write_byte(Uint8 u) { write8(u); }
	
	// Read/write a byte buffer.
	// Returns number of bytes read/written.
	int 	read_bytes(void* dst, int num) { return m_read(dst, num, m_data); }
	int 	write_bytes(const void* src, int num) { return m_write(src, num, m_data); }
	
	// write a 0-terminated string.
	void 	write_string(const char* src);
	
	// read up to max_length characters, returns the number of characters 
	// read, or -1 if the string length is longer than max_length.
	int	read_string(char* dst, int max_length);

	// float/double IO
	void	write_float32(float value);
	void	write_double64(double value);
	float	read_float32();
	double	read_double64();

	// get/set pos
	int	get_position() const { return m_tell(m_data); }
	void 	set_position(int p) { m_seek(p, m_data); }

	int	get_error() { return m_error; }

	// UNSAFE back door, for testing only.
	void*	get_app_data_DEBUG() { return m_data; }


private:
	Uint64	read64() { Uint64 u; m_read(&u, 8, m_data); return u; }
	Uint32	read32() { Uint32 u; m_read(&u, 4, m_data); return u; }
	Uint16	read16() { Uint16 u; m_read(&u, 2, m_data); return u; }
	Uint8	read8() { Uint8 u; m_read(&u, 1, m_data); return u; }

	void	write64(Uint64 u) { m_write(&u, 8, m_data); }
	void	write32(Uint32 u) { m_write(&u, 4, m_data); }
	void	write16(Uint16 u) { m_write(&u, 2, m_data); }
	void	write8(Uint8 u) { m_write(&u, 1, m_data); }

	void	close();

	Uint16	swap16(Uint16 u);
	Uint32	swap32(Uint32 u);
	Uint64	swap64(Uint64 u);

	void * 	m_data;
	read_func 	m_read;
	write_func 	m_write;
	seek_func 	m_seek;
	tell_func 	m_tell;
	close_func 	m_close;
	int	m_error;
};


//
// Some inline stuff.
//

inline Uint16 tu_file::swap16(Uint16 u)
{ 
	return ((u & 0x00FF) << 8) | 
		((u & 0xFF00) >> 8);
}

inline Uint32 tu_file::swap32(Uint32 u)
{ 
	return ((u & 0x000000FF) << 24) | 
		((u & 0x0000FF00) << 8)  |
		((u & 0x00FF0000) >> 8)  |
		((u & 0xFF000000) >> 24);
}

inline Uint64 tu_file::swap64(Uint64 u)
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

#if _TU_LITTLE_ENDIAN_
	inline Uint64	tu_file::read_le64() { return read64(); }
	inline Uint32	tu_file::read_le32() { return read32(); }
	inline Uint16	tu_file::read_le16() { return read16(); }
	inline Uint64	tu_file::read_be64() { return swap64(read64()); }
	inline Uint32	tu_file::read_be32() { return swap32(read32()); }
	inline Uint16	tu_file::read_be16() { return swap16(read16()); }
	inline void	tu_file::write_le64(Uint64 u) { write64(u); }
	inline void	tu_file::write_le32(Uint32 u) { write32(u); }
	inline void	tu_file::write_le16(Uint16 u) { write16(u); }
	inline void	tu_file::write_be64(Uint64 u) { write64(swap64(u)); }
	inline void	tu_file::write_be32(Uint32 u) { write32(swap32(u)); }
	inline void	tu_file::write_be16(Uint16 u) { write16(swap16(u)); }
#else // not _TU_LITTLE_ENDIAN_
	inline Uint64	tu_file::read_le64() { return swap64(read64()); }
	inline Uint32	tu_file::read_le32() { return swap32(read32()); }
	inline Uint16	tu_file::read_le16() { return swap16(read16()); }
	inline Uint64	tu_file::read_be64() { return read64(); }
	inline Uint32	tu_file::read_be32() { return read32(); }
	inline Uint16	tu_file::read_be16() { return read16(); }
	inline void	tu_file::write_le64(Uint64 u) { write64(swap64(u)); }
	inline void	tu_file::write_le32(Uint32 u) { write32(swap32(u)); }
	inline void	tu_file::write_le16(Uint16 u) { write16(swap16(u)); }
	inline void	tu_file::write_be64(Uint64 u) { write64(u); }
	inline void	tu_file::write_be32(Uint32 u) { write32(u); }
	inline void	tu_file::write_be16(Uint16 u) { write16(u); }
#endif	// not _TU_LITTLE_ENDIAN_


inline void	tu_file::write_float32(float value)
// Write a 32-bit litt-endian double to this file.
{
	union alias {
		float	f;
		Uint32	i;
	} u;
	compiler_assert(sizeof(alias) == sizeof(Uint32));

	u.f = value;
	write_le32(u.i);
}


inline float	tu_file::read_float32()
// Read a 32-bit little-endian double from this file.
{
	union {
		float	f;
		Uint32	i;
	} u;
	compiler_assert(sizeof(u) == sizeof(u.i));

	u.i = read_le32();
	return u.f;
}


inline void		tu_file::write_double64(double value)
// Write a 64-bit little-endian double to this file.
{
	union {
		double	d;
		Uint64	l;
	} u;
	compiler_assert(sizeof(u) == sizeof(u.l));

	u.d = value;
	write_le64(u.l);
}


inline double	tu_file::read_double64()
// Read a little-endian 64-bit double from this file.
{
	union {
		double	d;
		Uint64	l;
	} u;
	compiler_assert(sizeof(u) == sizeof(u.l));

	u.l = read_le64();
	return u.d;
}


#endif // TU_FILE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
