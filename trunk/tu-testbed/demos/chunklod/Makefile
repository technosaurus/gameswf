# Makefile for chunklod programs


include ../../config

INCLUDE_DIRS := $(ENGINE_INCLUDE) -ISDL


all: heightfield_chunker.exe chunkdemo.exe

heightfield_chunker$(EXE_EXT): heightfield_chunker.obj
	$(CC) -o $@ heightfield_chunker.obj SDL.lib

CHUNKDEMO_OBJS = chunkdemo.obj chunklod.obj

chunkdemo$(EXE_EXT): $(CHUNKDEMO_OBJS) $(ENGINE)
	$(CC) -o $@ $(CHUNKDEMO_OBJS) $(ENGINE) $(LDFLAGS) SDL.lib $(ENGINE)
