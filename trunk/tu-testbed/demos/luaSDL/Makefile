# Makefile for luaSDL

include ../../config

SDL_INCLUDE := SDL
LUA := c:/thatcher/lua-4.0
LUA_INCLUDE := $(LUA)/include

# gtk...
GTK_DIR := c:/thatcher/gtk/src/gtk+/gtk
GDK_DIR := c:/thatcher/gtk/src/gtk+
GLIB_DIR := c:/thatcher/gtk/src/glib


INCLUDE_PATHS := -I$(SDL_INCLUDE) -I$(LUA_INCLUDE) -I$(ENGINE_INCLUDE) -I$(GTK_DIR) -I$(GDK_DIR) -I$(GDK_DIR)/gdk -I$(GLIB_DIR)

CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS) -DNO_GLIB
LDFLAGS := $(SDL_LDFLAGS) $(LDFLAGS)

LUA_LIBS := $(LUA)/lib/liblua.$(LIB_EXT) $(LUA)/lib/liblualib.$(LIB_EXT)

LIBS := $(ENGINE) $(LUA_LIBS) $(LIBS) ../../../luaswig/luaswig40.$(LIB_EXT) $(GTK_DIR)/gtk-1.3.lib $(GDK_DIR)/gdk/gdk-1.3.lib $(GLIB_DIR)/glib-1.3.lib

OBJS := luaSDL.$(OBJ_EXT) SDL_wrap.$(OBJ_EXT) # gui_console.$(OBJ_EXT)

all: luaSDL$(EXE_EXT)

SDL_wrap.c : SDL.i
	luaswig -shadow SDL.i

luaSDL$(EXE_EXT): $(OBJS) $(ENGINE)
	$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

