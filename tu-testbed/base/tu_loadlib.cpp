// tu_loadlib.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// plugin loader

#include "base/tu_types.h"
#include "base/container.h"
#include "base/tu_loadlib.h"

#ifdef _WIN32

#include <windows.h>

tu_loadlib::tu_loadlib(const char* library_name) :
	m_hlib(NULL)
{
	tu_string path = library_name;
	path += ".dll";

	m_hlib = LoadLibrary(path.c_str());
}

tu_loadlib::~tu_loadlib()
{
	FreeLibrary(m_hlib);
}

void* tu_loadlib::get_function(const char* function_name)
{
	if (m_hlib)
	{
		return (void*) GetProcAddress(m_hlib, function_name);
	}
	return NULL;
}

#else	// not _WIN32

// TODO using dlopen()

tu_loadlib* tu_loadlib::load(const char* library_name)
{
	return NULL;
}

tu_loadlib::tu_loadlib(lib_t hlib)
{
	assert(hlib);
	m_hlib = hlib;
}

tu_loadlib::~tu_loadlib()
{
}

void* tu_loadlib::get_function(const char* function_name)
{
	return NULL;
}

#endif	// not _WIN32


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
