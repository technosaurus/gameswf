// gameswf_processor.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A SWF preprocessor for the gameswf library.  Loads a set of SWF
// files and generates precomputed data such as font bitmaps and shape
// tesselation meshes.  The precomputed data can be appended to the
// original SWF files, so that gameswf can more rapidly load those SWF
// files later.


#include "base/tu_file.h"
#include "base/container.h"
#include "gameswf.h"


static bool	s_verbose = false;


static void	message_log(const char* message)
// Process a log message.
{
	if (s_verbose)
	{
		fputs(message, stdout);
                //flush(stdout); // needed on osx for some reason
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


static void	print_usage()
{
	printf(
		"gameswf_processor -- a SWF preprocessor for gameswf.\n"
		"\n"
		"This program has been donated to the Public Domain.\n"
		"See http://tulrich.com/geekstuff/gameswf.html for more info.\n"
		"\n"
		"usage: gameswf_processor [options] [swf files to process...]\n"
		"\n"
		"Preprocesses the given SWF movie files.  Optionally insert preprocessed shape\n"
		"and font data back into the input files, so the SWF files can be loaded\n"
		"faster by gameswf.\n"
		"\n"
		"options:\n"
		"\n"
		"  -h          Print this info.\n"
		"  -a          Append data into the input files.\n"
		"  -v          Be verbose; i.e. print log messages to stdout\n"
		"  -vp         Be verbose about movie parsing\n"
		"  -va         Be verbose about ActionScript\n"
		);
}


static int	process_movie(const char* filename);


static bool	s_append = false;
static bool	s_stop_on_errors = true;


int	main(int argc, char *argv[])
{
	assert(tu_types_validate());

	array<const char*> infiles;

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
			else if (argv[arg][1] == 'a')
			{
				// Append cached data to the input files.
				s_append = true;
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
			infiles.push_back(argv[arg]);
		}
	}

	if (infiles.size() == 0)
	{
		printf("no input files\n");
		print_usage();
		exit(1);
	}

	gameswf::register_file_opener_callback(file_opener);
	gameswf::set_log_callback(log_callback);
        
	// Process the movies.
	for (int i = 0, n = infiles.size(); i < n; i++)
	{
		int	error = process_movie(infiles[i]);
		if (error)
		{
			if (s_stop_on_errors)
			{
				// Fail.
				fprintf(stderr, "error processing movie '%s', quitting\n", infiles[i]);
				exit(1);
			}
		}
	}

	return 0;
}


int	process_movie(const char* filename)
// Process the given SWF file.  If s_append is true, then append
// cached precomputed data to the original file.
{
	tu_file*	in = new tu_file(filename, "rb");
	if (in->get_error())
	{
		printf("can't open '%s' for input\n", filename);
		delete in;
		return 1;
	}
	gameswf::movie_interface*	m = gameswf::create_movie(in);
	delete in;
	in = NULL;

	int	kick_count = 0;

	// Run through the movie.
	for (;;)
	{
		// @@ do we also have to run through all sprite frames
		// as well?
		//
		// @@ also, ActionScript can rescale things
		// dynamically -- we can't really do much about that I
		// guess?
		//
		// @@ Maybe we should allow the user to specify some
		// safety margin on scaled shapes.

		int	last_frame = m->get_current_frame();
		m->advance(0.010f);
		m->display();

		if (m->get_current_frame() == m->get_frame_count() - 1)
		{
			// Done.
			break;
		}

		if (m->get_play_state() == gameswf::movie_interface::STOP)
		{
			// Kick the movie.
			printf("kicking movie, kick ct = %d\n", kick_count);
			m->goto_frame(last_frame + 1);
			m->set_play_state(gameswf::movie_interface::PLAY);
			kick_count++;

			if (kick_count > 10)
			{
				printf("movie is stalled; giving up on playing it through.\n");
				break;
			}
		}
		else if (m->get_current_frame() < last_frame)
		{
			// Hm, apparently we looped back.  Skip ahead...
			printf("loop back; jumping to frame %d\n", last_frame);
			m->goto_frame(last_frame + 1);
		}
		else
		{
			kick_count = 0;
		}
	}

	if (s_append)
	{
		// grab computed data for this movie, dump it into a buffer
		tu_file	cached_data(tu_file::memory_buffer);
		m->output_cached_data(&cached_data);

		printf("generated %d bytes of cached data\n", cached_data.get_position());

		cached_data.set_position(0);	// rewind

		// // xxx temp debug code: dump cached data to stdout
		// tu_file	tu_stdout(stdout, false);
		// tu_stdout.copy_from(&cached_data);

		// if (s_append)
		// {
		//	open file for i/o;
		//	remove any existing cache tags;
		// 	append our cache tag;
		//	write out the file;
		// }
	}
	
	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
