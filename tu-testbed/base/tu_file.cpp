// tu_file.cpp	-- Ignacio Castaño, Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.


#include "base/tu_file.h"
#include "base/utility.h"
#include "base/container.h"


//
// tu_file functions using FILE
//


static int std_read_func(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
	assert(appdata);
	assert(dst);
	return fread( dst, 1, bytes, (FILE *)appdata );
}

static int std_write_func(const void* src, int bytes, void* appdata)
// Return the number of bytes actually written.
{
	assert(appdata);
	assert(src);
	return fwrite( src, 1, bytes, (FILE *)appdata );
}

static int std_seek_func(int pos, void *appdata)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
	assert(appdata);
	clearerr((FILE*) appdata);	// make sure EOF flag is cleared.
	int	result = fseek((FILE*)appdata, pos, SEEK_SET);
	if (result == EOF)
	{
		// @@ TODO should set m_error to something relevant based on errno.
		return TU_FILE_SEEK_ERROR;
	}
	return 0;
}

static int std_seek_to_end_func(void *appdata)
// Return 0 on success, TU_FILE_SEEK_ERROR on failure.
{
	assert(appdata);
	int	result = fseek((FILE*)appdata, 0, SEEK_END);
	if (result == EOF)
	{
		// @@ TODO should set m_error to something relevant based on errno.
		return TU_FILE_SEEK_ERROR;
	}
	return 0;
}

static int std_tell_func(const void *appdata)
// Return the file position, or -1 on failure.
{
	assert(appdata);
	return ftell((FILE*)appdata);
}

static bool std_get_eof_func(void *appdata)
// Return true if we're at EOF.
{
	assert(appdata);
	if (feof((FILE*) appdata))
	{
		return true;
	}
	else
	{
		return false;
	}
}

static int std_close_func(void *appdata)
// Return 0 on success, or TU_FILE_CLOSE_ERROR on failure.
{
	assert(appdata);
	int	result = fclose((FILE*)appdata);
	if (result == EOF)
	{
		// @@ TODO should set m_error to something relevant based on errno.
		return TU_FILE_CLOSE_ERROR;
	}
	return 0;
}


//
// tu_file functions using a readable/writable memory buffer
//


struct membuf
{
	array<unsigned char>	m_data;
	int	m_position;

	membuf()
		:
		m_position(0)
	{
	}

	bool	is_valid()
	{
		return
			m_position >= 0
			&& m_position <= m_data.size();
	}

	unsigned char*	get_cursor() { return &(m_data[m_position]); }
};


static int mem_read_func(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
	assert(appdata);
	assert(dst);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	int	bytes_to_read = imin(bytes, buf->m_data.size() - buf->m_position);
	if (bytes_to_read)
	{
		memcpy(dst, buf->get_cursor(), bytes_to_read);
	}
	buf->m_position += bytes_to_read;

	return bytes_to_read;
}


static int mem_write_func(const void* src, int bytes, void* appdata)
// Return the number of bytes actually written.
{
	assert(appdata);
	assert(src);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	// Expand buffer if necessary.
	int	bytes_to_expand = imax(0, buf->m_position + bytes - buf->m_data.size());
	if (bytes_to_expand)
	{
		buf->m_data.resize(buf->m_data.size() + bytes_to_expand);
	}

	memcpy(buf->get_cursor(), src, bytes);
	buf->m_position += bytes;

	return bytes;
}

static int mem_seek_func(int pos, void *appdata)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
	assert(appdata);
	assert(pos >= 0);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	if (pos < 0)
	{
		buf->m_position = 0;
		return TU_FILE_SEEK_ERROR;
	}

	if (pos > buf->m_data.size())
	{
		buf->m_position = buf->m_data.size();
		return TU_FILE_SEEK_ERROR;
	}

	buf->m_position = pos;
	return 0;
}

static int mem_seek_to_end_func(void* appdata)
// Return 0 on success, TU_FILE_SEEK_ERROR on failure.
{
	assert(appdata);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	buf->m_position = buf->m_data.size();
	return 0;
}

static int mem_tell_func(const void* appdata)
// Return the file position, or -1 on failure.
{
	assert(appdata);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	return buf->m_position;
}

