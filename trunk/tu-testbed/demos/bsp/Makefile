# Makefile for bspdemo program


include ../../config

all: engine bspdemo$(EXE_EXT)

INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS)

%.obj : %.cpp
	$(CC) -c $^ $(CFLAGS)


BSPDEMO_OBJS = bspdemo.$(OBJ_EXT)

bspdemo$(EXE_EXT): $(BSPDEMO_OBJS) $(ENGINE)
	$(CC) -o $@ $(BSPDEMO_OBJS) $(ENGINE) $(LDFLAGS) $(LIBS) $(ENGINE)

engine:
	cd ../../engine; make
