// test_swf.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test program for swf parser.


#include "SDL.h"
#include "swf.h"
#include <stdlib.h>
#include "engine/ogl.h"


#undef main	// SDL wackiness


#define SCALE	2.0f
#define OVERSIZE	1.0f


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

	swf::movie_interface*	m = swf::create_movie(in);

	// Initialize the SDL subsystems we're using.
	if (SDL_Init(SDL_INIT_VIDEO /* | SDL_INIT_JOYSTICK | SDL_INIT_CDROM | SDL_INIT_AUDIO*/))
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Set the video mode.
	if (SDL_SetVideoMode(int(m->get_width() * SCALE), int(m->get_height() * SCALE), 16, SDL_OPENGL) == 0)
	{
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	glMatrixMode(GL_PROJECTION);
	glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Uint32	last_ticks = SDL_GetTicks();
	for (;;)
	{
		Uint32	ticks = SDL_GetTicks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		// Handle input.
		SDL_Event	event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
			{
				int	key = event.key.keysym.sym;

				if (key == SDLK_q || key == SDLK_ESCAPE)
				{
					exit(0);
				}
				break;
			}

			case SDL_QUIT:
				exit(0);
				break;

			default:
				break;
			}
		}

		m->advance(delta_t);

		glClearDepth(1.0f);		// Depth Buffer Setup
		glDisable(GL_DEPTH_TEST);	// Enables Depth Testing
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT);

		m->display();

		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

	SDL_RWclose(in);

	return 0;
}

