// tu_file.cpp	-- Ignacio Casta�o, Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.


#include "engine/tu_file.h"
#include "engine/utility.h"


// TODO: add error detection and report!!!
static int std_read_func(void* dst, int bytes, void* appdata) 
{
	assert(appdata);
	assert(dst);
	return fread( dst, 1, bytes, (FILE *)appdata );
}
static int std_write_func(const void* src, int bytes, void* appdata)
{
	assert(appdata);
	assert(src);
	return fwrite( src, 1, bytes, (FILE *)appdata );
}
static int std_seek_func(int pos, void *appdata)
{
	assert(appdata);
	return fseek((FILE*)appdata, pos, SEEK_SET);
}
static int std_seek_to_end_func(void *appdata)
{
	assert(appdata);
	return fseek((FILE*)appdata, 0, SEEK_END);
}
static int std_tell_func(const void *appdata)
{
	assert(appdata);
	return ftell((FILE*)appdata);
}
static int std_close_func(const void *appdata)
{
	assert(appdata);
	return fclose((FILE*)appdata);
}

	
tu_file::tu_file(void * appdata, read_func rf, write_func wf, seek_func sf, seek_to_end_func ef, tell_func tf, close_func cf)
// Create a file using the custom callbacks.
{
	m_data = appdata;
	m_read = rf;
	m_write = wf;
	m_seek = sf;
	m_seek_to_end = ef;
	m_tell = tf;
	m_close = cf;
	m_error = NO_ERROR;
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
	m_close = autoclose ? std_close_func : NULL;
	m_error = NO_ERROR;
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
		m_close = std_close_func;
		m_error = NO_ERROR;
	}
	else 
	{
		m_read = NULL;
		m_write = NULL;
		m_seek = NULL;
		m_tell = NULL;
		m_close = NULL;
		m_error = OPEN_ERROR;
	}
}


tu_file::~tu_file()
// Close this file when destroyed.
{
	close();
}


void tu_file::close() 
// Close this file.
{ 
	if(m_close)
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


void tu_file::write_string(const char* src)
{
	do {
		write8(*src);
		src++;
	} while(*src!='\0');
}


int tu_file::read_string(char* dst, int max_length) 
{
	int i=0;
	while(i<max_length)
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


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