static bool	mem_get_eof_func(void* appdata)
// Return true if we're positioned at the end of the buffer.
{
	assert(appdata);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	return buf->m_position >= buf->m_data.size();
}

static int mem_close_func(void* appdata)
// Return 0 on success, or TU_FILE_CLOSE_ERROR on failure.
{
	assert(appdata);

	membuf*	buf = (membuf*) appdata;
	assert(buf->is_valid());

	delete buf;

	return 0;
}


//
// generic functionality
//


tu_file::tu_file(
	void * appdata,
	read_func rf,
	write_func wf,
	seek_func sf,
	seek_to_end_func ef,
	tell_func tf,
	get_eof_func gef,
	close_func cf)
// Create a file using the custom callbacks.
{
	m_data = appdata;
	m_read = rf;
	m_write = wf;
	m_seek = sf;
	m_seek_to_end = ef;
	m_tell = tf;
	m_get_eof = gef;
	m_close = cf;
	m_error = TU_FILE_NO_ERROR;
}


tu_file::tu_file(FILE* fp, bool autoclose=false)
// Create a file from a standard file pointer.
{
	m_data = (void *)fp;
	m_read = std_read_func;
	m_write = std_write_func;
	m_seek = std_seek_func;
	m_seek_to_end = std_seek_to_end_func;
	m_tell = std_tell_func;
	m_get_eof = std_get_eof_func;
	m_close = autoclose ? std_close_func : NULL;
	m_error = TU_FILE_NO_ERROR;
}


tu_file::tu_file(const char * name, const char * mode)
// Create a file from the given name and the given mode.
{
	m_data = fopen(name, mode);
	if (m_data)
	{
		m_read = std_read_func;
		m_write = std_write_func;
		m_seek = std_seek_func;
		m_seek_to_end = std_seek_to_end_func;
		m_tell = std_tell_func;
		m_get_eof = std_get_eof_func;
		m_close = std_close_func;
		m_error = TU_FILE_NO_ERROR;
	}
	else 
	{
		m_read = NULL;
		m_write = NULL;
		m_seek = NULL;
		m_seek_to_end = NULL;
		m_tell = NULL;
		m_get_eof = NULL;
		m_close = NULL;
		m_error = TU_FILE_OPEN_ERROR;
	}
}


tu_file::tu_file(memory_buffer_enum m)
// Create a read/write memory buffer.
{
	m_data = new membuf;

	m_read = mem_read_func;
	m_write = mem_write_func;
	m_seek = mem_seek_func;
	m_seek_to_end = mem_seek_to_end_func;
	m_tell = mem_tell_func;
	m_get_eof = mem_get_eof_func;
	m_close = mem_close_func;
	m_error = TU_FILE_NO_ERROR;
}


tu_file::~tu_file()
// Close this file when destroyed.
{
	close();
}


void tu_file::close() 
// Close this file.
{ 
	if (m_close)
	{
		m_close(m_data);
	}
	m_data = NULL; 
	m_read = NULL; 
	m_write = NULL; 
	m_seek = NULL; 
	m_tell = NULL; 
	m_close = NULL; 
}


void	tu_file::copy_from(tu_file* src)
// Copy remaining contents of *src into *this.
{
	while (src->get_eof() == false)
	{
		Uint8	b = src->read8();
		if (src->get_error())
		{
			break;
		}

		write8(b);
	}
}


void tu_file::write_string(const char* src)
{
	do {
		write8(*src);
		src++;
	} while (*src!='\0');
}


int tu_file::read_string(char* dst, int max_length) 
{
	int i=0;
	while (i<max_length)
	{
		dst[i] = read8();
		if (dst[i]=='\0')
		{
			return i;
		}
		i++;
	}

	dst[max_length - 1] = 0;	// force termination.

	return -1;
}



#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif // _WIN32


int	tu_file::printf(const char* fmt, ...)
// Use printf-like semantics to send output to this stream.
{
	// Workspace for vsnprintf formatting.
	static const int	BUFFER_SIZE = 1000;
	char	buffer[BUFFER_SIZE];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer, BUFFER_SIZE, fmt, ap);
	va_end(ap);

	return write_bytes(buffer, strlen(buffer));
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
