// tu_loadlib.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// plugin loader

#ifndef TU_LOADLIB_H
#define TU_LOADLIB_H

#ifdef _WIN32
	#include <windows.h>
	#define lib_t HINSTANCE
#else
	#define lib_t int
#endif

struct tu_loadlib
{
	typedef void (*init_module) (const char* function_name);

	tu_loadlib(lib_t hlib);
	~tu_loadlib();

	static tu_loadlib* load(const char* library_name);
	void* get_function(const char* function_name);
	
	private:

	lib_t m_hlib;

};


#endif // TU_LOADLIB_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
