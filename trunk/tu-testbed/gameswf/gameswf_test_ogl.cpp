// gameswf_test_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A minimal test player app for the gameswf library.


#include "SDL.h"
#include "gameswf.h"
#include <stdlib.h>
#include <string.h>
#include "base/ogl.h"
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"


void	print_usage()
// Brief instructions.
{
	printf(
		"gameswf_test_ogl -- a test player for the gameswf library.\n"
		"\n"
		"This program has been donated to the Public Domain.\n"
		"See http://tulrich.com/geekstuff/gameswf.html for more info.\n"
		"\n"
		"usage: gameswf_test_ogl [options] movie_file.swf\n"
		"\n"
		"Plays a SWF (Shockwave Flash) movie, using OpenGL and the\n"
		"gameswf library.\n"
		"\n"
		"options:\n"
		"\n"
		"  -h          Print this info.\n"
		"  -s <factor> Scale the movie up/down by the specified factor\n"
		"  -a          Turn antialiasing on/off.  (obsolete)\n"
		"  -c          Build a cache file (for prerendering fonts)\n"
		"  -v          Be verbose; i.e. print log messages to stdout\n"
		"  -va         Be verbose about movie Actions\n"
		"\n"
		"keys:\n"
		"  q or ESC    Quit/Exit\n"
		"  p           Toggle Pause\n"
		"  r           Restart the movie\n"
		"  [ or -      Step back one frame\n"
		"  ] or +      Step forward one frame\n"
		"  a           Toggle antialiasing (doesn't work)\n"
		"  t           Debug.  Put some text in a dynamic text field\n"
		"  b           Toggle background color\n"
		);
}


#define OVERSIZE	1.0f


static float	s_scale = 1.0f;
static bool	s_antialiased = false;
static bool	s_cache = false;
static bool	s_verbose = false;
static bool	s_background = true;


static void	message_log(const char* message)
// Process a log message.
{
	if (s_verbose)
	{
		fputs(message, stdout);
	}
}


static void	log_callback(bool error, const char* message)
// Error callback for handling gameswf messages.
{
	if (error)
	{
		// Log, and also print to stderr.
		message_log(message);
		fputs(message, stderr);
	}
	else
	{
		message_log(message);
	}
}


static tu_file*	file_opener(const char* url)
// Callback function.  This opens files for the gameswf library.
{
	return new tu_file(url, "rb");
}


#undef main	// SDL wackiness
int	main(int argc, char *argv[])
{
	assert(tu_types_validate());

	const char* infile = NULL;

	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.

			if (argv[arg][1] == 'h')
			{
				// Help.
				print_usage();
				exit(1);
			}
			else if (argv[arg][1] == 's')
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
					print_usage();
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
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'c')
			{
				// Build cache file.
				s_cache = true;
			}
			else if (argv[arg][1] == 'v')
			{
				// Be verbose; i.e. print log messages to stdout.
				s_verbose = true;

				if (argv[arg][2] == 'a')
				{
					// Enable spew re: action.
					gameswf::set_verbose_action(true);
				}
				else if (argv[arg][2] == 'p')
				{
					// Enable parse spew.
					gameswf::set_verbose_parse(true);
				}
				// ...
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
		print_usage();
		exit(1);
	}

	gameswf::register_file_opener_callback(file_opener);
	gameswf::set_log_callback(log_callback);
	gameswf::set_antialiased(s_antialiased);

	// Load the movie.
	tu_file*	in = new tu_file(infile, "rb");
	if (in->get_error())
	{
		printf("can't open '%s' for input\n", infile);
		delete in;
		exit(1);
	}
	gameswf::movie_interface*	m = gameswf::create_movie(in);
	delete in;
	in = NULL;

	tu_string	cache_name(infile);
	cache_name += ".cache";

	if (s_cache)
	{
		printf("\nsaving %s...\n", cache_name.c_str());
		gameswf::fontlib::save_cached_font_data(cache_name);
		exit(0);
	}
	
	// Initialize the SDL subsystems we're using.
	if (SDL_Init(SDL_INIT_VIDEO /* | SDL_INIT_JOYSTICK | SDL_INIT_CDROM | SDL_INIT_AUDIO*/))
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

//	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
////	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 5);
////	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
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

	// Try to open cached font textures
	if (!gameswf::fontlib::load_cached_font_data(cache_name))
	{
		// Generate cached textured versions of fonts.
		gameswf::fontlib::generate_font_bitmaps();
	}

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
					//delta_t = -0.1f;
					m->goto_frame(m->get_current_frame()-1);
				}
				else if (key == SDLK_RIGHTBRACKET || key == SDLK_KP_PLUS)
				{
					paused = true;
					//delta_t = +0.1f;
					m->goto_frame(m->get_current_frame()+1);
				}
				else if (key == SDLK_a)
				{
					// Toggle antialiasing.
					s_antialiased = !s_antialiased;
					gameswf::set_antialiased(s_antialiased);
				}
				else if (key == SDLK_t)
				{
					// test text replacement:
					m->set_edit_text("test_text", "set_edit_text was here...\nanother line of text for you to see in the text box");
				}
				else if (key == SDLK_b)
				{
					// toggle background color.
					s_background = !s_background;
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

		m->set_display_viewport(0, 0, width, height);
		m->set_background_alpha(s_background ? 1.0f : 0.05f);

		m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);

		m->advance(delta_t * speed_scale);

		glDisable(GL_DEPTH_TEST);	// Disable depth testing.
		glDrawBuffer(GL_BACK);

		m->display();

		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
