// tu_config.h	-- by Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic include file for configuring tu-testbed.


#ifndef TU_CONFIG_H
#define TU_CONFIG_H


// In case you need some unusual stuff to be predefined.  For example,
// maybe you #define new/delete to be something special.
#include "compatibility_include.h"


#include "base/dlmalloc.h"

// #define these in compatibility_include.h if you want something different.
#ifndef tu_malloc
#define tu_malloc(size) dlmalloc(size)
#endif
#ifndef tu_realloc
#define tu_realloc(old_ptr, new_size, old_size) dlrealloc(old_ptr, new_size)
#endif
#ifndef tu_free
#define tu_free(old_ptr, old_size) dlfree(old_ptr)
#endif


#endif // TU_CONFIG_H
