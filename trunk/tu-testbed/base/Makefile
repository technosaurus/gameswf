# Makefile for tu-testbed/base library


TOP = ..
include $(TOP)/config

INCLUDE_PATHS := $(SDL_INCLUDE) $(ENGINE_INCLUDE) $(LIBPNG_INCLUDE) $(ZLIB_INCLUDE) $(JPEGLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := -lGL -lGLU $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS)

# Source files.
SRCS := \
	dlmalloc.c	\
	config.cpp \
	ogl.cpp \
	image.cpp \
	image_filters.cpp \
	jpeg.cpp \
	postscript.cpp	\
	tu_file.cpp	\
	tu_file_SDL.cpp	\
	tu_random.cpp	\
	tu_types.cpp	\
	utility.cpp	\
	zlib_adapter.cpp
OBJS := $(SRCS:.cpp=.$(OBJ_EXT))
OBJS := $(OBJS:.c=.$(OBJ_EXT))


BASE = base.$(LIB_EXT)

all: $(BASE)

$(BASE): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$(ENGINE) $(OBJS)

clean:
	-@rm -f $(BASE) $(OBJS)
