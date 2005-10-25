# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2004

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for test_triangulate programs


TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIBS := $(GEOMETRY_LIB) $(BASE_LIBS) $(LIBS) $(JPEGLIB) $(ZLIB)


all: test_triangulate$(EXE_EXT)


test_triangulate$(EXE_EXT): test_triangulate.$(OBJ_EXT) base_lib net_lib geometry_lib
	$(CC) -o $@ $< $(LIBS) $(GEOMETRY_LIB) $(NET_LIB) $(BASE_LIB) $(LDFLAGS)


clean: base_clean net_clean geometry_clean
	-rm test_triangulate.$(OBJ_EXT) test_triangulate$(EXE_EXT)

fresh: clean all
