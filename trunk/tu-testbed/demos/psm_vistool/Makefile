# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2003

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for psm_vistool test program

TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIBS := $(LIB_OUT) $(ENGINE) $(LIBS) $(JPEGLIB) $(ZLIB)


all: engine psm_vistool$(EXE_EXT)


psm_vistool$(EXE_EXT): psm_vistool.$(OBJ_EXT) $(ENGINE)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


engine:
	make -C $(TOP)/engine


clean:
	make -C $(TOP)/engine clean
	rm psm_vistool.$(OBJ_EXT) psm_vistool$(EXE_EXT)
