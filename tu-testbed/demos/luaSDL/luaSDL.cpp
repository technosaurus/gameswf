// luaSDL.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lua toplevel, bound to SDL.  (experimental console GUI in progress, off by default)


#include <stdio.h>
#include <string.h>

#include <engine/config.h>

//#define USE_GUI	1

#ifdef USE_GUI
#include "gui_console.h"
#endif // USE_GUI


// Init function to bind SDL to Lua.
extern "C" int luaSDL_initialize(lua_State *L);


int	main(int argc, char *argv[])
{
	config::open();

	luaSDL_initialize(config::L);

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
