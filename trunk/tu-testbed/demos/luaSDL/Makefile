# Makefile for luaSDL

include ../../config

SDL_INCLUDE := SDL
LUA := ../../../lua
# c:/thatcher/lua-4.0
LUA_INCLUDE := $(LUA)/include

# gtk...
#GTK_DIR := c:/thatcher/gtk/src/gtk+/gtk
#GDK_DIR := c:/thatcher/gtk/src/gtk+
#GLIB_DIR := c:/thatcher/gtk/src/glib


INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE)
# -I$(GTK_DIR) -I$(GDK_DIR) -I$(GDK_DIR)/gdk -I$(GLIB_DIR)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS) -DNO_GLIB
LDFLAGS := $(SDL_LDFLAGS) $(LDFLAGS)

LUA_LIBS := $(LUA)/lib/lua.lib # $(LUA)/lib/$(LIB_PRE)lua.$(DLL_EXT) $(LUA)/lib/$(LIB_PRE)lualib.$(DLL_EXT)

#LIBS := $(ENGINE) $(LUA_LIBS) $(LIBS) ../../../luaswig/libluaswig40.$(LIB_EXT)
# $(GTK_DIR)/gtk-1.3.lib $(GDK_DIR)/gdk/gdk-1.3.lib $(GLIB_DIR)/glib-1.3.lib

LUA_USE_OBJS := luse.$(OBJ_EXT)
LUA_USE_LIBS := $(LUA_LIBS) $(DLL_LIBS)

LUA_SDL_OBJS := SDL_wrap.$(OBJ_EXT)
LUA_SDL_LIBS := ../../../luaswig/$(LIB_PRE)luaswig40.$(LIB_EXT) $(LUA_LIBS) $(LIBS)

all: luse$(EXE_EXT) $(LIB_PRE)luaSDL.$(DLL_EXT)

SDL_wrap.c : SDL.i
	luaswig -shadow SDL.i

luse$(EXE_EXT): $(LUA_USE_OBJS)
	$(CC) -o $@ $(LUA_USE_OBJS) $(LUA_USE_LIBS)


$(LIB_PRE)luaSDL.$(DLL_EXT): $(LUA_SDL_OBJS)
	$(MAKE_DLL) -o $@ $(LUA_SDL_OBJS) $(LUA_SDL_LIBS)
