# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2003

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for gameswf library & tests.

TOP = ..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE) $(SDL_MIXER_INCLUDE) $(LIBMAD_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS) -g
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)gameswf.$(LIB_EXT)
EXE_OUT = gameswf_test_ogl$(EXE_EXT)
PARSER_OUT = gameswf_parser$(EXE_EXT)
PROCESSOR_OUT = gameswf_processor$(EXE_EXT)

LIBS := $(LIB_OUT) $(BASE_LIB) $(LIBS) $(JPEGLIB) $(ZLIB) $(SDL_MIXER_LIB) $(LIBMAD_LIB)

all: base_lib $(LIB_OUT) $(EXE_OUT) $(PARSER_OUT) $(PROCESSOR_OUT)


LIB_OBJS = \
	gameswf_xml.$(OBJ_EXT)	        \
	gameswf_movie.$(OBJ_EXT)	\
	gameswf_xmlsocket.$(OBJ_EXT)	\
	gameswf_string.$(OBJ_EXT)	\
	gameswf_action.$(OBJ_EXT)	\
	gameswf_button.$(OBJ_EXT)	\
	gameswf_dlist.$(OBJ_EXT)	\
	gameswf_font.$(OBJ_EXT)	\
	gameswf_fontlib.$(OBJ_EXT)	\
	gameswf_impl.$(OBJ_EXT)	\
	gameswf_log.$(OBJ_EXT)	\
	gameswf_morph2.$(OBJ_EXT)	\
	gameswf_render.$(OBJ_EXT)	\
	gameswf_shape.$(OBJ_EXT)	\
	gameswf_sound.$(OBJ_EXT)	\
	gameswf_stream.$(OBJ_EXT)	\
	gameswf_styles.$(OBJ_EXT)	\
	gameswf_tesselate.$(OBJ_EXT)	\
	gameswf_text.$(OBJ_EXT)		\
	gameswf_types.$(OBJ_EXT)

TEST_PROGRAM_OBJS = \
	gameswf_test_ogl.$(OBJ_EXT) \
	gameswf_render_handler_ogl.$(OBJ_EXT)	\
	gameswf_sound_handler_sdl.$(OBJ_EXT)	\
	gameswf_sound_handler_mp3.$(OBJ_EXT)

PARSER_OBJS = \
	gameswf_parser.$(OBJ_EXT) 

PROCESSOR_OBJS = \
	gameswf_processor.$(OBJ_EXT)

OBJS = $(LIB_OBJS) $(TEST_PROGRAM_OBJS) $(PARSER_OBJS) $(PROCESSOR_OBJS)

gameswf_impl.$(OBJ_EXT): gameswf.h gameswf_impl.h gameswf_types.h

$(LIB_OUT): $(LIB_OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): $(TEST_PROGRAM_OBJS) $(LIB_OUT) $(BASE_LIB)
	$(CC) -o $@ $(TEST_PROGRAM_OBJS) $(LIBS) $(LDFLAGS)


$(PARSER_OUT): $(PARSER_OBJS) $(LIB_OUT) $(BASE_LIB)
	$(CC) -o $@ $(PARSER_OBJS) $(LIBS) $(LDFLAGS)


$(PROCESSOR_OUT): $(PROCESSOR_OBJS) $(LIB_OUT) $(BASE_LIB)
	$(CC) -o $@ $(PROCESSOR_OBJS) $(LIBS) $(LDFLAGS)


base_lib:
	make -C $(TOP)/base

clean:
	make -C $(TOP)/base clean
	-rm $(OBJS) $(LIB_OUT) $(EXE_OUT) $(PARSER_OUT) $(PROCESSOR_OUT)

depend:
	makedepend *.cpp

fresh: clean all
# DO NOT DELETE

