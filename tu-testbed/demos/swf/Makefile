# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2002

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for swf parser.

TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := -I$(ENGINE_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)


all: $(LIB_PRE)swf.$(LIB_EXT) test_swf$(EXE_EXT)


OBJS = swf_impl.$(OBJ_EXT)

$(LIB_PRE)swf.$(LIB_EXT): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


test_swf$(EXE_EXT): test_swf.$(OBJ_EXT)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)
