// luatoy.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lua programming console (with optional GUI) and SDL binding.


#include <stdio.h>
#include <string.h>

#include <engine/config.h>

//#define USE_GUI	1

#ifdef USE_GUI
#include "gui_console.h"
#endif // USE_GUI


// Init function to bind SDL to Lua.
extern "C" int luaSDL_initialize(lua_State *L);


static int	bit_or(lua_State* L)
// Lua wrapper for bitwise-or.  Casts all its args to integers, and
// returns the bitwise or of all its args.
{
	int	args = lua_gettop(L);
	int	result = 0;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			lua_error(L, "non-numeric arg to bit_or()");
		}
		result |= (int) lua_tonumber(L, i);
	}
	lua_pushnumber(L, result);
	return 1;
}


static int	bit_and(lua_State* L)
// Lua wrapper for bitwise-and.  Casts all its args to integers, and
// returns the bitwise and of all its args.
{
	int	args = lua_gettop(L);
	int	result = -1;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			lua_error(L, "non-numeric arg to bit_or()");
		}
		result &= (int) lua_tonumber(L, i);
	}
	lua_pushnumber(L, result);
	return 1;
}


static int	bit_not(lua_State* L)
// Lua wrapper for bitwise-not.  Casts its args to an integers, and
// returns ~arg.
{
	if (!lua_isnumber(L, 1)) {
		lua_error(L, "non-numeric arg to bit_or()");
	}
	lua_pushnumber(L, ~ (int) lua_tonumber(L, 1));
	return 1;
}


int	main(int argc, char *argv[])
{
	config::open();

	luaSDL_initialize(config::L);
	lua_register(config::L, "bit_or", bit_or);
	lua_register(config::L, "bit_and", bit_and);
	lua_register(config::L, "bit_not", bit_not);

	// Load any files specified on the command-line.
	for (int i = 1; i < argc; i++) {
		lua_dofile(config::L, argv[i]);
	}

	if (argc <= 1) {

#ifdef USE_GUI

		gui_console();

#else // not USE_GUI

		// Interpreter loop.
		for (;;) {
			// TODO: update this with lua.exe -like support for backslash input extending, etc.

			// Get a command line.
			char	buffer[1000];
			printf( "> " );
			fgets( buffer, 1000, stdin );

			// Execute it.
			lua_dostring( config::L, buffer );
		}

#endif // not USE_GUI

	}

	return 0;
}
