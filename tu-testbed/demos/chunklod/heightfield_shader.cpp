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
extern "C" {
#include <jpeglib.h>
}

#include <engine/utility.h>
#include <engine/container.h>
#include <engine/geometry.h>

#include "bt_array.h"


struct texture_tile {
	Uint8	r, g, b;	// identifier
	SDL_Surface*	image;
};


void	initialize_tileset(array<texture_tile>* tileset);
const texture_tile*	choose_tile(const array<texture_tile>& tileset, int r, int g, int b);
void	heightfield_shader(const char* infile,
			   FILE* out,
			   SDL_Surface* tilemap,
			   const array<texture_tile>& tileset,
			   SDL_Surface* altitude_gradient,
			   float resolution,
			   float input_vertical_scale
			   );


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
	printf("heightfield_shader: program for generating a .jpg texture for a heightfield\n"
	       "terrain.\n\n"
	       "This program has been donated to the Public Domain by Thatcher Ulrich http://tulrich.com\n"
	       "Incorporates software from the Independent JPEG Group\n\n"
	       "usage: heightfield_shader <input_filename> <output_filename> [-t <tilemap_bitmap>]\n"
	       "\t[-g <altitude_gradient_bitmap>] [-r <texels per heixel>] [-v <bt_input_vertical_scale>]\n"
	       "\n"
	       "\tThe input file should be a .BT format terrain file with (2^N+1) x (2^N+1) datapoints.\n"
		);
}


