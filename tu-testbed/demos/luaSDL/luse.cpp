// luause.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 2 Feb 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lua toplevel, with "lua_uselib" extension for loading modules.


#include <stdio.h>
#include <string.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "luselib.h"
}


int	main(int argc, char *argv[])
{
	lua_State*	L = lua_open(2048);

	// Init the standard Lua libs.
	lua_baselibopen( L );
	lua_iolibopen( L );
	lua_strlibopen( L );
	lua_mathlibopen( L );

	// Init the "use" lib.
	lua_uselibopen(L);

	// Load any files specified on the command-line.
	for (int i = 1; i < argc; i++) {
		lua_dofile(L, argv[i]);
	}

	if (argc <= 1) {
		// Interpreter loop.
		for (;;) {
			// TODO: just use lua.exe, with line-editing support, etc.

			// Get a command line.
			char	buffer[1000];
			printf( "> " );
			fgets( buffer, 1000, stdin );

			// Execute it.
			lua_dostring(L, buffer);
		}
	}

	return 0;
}
