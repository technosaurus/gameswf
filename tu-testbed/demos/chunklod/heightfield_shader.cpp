// heightfield_shader.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Program to take heightfield input and generate texture for it.
//
// This is very hacky; I just need something quick that makes vaguely
// interesting textures for demoing the chunk renderer.


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <engine/utility.h>
#include <engine/container.h>
#include <engine/geometry.h>


struct texture_tile {
	Uint8	r, g, b;	// identifier
	SDL_Surface*	image;
};


void	initialize_tileset(array<texture_tile>* tileset);
const texture_tile*	choose_tile(const array<texture_tile>& tileset, int r, int g, int b);
void	heightfield_shader(SDL_RWops* rwin, SDL_RWops* out, SDL_Surface* tilemap, const array<texture_tile>& tileset, float resolution);


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
		   "usage: heightfield_shader <input_filename> <output_filename> [-t <tilemap_bitmap>]\n"
		   "\t[-r <texels per heixel>]\n"
		   "\n"
		   "\tThe input filename should either be a .BT format terrain file with\n"
		   "\t(2^N+1) x (2^N+1) datapoints, or a grayscale bitmap.\n"
		);
}


int	wrapped_main(int argc, char* argv[])
// Reads the given .BT terrain file or grayscale bitmap, and generates
// a texture map to use when rendering the heightfield.
{
	// Process command-line options.
	char*	infile = NULL;
	char*	outfile = NULL;
	char*	lightmap_filename = NULL;
	char*	tilemap = NULL;
	array<texture_tile>	tileset;
	float	resolution = 1.0;

	for ( int arg = 1; arg < argc; arg++ ) {
		if ( argv[arg][0] == '-' ) {
			// command-line switch.
			
			switch ( argv[arg][1] ) {
			case 'h':
			case '?':
				print_usage();
				exit( 1 );
				break;

			case 't':
				// Get tilemap filename.
				if (arg + 1 >= argc) {
					printf("error: -t option requires a filename\n");
					print_usage();
					exit(1);
				}
				arg++;
				tilemap = argv[arg];
				break;
			case 'r':
				// Get output resolution.
				if (arg + 1 >= argc) {
					printf("error: -r option requires a resolution (floating point value for texels/heixel)\n");
					print_usage();
					exit(1);
				}
				arg++;
				resolution = atoi(argv[arg]);
				if (resolution <= 0.001) {
					printf("error: resolution must be greater than 0\n");
					print_usage();
					exit(1);
				}
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

	// Initialize the tilemap and tileset.
	SDL_Surface*	tilemap_surface = NULL;
	if (tilemap && tilemap[0]) {
		// The tilemap is a bitmap, where the color of each pixel
		// chooses a tile from the tilemap.
		tilemap_surface = IMG_Load(tilemap);
	}
		
	// Tileset is a set of textures, associated with a particular
	// color in the tilemap.
	initialize_tileset(&tileset);

	// Print the parameters.
	printf("infile: %s\n", infile);
	printf("outfile: %s\n", outfile);

	// Process the data.
	heightfield_shader(in, out, tilemap_surface, tileset, resolution);

	SDL_RWclose(in);
	SDL_RWclose(out);

	return 0;
}


#undef main	// @@ some crazy SDL/WIN32 thing that I don't understand.
int	main(int argc, char* argv[])
{
	try {
		return wrapped_main(argc, argv);
	}
	catch (const char* message) {
		printf("exception: %s\n", message);
	}
	catch (...) {
		printf("unknown exception\n");
	}
	return -1;
}


void	ReadPixel(SDL_Surface *s, int x, int y, Uint8* R, Uint8* G, Uint8* B, Uint8* A)
// Utility function to read a pixel from an SDL surface.
// TODO: Should go in the engine utilities somewhere.
{
	assert(x >= 0 && x < s->w);
	assert(y >= 0 && y < s->h);

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
			printf("%d %d\n", x, z);//xxxxxx
			error("heightfield::elem() -- array access out of bounds.");
		}

		return data[x + z * size];
	}

	float	get_elem(int x, int z) const
	{
		return (const_cast<heightfield*>(this))->elem(x, z);
	}


	Uint8	compute_light(float x, float z) const
	// Compute a lighting value at a location in heightfield-space.
	// Result is 0-255.  Input is in heightfield index coordinates,
	// but is interpolated between the nearest samples.  Uses an
	// arbitrary hard-coded sun direction & lighting parameters.
	{
		x = fclamp(x, 0, size-3);
		z = fclamp(z, 0, size-3);

		int	i = (int) floor(x);
		int	j = (int) floor(z);

		float	wx = x - i;
		float	wz = z - j;

		// Compute lighting at nearest grid points, and bilinear blend
		// the results.
		Uint8	s00 = compute_light_int(i, j);
		Uint8	s10 = compute_light_int(i+1, j);
		Uint8	s01 = compute_light_int(i, j+1);
		Uint8	s11 = compute_light_int(i+1, j+1);
		
		return (Uint8) (s00 * (1 - wx) * (1 - wz)
				+ s10 * (wx) * (1 - wz)
				+ s01 * (1 - wx) * (wz)
				+ s11 * (wx) * (wz));
	}


	Uint8	compute_light_int(int i, int j) const
	// Aux function for compute_light().  Computes lighting at a
	// discrete heightfield spot.
	{

		float	y00 = get_elem(i, j);
		float	y10 = get_elem(i+1, j);
		float	y01 = get_elem(i, j+1);
		float	y11 = get_elem(i+1, j+1);
				
		float	slopex = (((y00 - y10) + (y01 - y11)) * 0.5) / sample_spacing;
		float	slopez = (((y00 - y01) + (y10 - y11)) * 0.5) / sample_spacing;

		return iclamp((int) ((slopex * 0.25 + slopez * 0.15 + 0.5) * 255), 0, 255);	// xxx arbitrary params
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
			printf("Warning: non-square data; will extend to make it square\n");
		}

		size = fmax(width, height);

		// Compute the log_size and make sure the size is 2^N + 1
		log_size = (int) (log2(size - 1) + 0.5);
		if (size != (1 << log_size) + 1) {
			if (size < 0 || size > (1 << 20)) {
				throw "invalid heightfield dimensions";
			}

			printf("Warning: data is not (2^N + 1) x (2^N + 1); will extend to make it the correct size.\n");

			// Expand log_size until it contains size.
			while (size > (1 << log_size) + 1) {
				log_size++;
			}
			size = (1 << log_size) + 1;
		}

		sample_spacing = fabs(right - left) / (size - 1);
		printf("sample_spacing = %f\n", sample_spacing);//xxxxxxx

		// Allocate the storage.
		int	sample_count = size * size;
		data = new float[sample_count];

		// Load the data.  BT data is goes in columns, bottom-to-top, left-to-right.
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
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

		// Extend data to fill out any unused space.
		{for (int i = 0; i < width; i++) {
			for (int j = height; j < size; j++) {
				data[i + (size - 1 - j) * size] = data[i + (size - 1 - (height-1)) * size];
			}
		}}
		{for (int i = width; i < size; i++) {
			for (int j = 0; j < size; j++) {
				data[i + (size - 1 - j) * size] = data[width - 1 + (size - 1 - j) * size];
			}
		}}

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


void	heightfield_shader(SDL_RWops* in, SDL_RWops* out, SDL_Surface* tilemap, const array<texture_tile>& tileset, float resolution)
// Generate texture for heightfield.
{
	heightfield	hf;

	// Load the heightfield data.
	if (hf.load_bt(in) < 0) {
		hf.load_bitmap(in);
	}

	// Compute and write the lightmap, if any.
	printf("Shading... ");

	const char*	spinner = "-\\|/";

	int	width = (int) ((hf.size - 1) * resolution);
	int	height = (int) ((hf.size - 1) * resolution);
	
	SDL_Surface*	texture = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 24, 0xFF0000, 0xFF00, 0xFF, 0);
	Uint8*	p = (Uint8*) texture->pixels;
	{for (int j = 0; j < height; j++) {
		{for (int i = 0; i < width; i++) {
			const texture_tile*	t;
			if (tilemap) {
				// Get the corresponding pixel from the tilemap, and
				// get the tile that it corresponds to.
				Uint8	r, g, b, a;
				ReadPixel(tilemap, imin((int) (i / resolution), tilemap->w-1), imin((int) (j / resolution), tilemap->h-1), &r, &g, &b, &a);
				t = choose_tile(tileset, r, g, b);
			} else {
				// In the absense of a tilemap, use the first tile for
				// everything.
				t = &tileset[0];
			}
			assert(t);

			// get tile pixel
			int	tx, ty;
			tx = (i % t->image->w);
			ty = (j % t->image->h);
			Uint8	r, g, b, a;
			ReadPixel(t->image, tx, ty, &r, &g, &b, &a);

			// apply light to it
			Uint8	light = hf.compute_light(i / resolution, j / resolution);

			r = (r * light) >> 8;
			g = (g * light) >> 8;
			b = (b * light) >> 8;

			*p++ = r;
			*p++ = g;
			*p++ = b;
		}}
		printf("\b%c", spinner[j&3]);
	}}
	
	SDL_SaveBMP_RW(texture, out, 0);
	SDL_FreeSurface(texture);
	
	printf("done\n");
}


void	initialize_tileset(array<texture_tile>* tileset)
// Adds a few standard tiles to the given tileset.
{
	texture_tile	t;

	t.r = 0xFF;
	t.g = 0x00;
	t.b = 0x00;
	t.image = IMG_Load("tileFF0000.jpg");
	assert_else(t.image) {
		error("can't load tileFF0000.jpg");
	}
	tileset->push_back(t);

	t.r = 0x00;
	t.g = 0xFF;
	t.b = 0x00;
	t.image = IMG_Load("tile00FF00.jpg");
	assert_else(t.image) {
		error("can't load tile00FF00.jpg");
	}
	tileset->push_back(t);

	t.r = 0x00;
	t.g = 0x00;
	t.b = 0xFF;
	t.image = IMG_Load("tile0000FF.jpg");
	assert_else(t.image) {
		error("can't load tile0000FF.jpg");
	}
	tileset->push_back(t);

	t.r = 0xFF;
	t.g = 0xFF;
	t.b = 0xFF;
	t.image = IMG_Load("tileFFFFFF.jpg");
	assert_else(t.image) {
		error("can't load tileFFFFFF.jpg");
	}
	tileset->push_back(t);
}


const texture_tile*	choose_tile(const array<texture_tile>& tileset, int r, int g, int b)
// Returns a reference to the tile within the given tileset whose
// r,g,b tag is closest to the specified r,g,b triple.
{
	const texture_tile*	result = &tileset[0];
	int	min_distance = 255 * 255 * 3;
	
	// Linear search for closest color in RGB space.
	for (int i = 0; i < tileset.size(); i++) {
		int	dr = (r - tileset[i].r);
		int	dg = (g - tileset[i].g);
		int	db = (b - tileset[i].b);

		int	dist = dr * dr + dg * dg + db * db;
		if (dist < min_distance) {
			min_distance = dist;
			result = &tileset[i];
		}
	}

	return result;
}

