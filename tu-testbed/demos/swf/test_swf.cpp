// test_swf.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test program for swf parser.


#include "SDL.h"
#include "swf.h"


int	main(int argc, char *argv[])
{
	const char* infile = NULL;

	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.
			// @@ TODO
		}
		else
		{
			infile = argv[arg];
		}
	}

	if (infile == NULL)
	{
		printf("no input file\n");
		exit(1);
	}

	SDL_RWops*	in = SDL_RWFromFile(infile, "rb");
	if (in == NULL)
	{
		printf("can't open '%s' for input\n", infile);
		exit(1);
	}

	swf::movie*	m = swf::create_movie(in);

	SDL_RWclose(in);

	return 0;
}

