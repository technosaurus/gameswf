# Makefile for luaSDL

TOP = ../..
include $(TOP)/config

SDL_INCLUDE := SDL
LUA := $(TOP)/../lua-4.0-luselib
LUA_INCLUDE := $(LUA)/include

INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS) -DNO_GLIB
LDFLAGS := $(SDL_LDFLAGS) $(LDFLAGS)

LUA_LIBS := $(LUA)/lib/$(LIB_PRE)lua.$(LIB_EXT) $(LUA)/lib/$(LIB_PRE)lualib.$(LIB_EXT)

LUA_USE_OBJS := lua.$(OBJ_EXT)
LUA_USE_LIBS := $(LUA_LIBS) $(DLL_LIBS) $(LIBS)

TOLUA_LIBS := -ltolua
#$(TOP)/../tolua/lib/libtolua.$(LIB_EXT) 

LUA_SDL_OBJS := SDL_tolua.$(OBJ_EXT)
LUA_SDL_LIBS := -ltolua $(LUA_LIBS) $(LIBS)
# ../../../tolua/lib/$(LIB_PRE)libtolua.$(LIB_EXT) 

SDL_tolua.c: SDL.pkg
	tolua -n SDL -o $@ $^

SDL_tolua.$(OBJ_EXT): SDL_tolua.c
	$(CC) -c $^ -I../../../tolua/include -I$(LUA_INCLUDE)


all: luse$(EXE_EXT) $(LIB_PRE)luaSDL.$(DLL_EXT) luaSDL$(EXE_EXT)

SDL_wrap.c : SDL.i
	luaswig -shadow SDL.i

luse$(EXE_EXT): $(LUA_USE_OBJS)
	$(CC) -o $@ $(LUA_USE_OBJS) $(LUA_USE_LIBS)


luaSDL$(EXE_EXT): luaSDL.$(OBJ_EXT) SDL_tolua.$(OBJ_EXT)
	$(CC) -o $@ $^ $(LUA_LIBS) $(TOLUA_LIBS) $(LIBS)


$(LIB_PRE)luaSDL.$(DLL_EXT): $(LUA_SDL_OBJS)
	$(MAKE_DLL) -o $@ $(LUA_SDL_OBJS) $(LUA_SDL_LIBS)
