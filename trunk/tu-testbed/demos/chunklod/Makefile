# Makefile for chunklod programs

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) -g
LDFLAGS := -lGL -lGLU -lSDL_image $(SDL_LDFLAGS) -g
# -lstdc++

.cpp.o :
	g++ -c $^ $(CFLAGS)

all: heightfield_chunker chunkdemo

heightfield_chunker: heightfield_chunker.o
	g++ -o $@ $@.o $(LDFLAGS)

CHUNKDEMO_OBJS = chunkdemo.o chunklod.o geometry.o cull.o

chunkdemo: $(CHUNKDEMO_OBJS)
	g++ -o $@ $(CHUNKDEMO_OBJS) $(LDFLAGS)
