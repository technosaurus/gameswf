# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2002

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for swf parser.

TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)swf.$(LIB_EXT)
EXE_OUT = test_swf$(EXE_EXT)

LIBS := $(LIB_OUT) $(ENGINE) $(LIBS) $(JPEGLIB) $(ZLIB)


all: $(LIB_OUT) $(EXE_OUT) engine


OBJS = \
	swf_impl.$(OBJ_EXT)	\
	swf_types.$(OBJ_EXT)	\
	swf_stream.$(OBJ_EXT)	\
	swf_render.$(OBJ_EXT)

swf_impl.$(OBJ_EXT): swf.h swf_impl.h swf_types.h

$(LIB_OUT): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): test_swf.$(OBJ_EXT) $(LIB_OUT) $(ENGINE)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


engine:
	make -C ../../engine


clean:
	make -C ../../engine clean
	rm test_swf.$(OBJ_EXT) $(OBJS) $(LIB_OUT) $(EXE_OUT)
