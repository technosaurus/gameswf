# Makefile for tu-testbed/engine library


TOP = ..
include $(TOP)/config

ifeq ($(TU_TESTBED_COMPILER), gcc)
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)
endif

SDL_INCLUDE := -ISDL
LUA_INCLUDE := -I$(TOP)/../lua-4.0.1/include

INCLUDE_PATHS := $(SDL_INCLUDE) $(LUA_INCLUDE) $(ENGINE_INCLUDE) $(LIBPNG_INCLUDE) $(ZLIB_INCLUDE) $(JPEGLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := -lGL -lGLU -lSDL_image $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS)
# -lstdc++

# Source files.
SRCS := \
	dlmalloc.c	\
	utility.cpp	\
	config.cpp \
	geometry.cpp \
	cull.cpp \
	bsp.cpp \
	ogl.cpp \
	jpeg.cpp \
	image.cpp \
	image_filters.cpp \
	tqt.cpp
OBJS := $(SRCS:.cpp=.$(OBJ_EXT))
OBJS := $(OBJS:.c=.$(OBJ_EXT))


ENGINE = engine.$(LIB_EXT)

all: $(ENGINE)

$(ENGINE): $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$(ENGINE) $(OBJS)

clean:
	-@rm -f $(ENGINE) $(OBJS)
