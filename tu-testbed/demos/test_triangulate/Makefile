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


all: engine_libs test_triangulate$(EXE_EXT)


test_triangulate$(EXE_EXT): test_triangulate.$(OBJ_EXT) $(GEOMETRY_LIB) $(BASE_LIB)
	$(CC) -o $@ $^ $(LIBS) $(GEOMETRY_LIB) $(BASE_LIB) $(LDFLAGS)


engine_libs:
	$(MAKE) -C $(TOP)/base
	$(MAKE) -C $(TOP)/geometry


clean:
	$(MAKE) -C $(TOP)/base clean
	$(MAKE) -C $(TOP)/geometry clean
	-rm test_triangulate.$(OBJ_EXT) test_triangulate$(EXE_EXT)

fresh: clean all
