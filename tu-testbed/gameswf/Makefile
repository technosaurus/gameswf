# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2003

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for gameswf library & tests.

TOP = ..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)gameswf.$(LIB_EXT)
EXE_OUT = gameswf_test_ogl$(EXE_EXT)

LIBS := $(LIB_OUT) $(ENGINE) $(LIBS) $(JPEGLIB) $(ZLIB)


all: engine $(LIB_OUT) $(EXE_OUT)


OBJS = \
	gameswf_action.$(OBJ_EXT)	\
	gameswf_button.$(OBJ_EXT)	\
	gameswf_font.$(OBJ_EXT)	\
	gameswf_fontlib.$(OBJ_EXT)	\
	gameswf_impl.$(OBJ_EXT)	\
	gameswf_log.$(OBJ_EXT)	\
	gameswf_render_ogl.$(OBJ_EXT)	\
	gameswf_shape.$(OBJ_EXT)	\
	gameswf_stream.$(OBJ_EXT)	\
	gameswf_styles.$(OBJ_EXT)	\
	gameswf_tesselate.$(OBJ_EXT)	\
	gameswf_types.$(OBJ_EXT)

gameswf_impl.$(OBJ_EXT): gameswf.h gameswf_impl.h gameswf_types.h

$(LIB_OUT): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): gameswf_test_ogl.$(OBJ_EXT) $(LIB_OUT) $(ENGINE)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


engine:
	make -C $(TOP)/engine


clean:
	make -C $(TOP)/engine clean
	rm gameswf_test_ogl.$(OBJ_EXT) $(OBJS) $(LIB_OUT) $(EXE_OUT)
