// luaSDL.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lua toplevel, bound to SDL.


#include <stdio.h>
#include <string.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "luselib.h"	// TODO: move lua_uselibopen() to engine/config.cpp
}

// Init function to bind SDL to Lua.
extern "C" int tolua_SDL_open(lua_State *L);
extern "C" void tolua_SDL_close(lua_State *L);


static const int	LUA_STACK_SIZE = 2048;


int	main(int argc, char *argv[])
{
	// Init Lua.
	lua_State*	L = lua_open(LUA_STACK_SIZE);

	// Init the standard Lua libs.
	lua_baselibopen(L);
	lua_iolibopen(L);
	lua_strlibopen(L);
	lua_mathlibopen(L);

//	lua_uselibopen(config::L);

//	luaSDL_initialize(config::L);
	tolua_SDL_open(L);

	// Load any files specified on the command-line.
	for (int i = 1; i < argc; i++) {
		lua_dofile(L, argv[i]);
	}

	if (argc <= 1) {
		// Interpreter loop.
		for (;;) {
			// TODO: update this with lua.exe -like support for backslash input extending, etc.

			// Get a command line.
			char	buffer[1000];
			printf("> ");
			fgets(buffer, 1000, stdin);

			// Execute it.
			lua_dostring(L, buffer);
		}
	}

	tolua_SDL_close(L);

	return 0;
}
