# Makefile for tu-testbed/engine library

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

SDL_INCLUDE := SDL
LUA_INCLUDE := ../lua/include

INCLUDE := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -g
LDFLAGS := -lGL -lGLU -lSDL_image $(SDL_LDFLAGS) -g
# -lstdc++

OBJS := config.o

.cpp.o :
	g++ -c $^ $(CFLAGS)

all: engine.a

engine.a: $(OBJS)
	ar -r engine.a $(OBJS)

clean:
	rm engine.a *.o
