// gameswf_file.h	-- Ignacio Castaño, Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.

#include "gameswf_file.h"

namespace gameswf
{

	// TODO: add error detection and report!!!
	static int std_read_func(void* dst, int bytes, void* appdata) 
	{
		return fread( dst, bytes, 1, (FILE *)appdata );
	}
	static int std_write_func(const void* src, int bytes, void* appdata)
	{
		return fwrite( src, bytes, 1, (FILE *)appdata );
	}
	static int std_seek_func(int pos, void *appdata)
	{
		return fseek((FILE*)appdata, pos, SEEK_SET);
	}
	static int std_tell_func(const void *appdata)
	{
		return ftell((FILE*)appdata);
	}
	static int std_close_func(const void *appdata)
	{
		return fclose((FILE*)appdata);
	}
	
	file::file(void * appdata, read_func rf, write_func wf, seek_func sf, tell_func tf, close_func cf)
	// Create a file using the custom callbacks.
	{
		m_data = appdata;
		m_read = rf;
		m_write = wf;
		m_seek = sf;
		m_tell = tf;
		m_close = cf;
		m_error = NO_ERROR;
	}

	file::file(FILE* fp, bool autoclose=false)
	// Create a file from a standard file pointer.
	{
		m_data = (void *)fp;
		m_read = std_read_func;
		m_write = std_write_func;
		m_seek = std_seek_func;
		m_tell = std_tell_func;
		m_close = autoclose ? std_close_func : NULL;
		m_error = NO_ERROR;
	}
	
	file::file(const char * name, const char * mode)
	// Create a file from the given name and the given mode.
	{
		m_data = fopen(name, mode);
		if (m_data)
		{
			m_read = std_read_func;
			m_write = std_write_func;
			m_seek = std_seek_func;
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
	
	file::~file()
	// Close this file when destroyed.
	{
		close();
	}
	
	void file::close() 
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

	void 	file::write_string(const char* src)
	{
		do {
			write8(*src);
			src++;
		} while(*src!=NULL);
	}
	
};	// end namespace gameswf




// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
