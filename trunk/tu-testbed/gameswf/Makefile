# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2003

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for gameswf library & tests.

TOP = ..
include $(TOP)/config

# temporary hack...
ifeq ($(TU_TESTBED_COMPILER), msvc)
	SDL_MIXER_INCLUDE := -I../../SDL_mixer-1.2.5
	SDL_MIXER_LIB := ../../SDL_mixer-1.2.5/VisualC/Release/SDL_mixer.lib
else
	SDL_MIXER_INCLUDE := 
	SDL_MIXER_LIB := -lSDL_mixer
endif


INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE) $(SDL_MIXER_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)gameswf.$(LIB_EXT)
EXE_OUT = gameswf_test_ogl$(EXE_EXT)

LIBS := $(LIB_OUT) $(BASE_LIB) $(LIBS) $(JPEGLIB) $(ZLIB) $(SDL_MIXER_LIB)


all: base_lib $(LIB_OUT) $(EXE_OUT)


OBJS = \
	gameswf_action.$(OBJ_EXT)	\
	gameswf_button.$(OBJ_EXT)	\
	gameswf_dlist.$(OBJ_EXT)	\
	gameswf_font.$(OBJ_EXT)	\
	gameswf_fontlib.$(OBJ_EXT)	\
	gameswf_impl.$(OBJ_EXT)	\
	gameswf_log.$(OBJ_EXT)	\
	gameswf_render.$(OBJ_EXT)	\
	gameswf_shape.$(OBJ_EXT)	\
	gameswf_sound.$(OBJ_EXT)	\
	gameswf_stream.$(OBJ_EXT)	\
	gameswf_styles.$(OBJ_EXT)	\
	gameswf_tesselate.$(OBJ_EXT)	\
	gameswf_text.$(OBJ_EXT)		\
	gameswf_types.$(OBJ_EXT)

gameswf_impl.$(OBJ_EXT): gameswf.h gameswf_impl.h gameswf_types.h

$(LIB_OUT): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): gameswf_test_ogl.$(OBJ_EXT) $(LIB_OUT) $(BASE_LIB)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


base_lib:
	make -C $(TOP)/base


clean:
	make -C $(TOP)/base clean
	-rm gameswf_test_ogl.$(OBJ_EXT) $(OBJS) $(LIB_OUT) $(EXE_OUT)
