// test_swf.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test program for swf parser.


#include "SDL.h"
#include "swf.h"
#include <stdlib.h>
#include "engine/ogl.h"
#include "engine/utility.h"


#undef main	// SDL wackiness


#define SCALE	6.0f
#define OVERSIZE	1.0f


#if 1
// xxxx HACK FOR DEBUGGING
int	hilite_depth = -1;
#endif // 0


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

	int	width = imin(int(m->get_width() * SCALE), 800);
	int	height = imin(int(m->get_height() * SCALE), 600);

	// Set the video mode.
	if (SDL_SetVideoMode(width, height, 16, SDL_OPENGL) == 0)
	{
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	// Turn on alpha blending.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	bool	paused = false;
	float	speed_scale = 1.0f;
	Uint32	last_ticks = SDL_GetTicks();
	for (;;)
	{
		Uint32	ticks = SDL_GetTicks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		if (paused == true)
		{
			delta_t = 0.0f;
		}

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
				else if (key == SDLK_p)
				{
					// Toggle paused state.
					paused = ! paused;
					printf("paused = %d\n", int(paused));
				}
#if 1
				else if (key == SDLK_MINUS)
				{
					hilite_depth--;
					printf("hilite depth = %d\n", hilite_depth);
				}
				else if (key == SDLK_EQUALS)
				{
					hilite_depth++;
					printf("hilite depth = %d\n", hilite_depth);
				}
#endif // 0
				else if (key == SDLK_LEFTBRACKET || key == SDLK_KP_MINUS)
				{
					paused = true;
					delta_t = -0.1f;
				}
				else if (key == SDLK_RIGHTBRACKET || key == SDLK_KP_PLUS)
				{
					paused = true;
					delta_t = +0.1f;
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

		int	frame0 = m->get_current_frame();
		{
			m->advance(delta_t * speed_scale);
		}
		int	frame1 = m->get_current_frame();
		if (frame0 != frame1)
		{
			// Movie frame has changed -- show the new frame.

			glDisable(GL_DEPTH_TEST);	// Disable depth testing.
			glDrawBuffer(GL_BACK);
			glClear(GL_COLOR_BUFFER_BIT);

			m->display();

			SDL_GL_SwapBuffers();
		}

		SDL_Delay(10);
	}

	SDL_RWclose(in);

	return 0;
}

