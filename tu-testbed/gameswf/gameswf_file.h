// gameswf_file.h	-- Ignacio Castaño, Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.


#ifndef GAMESWF_FILE_H
#define GAMESWF_FILE_H


#include <stdio.h>
#include <assert.h>

// System dependent definitions, move this elswhere!
	#ifndef _GAMESWF_LITTLE_ENDIAN_
	#define _GAMESWF_LITTLE_ENDIAN_	1
	#endif
	
	typedef unsigned char	Uint8;
	typedef unsigned short	Uint16;
	typedef unsigned int	Uint32;
#ifdef __GNUC__
	typedef unsigned long long	Uint64;
#else
	typedef unsigned __int64	Uint64;
#endif



namespace gameswf
{

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

	// a file abstraction that can be customized with callbacks.
	class file
	{
	public:

		file(void * appdata, read_func rf, write_func wf, seek_func sf, tell_func tf, close_func cf=NULL);
		file(FILE* fp, bool autoclose);
		file(const char* name, const char* mode);
		~file();

#if _GAMESWF_LITTLE_ENDIAN_
		Uint64	read_le64() { return read64(); }
		Uint32 	read_le32() { return read32(); }
		Uint16 	read_le16() { return read16(); }
		Uint64 	read_be64() { return swap64(read64()); }
		Uint32 	read_be32() { return swap32(read32()); }
		Uint16 	read_be16() { return swap16(read16()); }
		void 	write_le64(Uint64 u) { write64(u); }
		void 	write_le32(Uint32 u) { write32(u); }
		void 	write_le16(Uint16 u) { write16(u); }
		void	write_be64(Uint64 u) { write64(swap64(u)); }
		void 	write_be32(Uint32 u) { write32(swap32(u)); }
		void 	write_be16(Uint16 u) { write16(swap16(u)); }
#else
		Uint64	read_le64() { return swap64(read64()); }
		Uint32 	read_le32() { return swap32(read32()); }
		Uint16 	read_le16() { return swap16(read16()); }
		Uint64 	read_be64() { return read64(); }
		Uint32 	read_be32() { return read32(); }
		Uint16 	read_be16() { return read16(); }
		void	write_le64(Uint64 u) { write64(swap64(u)); }
		void 	write_le32(Uint32 u) { write32(swap32(u)); }
		void 	write_le16(Uint16 u) { write16(swap16(u)); }
		void 	write_be64(Uint64 u) { write64(u); }
		void 	write_be32(Uint32 u) { write32(u); }
		void 	write_be16(Uint16 u) { write16(u); }
#endif

		// read/write a single byte
		Uint8 	read_byte() { return read8(); }
		void	write_byte(Uint8 u) { write8(u); }
		
		// read/write a byte stream
		void 	read_bytes(void* dst, int num) { m_read(dst, num, m_data); }
		void 	write_bytes(const void* src, int num) { m_write(src, num, m_data); }
		
		// write a 0-terminated string.
		void 	write_string(const char* src);
		
		// read up to max_length characters, returns the number of characters 
		// read, or -1 if the string length is longer than max_length.
		int	read_string(char* dst, int max_length);
		
		// get/set pos
		int 	get_position() { return m_tell(m_data); }
		void 	set_position(int p) const { m_seek(p, m_data); }

		int	error() { return m_error; }
		
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

		Uint16 swap16(Uint16 u) 
		{ 
			return ((u & 0x00FF) << 8) | 
				((u & 0xFF00) >> 8);
		}
		Uint32 swap32(Uint32 u) 
		{ 
			return ((u & 0x000000FF) << 24) | 
				((u & 0x0000FF00) << 8)  |
				((u & 0x00FF0000) >> 8)  |
				((u & 0xFF000000) >> 24);
		}
		Uint64 swap64(Uint64 u) 
		{ 
			return ((u & 0x00000000000000FFLL) << 56) | 
				((u & 0x000000000000FF00LL) << 40)  |
				((u & 0x0000000000FF0000LL) << 24)  |
				((u & 0x00000000FF000000LL) << 8) |
				((u & 0x000000FF00000000LL) >> 8) |
				((u & 0x0000FF0000000000LL) >> 24) |
				((u & 0x00FF000000000000LL) >> 40) |
				((u & 0xFF00000000000000LL) >> 56);
		}

		void * 	m_data;
		read_func 	m_read;
		write_func 	m_write;
		seek_func 	m_seek;
		tell_func 	m_tell;
		close_func 	m_close;
		int	m_error;
	};

};	// end namespace gameswf


#endif // GAMESWF_FILE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
