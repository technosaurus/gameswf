// heightfield_shader.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Program to take heightfield input and generate texture for it.


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <engine/utility.h>
#include <engine/container.h>
#include <engine/geometry.h>


void	heightfield_shader(SDL_RWops* rwin, SDL_RWops* out);


void	error(const char* fmt)
// Generic bail function.
//
// TODO: This should go in the engine library, with an
// API to register a callback instead of exit()ing.
{
	printf(fmt);
	exit(1);
}


void	print_usage()
// Print usage info for this program.
{
	// no args, or -h or -?.  print usage.
	printf("heightfield_shader: program for generating a texture for a heightfield\n"
		   "terrain.\n\n"
		   "This program has been donated to the Public Domain by Thatcher Ulrich <tu@tulrich.com>\n\n"
		   "usage: heightfield_shader <input_filename> <output_filename>\n"
		   "\n"
		   "\tThe input filename should either be a .BT format terrain file with\n"
		   "\t(2^N+1) x (2^N+1) datapoints, or a grayscale bitmap.\n"
		);
}


#undef main	// @@ some crazy SDL/WIN32 thing that I don't understand.
int	main(int argc, char* argv[])
// Reads the given .BT terrain file or grayscale bitmap, and generates
// a texture map to use when rendering the heightfield.
{
	// Process command-line options.
	char*	infile = NULL;
	char*	outfile = NULL;
	char*	lightmap_filename = NULL;

	for ( int arg = 1; arg < argc; arg++ ) {
		if ( argv[arg][0] == '-' ) {
			// command-line switch.
			
			switch ( argv[arg][1] ) {
			case 'h':
			case '?':
				print_usage();
				exit( 1 );
				break;
			default:
				printf("error: unknown command-line switch -%c\n", argv[arg][1]);
				exit(1);
				break;
			}

		} else {
			// File argument.
			if ( infile == NULL ) {
				infile = argv[arg];
			} else if ( outfile == NULL ) {
				outfile = argv[arg];
			} else {
				// This looks like extra noise on the command line; complain and exit.
				printf( "argument '%s' looks like extra noise; exiting.\n" );
				print_usage();
				exit( 1 );
			}
		}
	}

	// Make sure we have input and output filenames.
	if ( infile == NULL || outfile == NULL ) {
		// No input or output -- can't run.
		printf( "error: you must specify input and output filenames.\n" );
		print_usage();
		exit( 1 );
	}
	
	SDL_RWops*	in = SDL_RWFromFile(infile, "rb");
	if ( in == 0 ) {
		printf( "error: can't open %s for input.\n", infile );
		exit( 1 );
	}

	SDL_RWops*	out = SDL_RWFromFile(outfile, "wb");
	if ( out == 0 ) {
		printf( "error: can't open %s for output.\n", outfile );
		exit( 1 );
	}

	// Print the parameters.
	printf("infile: %s\n", infile);
	printf("outfile: %s\n", outfile);

	// Process the data.
	heightfield_shader(in, out);

	SDL_RWclose(in);
	SDL_RWclose(out);

	return 0;
}


void	ReadPixel(SDL_Surface *s, int x, int y, Uint8* R, Uint8* G, Uint8* B, Uint8* A)
// Utility function to read a pixel from an SDL surface.
// TODO: Should go in the engine utilities somewhere.
{
	// Get the raw color data for this pixel out of the surface.
	// Location and size depends on surface format.
	Uint32	color = 0;

    switch (s->format->BytesPerPixel) {
	case 1: { /* Assuming 8-bpp */
		Uint8 *bufp;
		bufp = (Uint8*) s->pixels + y * s->pitch + x;
		color = *bufp;
	}
	break;
	case 2: { /* Probably 15-bpp or 16-bpp */
		Uint16 *bufp;
		bufp = (Uint16 *)s->pixels + y*s->pitch/2 + x;
		color = *bufp;
	}
	break;
	case 3: { /* Slow 24-bpp mode, usually not used */
		Uint8 *bufp;
		bufp = (Uint8 *)s->pixels + y*s->pitch + x * 3;
		if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
			color = bufp[0];
			color |= bufp[1] <<  8;
			color |= bufp[2] << 16;
		} else {
			color = bufp[2];
			color |= bufp[1] <<  8;
			color |= bufp[0] << 16;
		}
	}
	break;
	case 4: { /* Probably 32-bpp */
		Uint32 *bufp;
		bufp = (Uint32 *)s->pixels + y*s->pitch/4 + x;
		color = *bufp;
	}
	break;
    }

	// Extract the components.
	SDL_GetRGBA(color, s->format, R, G, B, A);
}