int	wrapped_main(int argc, char* argv[])
// Reads the given .BT terrain file or grayscale bitmap, and generates
// a texture map to use when rendering the heightfield.
{
	// Process command-line options.
	char*	infile = NULL;
	char*	outfile = NULL;
	char*	tilemap = NULL;
	char*	gradient_file = NULL;
	array<texture_tile>	tileset;
	float	resolution = 1.0;
	float	input_vertical_scale = 1.0f;

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
			case 'g':
				// Get gradient filename.
				if (arg + 1 >= argc) {
					printf("error: -g option requires a filename\n");
					print_usage();
					exit(1);
				}
				arg++;
				gradient_file = argv[arg];
				break;
			case 'r':
				// Get output resolution.
				if (arg + 1 >= argc) {
					printf("error: -r option requires a resolution (floating point value for texels/heixel)\n");
					print_usage();
					exit(1);
				}
				arg++;
				resolution = (float) atof(argv[arg]);
				if (resolution <= 0.001) {
					printf("error: resolution must be greater than 0\n");
					print_usage();
					exit(1);
				}
				break;
				
			case 'v':
				// Set the input vertical scale.
				arg++;
				if (arg < argc) {
					input_vertical_scale = (float) atof(argv[arg]);
				}
				else {
					printf("error: -v option must be followed by a value for the input vertical scale\n");
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
				printf( "argument '%s' looks like extra noise; exiting.\n", argv[arg]);
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
	
	FILE*	out = fopen(outfile, "wb");
	if (out == 0) {
		printf( "error: can't open %s for output.\n", outfile );
		exit(1);
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

	// Altitude gradient bitmap.
	SDL_Surface*	altitude_gradient = NULL;
	if (gradient_file && gradient_file[0]) {
		altitude_gradient = IMG_Load(gradient_file);
	}

	// Print the parameters.
	printf("infile: %s\n", infile);
	printf("outfile: %s\n", outfile);

	// Process the data.
	heightfield_shader(infile, out, tilemap_surface, tileset, altitude_gradient, resolution, input_vertical_scale);

	fclose(out);

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
	int	m_size;
	int	m_log_size;
	float	m_sample_spacing;
	float	m_input_vertical_scale;

	bt_array*	m_bt;

	heightfield() {
		m_size = 0;
		m_log_size = 0;
		m_sample_spacing = 1.0f;
		m_input_vertical_scale = 1.0f;
		m_bt = NULL;
	}

	~heightfield() {
		clear();
	}

	void	clear()
	// Frees any allocated data and resets our members.
	{
		if (m_bt) {
			delete m_bt;
			m_bt = NULL;
		}
		m_size = 0;
		m_log_size = 0;
	}


	float	height(int x, int z) const
	// Return the height at (x, z).
	{
		assert(m_bt);
		return m_bt->get_sample(x, z) * m_input_vertical_scale;
	}


	float	height_interp(float x, float z) const
	// Return the height at any point within the grid.  Bilinearly
	// interpolates between samples.
	{
		x = fclamp(x, 0.f, float(m_size - 3));
		z = fclamp(z, 0.f, float(m_size - 3));

		int	i = (int) floor(x);
		int	j = (int) floor(z);

		float	wx = x - i;
		float	wz = z - j;

		// Compute lighting at nearest grid points, and bilinear blend
		// the results.
		float	y00 = height(i, j);
		float	y10 = height(i+1, j);
		float	y01 = height(i, j+1);
		float	y11 = height(i+1, j+1);
		
		return (y00 * (1 - wx) * (1 - wz)
			+ y10 * (wx) * (1 - wz)
			+ y01 * (1 - wx) * (wz)
			+ y11 * (wx) * (wz));
	}


	Uint8	compute_light(float x, float z) const
	// Compute a lighting value at a location in heightfield-space.
	// Result is 0-255.  Input is in heightfield index coordinates,
	// but is interpolated between the nearest samples.  Uses an
	// arbitrary hard-coded sun direction & lighting parameters.
	{
		x = fclamp(x, 0.f, float(m_size - 3));
		z = fclamp(z, 0.f, float(m_size - 3));

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
		float	y00 = height(i, j);
		float	y10 = height(i+1, j);
		float	y01 = height(i, j+1);
		float	y11 = height(i+1, j+1);
				
		float	slopex = (((y00 - y10) + (y01 - y11)) * 0.5f) / m_sample_spacing;
		float	slopez = (((y00 - y01) + (y10 - y11)) * 0.5f) / m_sample_spacing;

		return iclamp((int) ((slopex * 0.25 + slopez * 0.15 + 0.5) * 255), 0, 255);	// xxx arbitrary params
	}


	bool	init_bt(const char* bt_filename)
	// Use the specified .BT format heightfield data file as our height input.
	//
	// Return true on success, false on failure.
	{
		clear();

		m_bt = bt_array::create(bt_filename);
		if (m_bt == NULL) {
			// failure.
			return false;
		}

		m_size = imax(m_bt->get_width(), m_bt->get_height());

		// Compute the log_size and make sure the size is 2^N + 1
		m_log_size = (int) (log2((float) m_size - 1) + 0.5);
		if (m_size != (1 << m_log_size) + 1) {
			if (m_size < 0 || m_size > (1 << 20)) {
				printf("invalid heightfield dimensions -- size from file = %d\n", m_size);
				return false;
			}

			printf("Warning: data is not (2^N + 1) x (2^N + 1); will extend to make it the correct size.\n");

			// Expand log_size until it contains size.
			while (m_size > (1 << m_log_size) + 1) {
				m_log_size++;
			}
			m_size = (1 << m_log_size) + 1;
		}

		m_sample_spacing = (float) (fabs(m_bt->get_right() - m_bt->get_left()) / (double) (m_size - 1));
		printf("sample_spacing = %f\n", m_sample_spacing);//xxxxxxx

		return true;
	}
};


void	compute_lightmap(SDL_Surface* out, const heightfield& hf);


void	heightfield_shader(const char* infile,
			   FILE* out,
			   SDL_Surface* tilemap,
			   const array<texture_tile>& tileset,
			   SDL_Surface* altitude_gradient,
			   float resolution,
			   float input_vertical_scale
			   )
// Generate texture for heightfield.
{
	heightfield	hf;

	// Load the heightfield data.
	if (hf.init_bt(infile) == false) {
		printf("Can't load %s as a .BT file\n", infile);
		exit(1);
	}
	hf.m_input_vertical_scale = input_vertical_scale;

	// Compute and write the lightmap, if any.
	printf("Shading... ");

	const char*	spinner = "-\\|/";

	int	width = (int) ((1 << hf.m_log_size) * resolution);
	int	height = (int) ((1 << hf.m_log_size) * resolution);
	
	// create a buffer to hold a 1-pixel high RGB buffer.  We're
	// going to create our texture one scanline at a time.
	Uint8*	texture_pixels = new Uint8[width * 3];

	// Create our JPEG compression object, and initialize compression settings.
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, out);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 80 /* 0..100 */, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	{for (int j = 0; j < height; j++) {

		Uint8*	p = texture_pixels;

		{for (int i = 0; i < width; i++) {

			// Pick a color for this texel.
			// TODO: break this out into a more sophisticated function.
			Uint8	r, g, b, a;
			if (altitude_gradient) {
				// Shade the terrain using an altitude gradient.
				float	y = hf.height_interp(i / resolution, j / resolution);
				int	h = altitude_gradient->h;
				if (y < 0.1f)
				{
					// Sea level.
					ReadPixel(altitude_gradient, 0, h - 1, &r, &g, &b, &a);
				}
				else {
					// Above sea level.
					int	hpix = (int) floor(((y - 0.1f) / 4500.f) * h);
					if (hpix >= h) {
						hpix = h - 1;
					}
					if (hpix < 1) {
						hpix = 1;
					}
					ReadPixel(altitude_gradient, 0, (h - 1) - hpix, &r, &g, &b, &a);	// TODO: smooth interpolation, or jittered sampling, or something.
				}
			}
			else {
				// Select a tile.
				const texture_tile*	t;
				if (tilemap) {
				// Get the corresponding pixel from the tilemap, and
				// get the tile that it corresponds to.
					Uint8	r, g, b, a;
					ReadPixel(tilemap, imin((int) (i / resolution), tilemap->w-1), imin((int) (j / resolution), tilemap->h-1), &r, &g, &b, &a);
					t = choose_tile(tileset, r, g, b);
				} else {
				// In the absence of a tilemap, use the first tile for
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
			}

			// apply light to the color
			Uint8	light = hf.compute_light(i / resolution, j / resolution);

			r = (r * light) >> 8;
			g = (g * light) >> 8;
			b = (b * light) >> 8;

			*p++ = r;
			*p++ = g;
			*p++ = b;
		}}

		// Write out the scanline.
		JSAMPROW	row_pointer[1];
		row_pointer[0] = texture_pixels;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);

		printf("\b%c", spinner[j&3]);
	}}
	
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

//	SDL_SaveBMP_RW(texture, out, 0);
	delete texture_pixels;
	
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

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
