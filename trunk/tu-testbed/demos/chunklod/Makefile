# Makefile for chunklod programs


include ../../config

INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS)

%.obj : %.cpp
	$(CC) -c $^ $(CFLAGS)


all: heightfield_chunker.exe chunkdemo.exe

heightfield_chunker$(EXE_EXT): heightfield_chunker.obj
	$(CC) -o $@ heightfield_chunker.obj SDL.lib

CHUNKDEMO_OBJS = chunkdemo.obj chunklod.obj

chunkdemo$(EXE_EXT): $(CHUNKDEMO_OBJS) $(ENGINE)
	$(CC) -o $@ $(CHUNKDEMO_OBJS) $(ENGINE) $(LDFLAGS) $(LIBS) $(ENGINE)