struct heightfield {
	int	size;
	int	log_size;
	float	sample_spacing;
	float*	data;

	heightfield() {
		size = 0;
		log_size = 0;
		sample_spacing = 1.0f;
		data = 0;
	}

	~heightfield() {
		clear();
	}

	void	clear()
	// Frees any allocated data and resets our members.
	{
		if (data) {
			delete [] data;
			data = 0;
		}
		size = 0;
		log_size = 0;
	}

	float&	elem(int x, int z)
	// Return a reference to the element at (x, z).
	{
		assert_else(data && x >= 0 && x < size && z >= 0 && z < size) {
			error("heightfield::elem() -- array access out of bounds.");
		}

		return data[x + z * size];
	}

	float	get_elem(int x, int z) const
	{
		return (const_cast<heightfield*>(this))->elem(x, z);
	}

	int	load_bt(SDL_RWops* in)
	// Load .BT format heightfield data from the given file and
	// initialize heightfield.
	//
	// Return -1, and rewind the source file, if the data is not in .BT format.
	{
		clear();

		int	start = SDL_RWtell(in);

		// Read .BT header.

		// File-type marker.
		char	buf[11];
		SDL_RWread(in, buf, 1, 10);
		buf[10] = 0;
		if (strcmp(buf, "binterr1.1") != 0) {
			// Bad input file format.  Must not be BT 1.1.
			
			// Rewind to where we started.
			SDL_RWseek(in, start, SEEK_SET);

			return -1;
		}

		int	width = SDL_ReadLE32(in);
		int	height = SDL_ReadLE32(in);
		int	sample_size = SDL_ReadLE16(in);
		bool	float_flag = SDL_ReadLE16(in) == 1 ? true : false;
		bool	utm_flag = SDL_ReadLE16(in) ? true : false;
		int	utm_zone = SDL_ReadLE16(in);
		int	datum = SDL_ReadLE16(in);
		double	left = ReadDouble64(in);
		double	right = ReadDouble64(in);
		double	bottom = ReadDouble64(in);
		double	top = ReadDouble64(in);

		// Skip to the start of the data.
		SDL_RWseek(in, start + 256, SEEK_SET);

		//xxxxxx
		printf("width = %d, height = %d, sample_size = %d, left = %lf, right = %lf, bottom = %lf, top = %lf\n",
		       width, height, sample_size, left, right, bottom, top);
		//xxxxxx

		// If float_flag is set, make sure datasize is 4 bytes.
		if (float_flag == true && sample_size != 4) {
			throw "heightfield::load() -- for float data, sample_size must be 4 bytes!";
		}

		if (sample_size != 2 && sample_size != 4) {
			throw "heightfield::load() -- sample_size must be 2 or 4 bytes!";
		}

		// Set and validate the size.
		size = width;
		if (height != size) {
			// Data must be square!
			throw "heightfield::load() -- input dataset is not square!";
		}

		// Compute the log_size and make sure the size is 2^N + 1
		log_size = (int) (log2(size - 1) + 0.5);
		if (size != (1 << log_size) + 1) {
			throw "heightfield::load() -- input dataset size is not (2^N + 1) x (2^N + 1)!";
		}

		sample_spacing = fabs(right - left) / (size - 1);
		printf("sample_spacing = %f\n", sample_spacing);//xxxxxxx

		// Allocate the storage.
		int	sample_count = size * size;
		data = new float[sample_count];

		// Load the data.  BT data is goes in columns, bottom-to-top, left-to-right.
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				float	y;
				if (float_flag) {
					y = ReadFloat32(in);
				} else if (sample_size == 2) {
					y = SDL_ReadLE16(in);
				} else if (sample_size == 4) {
					y = SDL_ReadLE32(in);
				}
				data[i + (size - 1 - j) * size] = y;
			}
		}

		return 0;
	}


	void	load_bitmap(SDL_RWops* in /* float vertical_scale */)
	// Load a bitmap from the given file, and use it to initialize our
	// heightfield data.
	{
		SDL_Surface* s = IMG_Load_RW(in, 0);
		if (s == NULL) {
			error("heightfield::load_bitmap() -- could not load bitmap file.");
		}
		
		// Compute the dimension (width & height) for the heightfield.
		size = imax(s->w, s->h);
		log_size = (int) (log2(size - 1) + 0.5f);

		// Expand the heightfield dimension to contain the bitmap.
		while (((1 << log_size) + 1) < size) {
			log_size++;
		}
		size = (1 << log_size) + 1;

		sample_spacing = 1.0;	// TODO: parameterize this

		// Allocate storage.
		int	sample_count = size * size;
		data = new float[sample_count];

		// Initialize the data.
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				float	y = 0;

				// Extract a height value from the pixel data.
				Uint8	r, g, b, a;
				ReadPixel(s, imin(i, s->w - 1), imin(j, s->h - 1), &r, &g, &b, &a);
				y = r * 1.0f;	// just using red component for now.

				data[i + (size - 1 - j) * size] = y;
			}
		}

		SDL_FreeSurface(s);
	}
};


