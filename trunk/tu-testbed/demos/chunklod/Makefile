# Makefile for chunklod programs


include ../../config

INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

%.$(OBJ_EXT) : %.cpp
	$(CC) -c $^ $(CFLAGS)


all: heightfield_chunker$(EXE_EXT) chunkdemo$(EXE_EXT)

heightfield_chunker$(EXE_EXT): heightfield_chunker.$(OBJ_EXT)
	$(CC) -o $@ heightfield_chunker.$(OBJ_EXT) $(LIBS) $(LDFLAGS)

CHUNKDEMO_OBJS = chunkdemo.$(OBJ_EXT) chunklod.$(OBJ_EXT)

chunkdemo$(EXE_EXT): $(CHUNKDEMO_OBJS) $(ENGINE)
	$(CC) -o $@ $(CHUNKDEMO_OBJS) $(ENGINE) $(LIBS) $(LDFLAGS)

clean:
	rm $(CHUNKDEMO_OBJS) chunkdemo$(EXE_EXT)
	rm heightfield_chunker.$(OBJ_EXT) heightfield_chunker$(EXE_EXT)
