// test_config.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test for engine config stuff.


#include <stdio.h>
#include <string.h>

#include <engine/config.h>


cfloat test_cfloat( "test_cfloat", 55.f );
cvalue c10("return 10");
cvar test_cvar( "test_cvar", cvalue( "return 'string bobo'" ) );

// cvalue("poly(vec3(10,10,10), vec3(20,10,10), vec3(10,20,10))");

// cvar("test_float", cfloat(55.f));	// declares global Lua var "test_cfloat"


namespace actorprefs {
	cvar	jumpheight("actorprefs.jumpheight");
	cvar	runspeed("actorprefs.runspeed");
	cvar	color("actorprefs.color");
};


int	main()
{
	config::open();

	lua_dofile(config::L, "property.lua");	// migrate this into config::open().
	lua_dofile(config::L, "init.lua");

	// Show initial test var values.
	printf( "test_cfloat = %f, test_cvar = %s\n",
			(float) test_cfloat,
			(const char*) test_cvar );

	// Change the values, and show the results.
	test_cfloat = 100.5f;
	test_cvar = "newfilename.jpg";
	printf( "test_cfloat = %f, test_cvar = %s\n", (float) test_cfloat, (const char*) test_cvar );

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
		lua_dostring( config::L, buffer );
	}

	// Show ending var values.
	printf( "test_cfloat = %f, test_cvar = %s, test_cvalue = %s\n", (float) test_cfloat, (const char*) cvar("test_cvar"), (const char*) c10 );

	printf("actorprefs.jumpheight = %f, actorprefs.runspeed = %f, actorprefs.color = %s",
		   (float) actorprefs::jumpheight,
		   (float) actorprefs::runspeed,
		   (const char*) actorprefs::color
		);

	return 0;
}