void	compute_lightmap(SDL_Surface* out, const heightfield& hf);


void	heightfield_shader(SDL_RWops* in, SDL_RWops* out)
// Generate texture for heightfield.
{
	heightfield	hf;

	// Load the heightfield data.
	if (hf.load_bt(in) < 0) {
		hf.load_bitmap(in);
	}

	// Compute and write the lightmap, if any.
	printf("Computing lightmap...");

	SDL_Surface*	lightmap = SDL_CreateRGBSurface(SDL_SWSURFACE, hf.size - 1, hf.size - 1, 24, 0xFF0000, 0xFF00, 0xFF, 0);
	compute_lightmap(lightmap, hf);
	
	SDL_SaveBMP_RW(lightmap, out, 0);
	SDL_FreeSurface(lightmap);
	
	printf("done\n");
}


//
// Quickie lightmap generation
//


void	compute_lightmap(SDL_Surface* out, const heightfield& hf)
// Computes a lightmap into the given surface.
{
	assert(out->w == hf.size - 1);
	assert(out->h == hf.size - 1);

	Uint8*	p = (Uint8*) out->pixels;
	for (int j = 0; j < out->h; j++) {
		for (int i = 0; i < out->w; i++) {
			float	y00 = hf.get_elem(i, j);
			float	y10 = hf.get_elem(i+1, j);
			float	y01 = hf.get_elem(i, j+1);
			float	y11 = hf.get_elem(i+1, j+1);
				
			float	slopex = (((y00 - y10) + (y01 - y11)) * 0.5) / hf.sample_spacing;
			float	slopez = (((y00 - y01) + (y10 - y11)) * 0.5) / hf.sample_spacing;

			Uint8	light = iclamp((slopex * 0.25 + slopez * 0.15 + 0.5) * 255, 0, 255);	// xxx arbitrary params

			*p++ = light;
			*p++ = light;
			*p++ = light;
		}
	}
}
