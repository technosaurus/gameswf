# Makefile for tu-testbed/engine library


include config

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

SDL_INCLUDE := SDL
LUA_INCLUDE := ../../lua/include

INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := -lGL -lGLU -lSDL_image $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS)
# -lstdc++

# TODO: there's probably some slick way to apply the OBJ_EXT to a file list.
OBJS := \
	config.$(OBJ_EXT)	\
	geometry.$(OBJ_EXT) \
	cull.$(OBJ_EXT)

%.obj : %.cpp
	$(CC) -c $^ $(CFLAGS)


ENGINE = engine.$(LIB_EXT)

all: $(ENGINE)

$(ENGINE): $(OBJS)
	$(LIB) $(LIB_OUT_FLAG)$(ENGINE) $(OBJS)

clean:
	rm $(ENGINE) $(OBJS)
