# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2002

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for swf parser.

TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := -I$(ENGINE_INCLUDE)

CFLAGS := -g $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)swf.$(LIB_EXT)
EXE_OUT = test_swf$(EXE_EXT)

LIBS = $(LIB_OUT) $(ENGINE) $(JPEGLIB) $(ZLIB)


all: $(LIB_OUT) $(EXE_OUT)


OBJS = \
	swf_impl.$(OBJ_EXT)	\
	swf_stream.$(OBJ_EXT)

swf_impl.$(OBJ_EXT): swf.h swf_impl.h

$(LIB_OUT): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): test_swf.$(OBJ_EXT) $(LIB_OUT)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


clean:
	rm $(LIB_OUT) $(EXE_OUT)
