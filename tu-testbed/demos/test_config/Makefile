# Makefile for config demo

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

SDL_INCLUDE := SDL
LUA_INCLUDE := c:/thatcher/lua/include
ENGINE_INCLUDE := c:/thatcher/tu-testbed/engine

INCLUDE := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)

LUA := c:/thatcher/lua/lib/liblua.a c:/thatcher/lua/lib/liblualib.a
ENGINE := c:/thatcher/tu-testbed/engine/engine.a

LIBS := $(ENGINE) $(LUA)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -g
LDFLAGS := -lGL -lGLU -lSDL_image $(SDL_LDFLAGS) -g
# -lstdc++

OBJS := test_config.o

.cpp.o :
	g++ -c $^ $(CFLAGS)

all: test_config

test_config: $(OBJS)
	g++ -o test_config $(OBJS) $(LIBS)
