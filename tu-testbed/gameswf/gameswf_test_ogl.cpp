// gameswf_test_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A minimal test player app for the gameswf library.


#include "SDL.h"
#include "gameswf.h"
#include <stdlib.h>
#include "engine/ogl.h"
#include "engine/utility.h"




#define OVERSIZE	1.0f


static float	s_scale = 1.0f;
static bool	s_antialiased = false;
static bool	s_loop = true;


#undef main	// SDL wackiness
int	main(int argc, char *argv[])
{
	const char* infile = NULL;

	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.

			if (argv[arg][1] == 's')
			{
				// Scale.
				arg++;
				if (arg < argc)
				{
					s_scale = fclamp((float) atof(argv[arg]), 0.01f, 100.f);
				}
				else
				{
					printf("-s arg must be followed by a scale value\n");
					// print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'a')
			{
				// Set antialiasing on or off.
				arg++;
				if (arg < argc)
				{
					s_antialiased = atoi(argv[arg]) ? true : false;
				}
				else
				{
					printf("-a arg must be followed by 0 or 1 to disable/enable antialiasing\n");
					// print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'l')
			{
				// Set loop mode on or off.
				arg++;
				if (arg < argc)
				{
					s_loop = atoi(argv[arg]) ? true : false;
				}
				else
				{
					printf("-l arg must be followed by 0 or 1 to disable/enable looping\n");
					// print_usage();
					exit(1);
				}
			}
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

	gameswf::set_pixel_scale(s_scale);
	gameswf::set_antialiased(s_antialiased);
	gameswf::movie_interface*	m = gameswf::create_movie(in);

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
//	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int	width = int(m->get_width() * s_scale);
	int	height = int(m->get_height() * s_scale);

	// Set the video mode.
	if (SDL_SetVideoMode(width, height, 16 /* 32 */, SDL_OPENGL) == 0)
	{
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	ogl::open();

	// Generate cached textured versions of fonts.
	gameswf::fontlib::generate_font_bitmaps();

	// Turn on alpha blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Turn on line smoothing.  Antialiased lines can be used to
	// smooth the outsides of shapes.
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);	// GL_NICEST, GL_FASTEST, GL_DONT_CARE

	glMatrixMode(GL_PROJECTION);
	glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Mouse state.
	int	mouse_x = 0;
	int	mouse_y = 0;
	int	mouse_buttons = 0;

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
				else if (key == SDLK_r)
				{
					// Restart the movie.
					m->restart();
				}
#if 0
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
				else if (key == SDLK_a)
				{
					// Toggle antialiasing.
					s_antialiased = !s_antialiased;
					gameswf::set_antialiased(s_antialiased);
				}
				else if (key == SDLK_l)
				{
					// Toggle looping.
					s_loop = !s_loop;
				}

				break;
			}

			case SDL_MOUSEMOTION:
				mouse_x = (int) (event.motion.x / s_scale);
				mouse_y = (int) (event.motion.y / s_scale);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int	mask = 1 << (event.button.button);
				if (event.button.state == SDL_PRESSED)
				{
					mouse_buttons |= mask;
				}
				else
				{
					mouse_buttons &= ~mask;
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

		m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);

		m->advance(delta_t * speed_scale);

		glDisable(GL_DEPTH_TEST);	// Disable depth testing.
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT);

		m->display();

		if (s_loop
		    && m->get_current_frame() >= m->get_frame_count() - 1)
		{
			m->restart();
		}

		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

	SDL_RWclose(in);

	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
