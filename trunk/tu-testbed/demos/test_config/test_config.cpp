// test_config.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test for engine config stuff.


#include <stdio.h>
#include <string.h>

#include <config.h>


declare_cfloat( test_cfloat, 55.f );
declare_cvar( test_cvar, "testfilename.txt" );

int	main()
{
	// Show initial test var values.
	printf( "test_cfloat = %f, test_cvar = %s\n", float(test_cfloat), (const char*) test_cvar );

	// Change the values, and show the results.
	test_cfloat = 100.5f;
	test_cvar = "newfilename.jpg";
	printf( "test_cfloat = %f, test_cvar = %s\n", float(test_cfloat), (const char*) test_cvar );

	// Interpreter loop, to fiddle with cfloats & cvars...
	for (;;) {
		// Get a command line.
		char	buffer[1000];
		printf( "> " );
		fgets( buffer, 1000, stdin );

		if ( strstr( buffer, "quit" ) ) {
			break;
		}
		
		// Execute it.
		lua_dostring( config::g_luastate, buffer );
	}

	// Show ending var values.
	printf( "test_cfloat = %f, test_cvar = %s\n", float(test_cfloat), (const char*) test_cvar );

	return 0;
}