gameswf_action.o: gameswf_action.h gameswf.h /usr/include/ctype.h
gameswf_action.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_action.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_action.o: /usr/include/bits/wordsize.h
gameswf_action.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_action.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_action.o: /usr/include/bits/endian.h
gameswf_action.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_action.o: gameswf_types.h gameswf_impl.h gameswf_log.h
gameswf_action.o: /usr/include/assert.h gameswf_stream.h gameswf_sound.h
gameswf_action.o: gameswf_string.h gameswf_xml.h gameswf_xmlsocket.h
gameswf_action.o: gameswf_movie.h gameswf_timers.h /usr/include/sys/time.h
gameswf_action.o: /usr/include/time.h /usr/include/bits/time.h
gameswf_action.o: /usr/include/sys/select.h /usr/include/bits/select.h
gameswf_action.o: /usr/include/bits/sigset.h /usr/include/stdio.h
gameswf_action.o: /usr/include/libio.h /usr/include/_G_config.h
gameswf_action.o: /usr/include/wchar.h /usr/include/bits/wchar.h
gameswf_action.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
gameswf_action.o: /usr/include/bits/sys_errlist.h
gameswf_button.o: gameswf_button.h gameswf_impl.h gameswf.h
gameswf_button.o: /usr/include/ctype.h /usr/include/features.h
gameswf_button.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_button.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_button.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_button.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_button.o: /usr/include/bits/endian.h
gameswf_button.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_button.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_button.o: /usr/include/assert.h gameswf_sound.h gameswf_render.h
gameswf_button.o: gameswf_stream.h
gameswf_dlist.o: gameswf_dlist.h gameswf_types.h gameswf_impl.h gameswf.h
gameswf_dlist.o: /usr/include/ctype.h /usr/include/features.h
gameswf_dlist.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_dlist.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_dlist.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_dlist.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_dlist.o: /usr/include/bits/endian.h
gameswf_dlist.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_dlist.o: gameswf_action.h gameswf_log.h /usr/include/assert.h
gameswf_dlist.o: gameswf_render.h
gameswf_font.o: gameswf_font.h gameswf.h /usr/include/ctype.h
gameswf_font.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_font.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_font.o: /usr/include/bits/wordsize.h
gameswf_font.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_font.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_font.o: /usr/include/bits/endian.h
gameswf_font.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_font.o: gameswf_types.h gameswf_impl.h gameswf_action.h gameswf_log.h
gameswf_font.o: /usr/include/assert.h gameswf_stream.h gameswf_shape.h
gameswf_font.o: gameswf_styles.h
gameswf_fontlib.o: gameswf.h /usr/include/ctype.h /usr/include/features.h
gameswf_fontlib.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_fontlib.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_fontlib.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_fontlib.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_fontlib.o: /usr/include/bits/endian.h
gameswf_fontlib.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_fontlib.o: gameswf_font.h gameswf_types.h gameswf_impl.h
gameswf_fontlib.o: gameswf_action.h gameswf_log.h /usr/include/assert.h
gameswf_fontlib.o: gameswf_render.h gameswf_shape.h gameswf_styles.h
gameswf_fontlib.o: gameswf_tesselate.h
gameswf_impl.o: gameswf_action.h gameswf.h /usr/include/ctype.h
gameswf_impl.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_impl.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_impl.o: /usr/include/bits/wordsize.h
gameswf_impl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_impl.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_impl.o: /usr/include/bits/endian.h
gameswf_impl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_impl.o: gameswf_types.h gameswf_button.h gameswf_impl.h gameswf_log.h
gameswf_impl.o: /usr/include/assert.h gameswf_sound.h gameswf_font.h
gameswf_impl.o: gameswf_fontlib.h gameswf_morph2.h gameswf_shape.h
gameswf_impl.o: gameswf_styles.h gameswf_render.h gameswf_stream.h
gameswf_impl.o: gameswf_dlist.h /usr/include/string.h
gameswf_impl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/float.h
gameswf_log.o: gameswf_log.h gameswf.h /usr/include/ctype.h
gameswf_log.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_log.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_log.o: /usr/include/bits/wordsize.h
gameswf_log.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_log.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_log.o: /usr/include/bits/endian.h
gameswf_log.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_log.o: /usr/include/stdio.h /usr/include/libio.h
gameswf_log.o: /usr/include/_G_config.h /usr/include/wchar.h
gameswf_log.o: /usr/include/bits/wchar.h /usr/include/gconv.h
gameswf_log.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gameswf_morph2.o: gameswf_morph2.h gameswf_shape.h gameswf_styles.h
gameswf_morph2.o: gameswf_impl.h gameswf.h /usr/include/ctype.h
gameswf_morph2.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_morph2.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_morph2.o: /usr/include/bits/wordsize.h
gameswf_morph2.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_morph2.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_morph2.o: /usr/include/bits/endian.h
gameswf_morph2.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_morph2.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_morph2.o: /usr/include/assert.h gameswf_stream.h
gameswf_morph.o: gameswf_morph.h gameswf_shape.h gameswf_styles.h
gameswf_morph.o: gameswf_impl.h gameswf.h /usr/include/ctype.h
gameswf_morph.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_morph.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_morph.o: /usr/include/bits/wordsize.h
gameswf_morph.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_morph.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_morph.o: /usr/include/bits/endian.h
gameswf_morph.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_morph.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_morph.o: /usr/include/assert.h gameswf_tesselate.h gameswf_stream.h
gameswf_morph.o: gameswf_render.h
gameswf_morph.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/float.h
gameswf_movie.o: gameswf_movie.h gameswf_log.h gameswf_action.h gameswf.h
gameswf_movie.o: /usr/include/ctype.h /usr/include/features.h
gameswf_movie.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_movie.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_movie.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_movie.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_movie.o: /usr/include/bits/endian.h
gameswf_movie.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_movie.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
gameswf_parser.o: gameswf_stream.h gameswf_log.h gameswf.h
gameswf_parser.o: /usr/include/ctype.h /usr/include/features.h
gameswf_parser.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_parser.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_parser.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_parser.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_parser.o: /usr/include/bits/endian.h
gameswf_parser.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_processor.o: gameswf.h /usr/include/ctype.h /usr/include/features.h
gameswf_processor.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_processor.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_processor.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_processor.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_processor.o: /usr/include/bits/endian.h
gameswf_processor.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_render.o: gameswf_render.h gameswf_types.h gameswf.h
gameswf_render.o: /usr/include/ctype.h /usr/include/features.h
gameswf_render.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_render.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_render.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_render.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_render.o: /usr/include/bits/endian.h
gameswf_render.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_render.o: gameswf_log.h
gameswf_render_handler_ogl.o: gameswf.h /usr/include/ctype.h
gameswf_render_handler_ogl.o: /usr/include/features.h
gameswf_render_handler_ogl.o: /usr/include/sys/cdefs.h
gameswf_render_handler_ogl.o: /usr/include/gnu/stubs.h
gameswf_render_handler_ogl.o: /usr/include/bits/types.h
gameswf_render_handler_ogl.o: /usr/include/bits/wordsize.h
gameswf_render_handler_ogl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_render_handler_ogl.o: /usr/include/bits/typesizes.h
gameswf_render_handler_ogl.o: /usr/include/endian.h
gameswf_render_handler_ogl.o: /usr/include/bits/endian.h
gameswf_render_handler_ogl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_render_handler_ogl.o: gameswf_types.h /usr/include/string.h
gameswf_shape.o: gameswf_shape.h gameswf_styles.h gameswf_impl.h gameswf.h
gameswf_shape.o: /usr/include/ctype.h /usr/include/features.h
gameswf_shape.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_shape.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_shape.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_shape.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_shape.o: /usr/include/bits/endian.h
gameswf_shape.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_shape.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_shape.o: /usr/include/assert.h gameswf_render.h gameswf_stream.h
gameswf_shape.o: gameswf_tesselate.h
gameswf_shape.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/float.h
gameswf_sound.o: gameswf_sound.h gameswf_impl.h gameswf.h
gameswf_sound.o: /usr/include/ctype.h /usr/include/features.h
gameswf_sound.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_sound.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_sound.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_sound.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_sound.o: /usr/include/bits/endian.h
gameswf_sound.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_sound.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_sound.o: /usr/include/assert.h gameswf_stream.h
gameswf_stream.o: gameswf_stream.h gameswf_log.h gameswf_types.h
gameswf_stream.o: /usr/include/string.h /usr/include/features.h
gameswf_stream.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_stream.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_string.o: gameswf_string.h gameswf_log.h gameswf_action.h gameswf.h
gameswf_string.o: /usr/include/ctype.h /usr/include/features.h
gameswf_string.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_string.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_string.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_string.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_string.o: /usr/include/bits/endian.h
gameswf_string.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_string.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
gameswf_styles.o: gameswf_styles.h gameswf_impl.h gameswf.h
gameswf_styles.o: /usr/include/ctype.h /usr/include/features.h
gameswf_styles.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_styles.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_styles.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_styles.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_styles.o: /usr/include/bits/endian.h
gameswf_styles.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_styles.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_styles.o: /usr/include/assert.h gameswf_render.h gameswf_stream.h
gameswf_tesselate.o: gameswf_tesselate.h gameswf.h /usr/include/ctype.h
gameswf_tesselate.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_tesselate.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_tesselate.o: /usr/include/bits/wordsize.h
gameswf_tesselate.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_tesselate.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_tesselate.o: /usr/include/bits/endian.h
gameswf_tesselate.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_tesselate.o: gameswf_types.h /usr/include/stdlib.h
gameswf_test_ogl.o: gameswf.h /usr/include/ctype.h /usr/include/features.h
gameswf_test_ogl.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_test_ogl.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_test_ogl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_test_ogl.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_test_ogl.o: /usr/include/bits/endian.h
gameswf_test_ogl.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_test_ogl.o: /usr/include/stdlib.h /usr/include/stdio.h
gameswf_test_ogl.o: /usr/include/libio.h /usr/include/_G_config.h
gameswf_test_ogl.o: /usr/include/wchar.h /usr/include/bits/wchar.h
gameswf_test_ogl.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
gameswf_test_ogl.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
gameswf_test_ogl.o: gameswf_timers.h /usr/include/sys/time.h
gameswf_test_ogl.o: /usr/include/time.h /usr/include/bits/time.h
gameswf_test_ogl.o: /usr/include/sys/select.h /usr/include/bits/select.h
gameswf_test_ogl.o: /usr/include/bits/sigset.h gameswf_log.h gameswf_action.h
gameswf_test_ogl.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
gameswf_text.o: gameswf_impl.h gameswf.h /usr/include/ctype.h
gameswf_text.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_text.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_text.o: /usr/include/bits/wordsize.h
gameswf_text.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_text.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_text.o: /usr/include/bits/endian.h
gameswf_text.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_text.o: gameswf_action.h gameswf_types.h gameswf_log.h
gameswf_text.o: /usr/include/assert.h gameswf_shape.h gameswf_styles.h
gameswf_text.o: gameswf_stream.h gameswf_font.h gameswf_fontlib.h
gameswf_text.o: gameswf_render.h
gameswf_timers.o: gameswf_log.h gameswf_action.h gameswf.h
gameswf_timers.o: /usr/include/ctype.h /usr/include/features.h
gameswf_timers.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_timers.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_timers.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_timers.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_timers.o: /usr/include/bits/endian.h
gameswf_timers.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_timers.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
gameswf_timers.o: gameswf_timers.h /usr/include/sys/time.h
gameswf_timers.o: /usr/include/time.h /usr/include/bits/time.h
gameswf_timers.o: /usr/include/sys/select.h /usr/include/bits/select.h
gameswf_timers.o: /usr/include/bits/sigset.h
gameswf_tools.o: gameswf.h /usr/include/ctype.h /usr/include/features.h
gameswf_tools.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_tools.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_tools.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_tools.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_tools.o: /usr/include/bits/endian.h
gameswf_tools.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_tools.o: gameswf_log.h gameswf_stream.h gameswf_types.h
gameswf_types.o: gameswf_types.h gameswf_log.h gameswf_stream.h
gameswf_types.o: gameswf_render.h gameswf.h /usr/include/ctype.h
gameswf_types.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_types.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_types.o: /usr/include/bits/wordsize.h
gameswf_types.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_types.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_types.o: /usr/include/bits/endian.h
gameswf_types.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_types.o: /usr/include/string.h
gameswf_xml.o: gameswf_log.h gameswf_action.h gameswf.h /usr/include/ctype.h
gameswf_xml.o: /usr/include/features.h /usr/include/sys/cdefs.h
gameswf_xml.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
gameswf_xml.o: /usr/include/bits/wordsize.h
gameswf_xml.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_xml.o: /usr/include/bits/typesizes.h /usr/include/endian.h
gameswf_xml.o: /usr/include/bits/endian.h
gameswf_xml.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_xml.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
gameswf_xml.o: gameswf_string.h gameswf_xml.h gameswf_xmlsocket.h
gameswf_xmlsocket.o: /usr/include/sys/types.h /usr/include/features.h
gameswf_xmlsocket.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
gameswf_xmlsocket.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
gameswf_xmlsocket.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stddef.h
gameswf_xmlsocket.o: /usr/include/bits/typesizes.h /usr/include/time.h
gameswf_xmlsocket.o: /usr/include/sys/time.h /usr/include/bits/time.h
gameswf_xmlsocket.o: /usr/include/sys/select.h /usr/include/bits/select.h
gameswf_xmlsocket.o: /usr/include/bits/sigset.h /usr/include/unistd.h
gameswf_xmlsocket.o: /usr/include/bits/posix_opt.h
gameswf_xmlsocket.o: /usr/include/bits/confname.h /usr/include/netinet/in.h
gameswf_xmlsocket.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gameswf_xmlsocket.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
gameswf_xmlsocket.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
gameswf_xmlsocket.o: /usr/include/limits.h
gameswf_xmlsocket.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/limits.h
gameswf_xmlsocket.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
gameswf_xmlsocket.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
gameswf_xmlsocket.o: /usr/include/endian.h /usr/include/bits/endian.h
gameswf_xmlsocket.o: /usr/include/bits/byteswap.h /usr/include/arpa/inet.h
gameswf_xmlsocket.o: /usr/include/netdb.h /usr/include/bits/netdb.h
gameswf_xmlsocket.o: /usr/include/errno.h /usr/include/bits/errno.h
gameswf_xmlsocket.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
gameswf_xmlsocket.o: /usr/include/sys/param.h /usr/include/linux/limits.h
gameswf_xmlsocket.o: /usr/include/linux/param.h /usr/include/asm/param.h
gameswf_xmlsocket.o: gameswf_log.h gameswf_xmlsocket.h gameswf_action.h
gameswf_xmlsocket.o: gameswf.h /usr/include/ctype.h
gameswf_xmlsocket.o: /usr/lib/gcc/i386-redhat-linux/3.4.2/include/stdarg.h
gameswf_xmlsocket.o: gameswf_types.h gameswf_impl.h /usr/include/assert.h
