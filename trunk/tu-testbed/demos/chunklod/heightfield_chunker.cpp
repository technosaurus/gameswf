// heightfield_chunker.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Program to take heightfield input and generate a chunked-lod
// structure as output.  Uses quadtree decomposition, with a
// Lindstrom-Koller-esque decimation algorithm used to decimate
// the individual chunks.


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <engine/utility.h>
#include <engine/container.h>
#include <engine/geometry.h>


void	heightfield_chunker(SDL_RWops* rwin, SDL_RWops* out, int tree_depth, float base_max_error, float spacing, float vertical_scale);


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
	printf("heightfield_chunker: program for processing terrain data and generating\n"
		   "a chunked LOD data file suitable for viewing by 'chunklod'.\n\n"
		   "This program has been donated to the Public Domain by Thatcher Ulrich <tu@tulrich.com>\n\n"
		   "usage: heightfield_chunker [-d depth] [-e error] [-s hspacing] <input_filename> <output_filename>\n"
		   "\n"
		   "\tThe input filename should either be a .BT format terrain file with\n"
		   "\t(2^N+1) x (2^N+1) datapoints, or a grayscale bitmap.\n"
		   "\n"
		   "\t'depth' gives the depth of the quadtree of chunks to generate; default = 6\n"
		   "\t'error' gives the maximum geometric error to allow at full LOD; default = 0.5\n"
		   "\t'hspacing' determines the horizontal spacing between grid points, ONLY if the\n"
		   "\t\tinput file is a bitmap (.bt files contain spacing info).  default = 4\n"
		);
}


// struct to hold statistics.
struct stats {
	int	input_vertices;
	int	output_vertices;
	int	output_real_triangles;
	int	output_degenerate_triangles;
	int	output_chunks;
	int	output_size;

	stats() {
		input_vertices = 0;
		output_vertices = 0;
		output_real_triangles = 0;
		output_degenerate_triangles = 0;
		output_chunks = 0;
		output_size = 0;
	}
} stats;


// A default value, to determine quantization of vertical units.
// TODO: set this value based on info from the .BT file header, or
// scale it to the largest height in the data.
const float MAX_HEIGHT = 32767.0;


// @@ under Win32, SDL likes to define its own main(), to fix up arg
// handling.  I still need to provide the right linker options to get
// SDL's arg handler linked in; in the meantime, use normal Windows
// arg handling.
#undef main

int	main(int argc, char* argv[])
// Reads the given .BT terrain file or grayscale bitmap, and generates
// a quadtree-chunked LOD data file, suitable for viewing by the
// 'chunklod' program.
{
	// Default processing parameters.
	int	tree_depth = 6;
	float	max_geometric_error = 1.0f;
	float	spacing = 4.0f;
	float	vertical_scale = MAX_HEIGHT / 32767.0;

	// Process command-line options.
	char*	infile = NULL;
	char*	outfile = NULL;

	for (int arg = 1; arg < argc; arg++) {
		if (argv[arg][0] == '-') {
			// command-line switch.
			
			switch (argv[arg][1]) {
			case 'h':
			case '?':
				print_usage();
				exit(1);
				break;

			case 'd':
				// Set the tree depth.
				arg++;
				if (arg < argc) {
					tree_depth = atoi(argv[ arg ]);

				} else {
					printf("error: -d option must be followed by an integer for the tree depth\n");
					print_usage();
					exit(1);
				}
				break;

			case 'e':
				// Set the max geometric error.
				arg++;
				if (arg < argc) {
					max_geometric_error = atof(argv[ arg ]);

				} else {
					printf("error: -e option must be followed by a value for the maximum geometric error\n");
					print_usage();
					exit(1);
				}
				break;

			case 's':
				// Set the horizontal spacing.
				arg++;
				if (arg < argc) {
					spacing = atof(argv[ arg ]);

				} else {
					printf("error: -s option must be followed by a value for the horizontal grid spacing\n");
					print_usage();
					exit(1);
				}
				break;
			}

		} else {
			// File argument.
			if (infile == NULL) {
				infile = argv[arg];
			} else if (outfile == NULL) {
				outfile = argv[arg];
			} else {
				// This looks like extra noise on the command line; complain and exit.
				printf("argument '%s' looks like extra noise; exiting.\n");
				print_usage();
				exit(1);
			}
		}
	}

	// Make sure we have input and output filenames.
	if (infile == NULL || outfile == NULL) {
		// No input or output -- can't run.
		printf("error: you must specify input and output filenames.\n");
		print_usage();
		exit(1);
	}
	
	SDL_RWops*	in = SDL_RWFromFile(infile, "rb");
	if (in == 0) {
		printf("error: can't open %s for input.\n", infile);
		exit(1);
	}

	SDL_RWops*	out = SDL_RWFromFile(outfile, "wb");
	if (out == 0) {
		printf("error: can't open %s for output.\n", outfile);
		exit(1);
	}

	// Print the parameters.
	printf("infile: %s\n", infile);
	printf("outfile: %s\n", outfile);
	printf("depth = %d, max error = %f\n", tree_depth, max_geometric_error);
	printf("vertical scale = %f\n", vertical_scale);

	// Process the data.
	heightfield_chunker(in, out, tree_depth, max_geometric_error, spacing, vertical_scale);

	stats.output_size = SDL_RWtell(out);

	SDL_RWclose(in);
	SDL_RWclose(out);

	float	verts_per_chunk = stats.output_vertices / (float) stats.output_chunks;

	// Print some stats.
	printf("========================================\n");
	printf("                chunks: %10d\n", stats.output_chunks);
	printf("           input verts: %10d\n", stats.input_vertices);
	printf("          output verts: %10d\n", stats.output_vertices);

	printf("       avg verts/chunk: %10.0f\n", verts_per_chunk);
	if (verts_per_chunk < 1000) {
		printf("NOTE: verts/chunk is low; for higher poly throughput consider setting '-d %d' on the command line and reprocessing.\n",
			   tree_depth - 1);
	} else if (verts_per_chunk > 5000) {
		printf("NOTE: verts/chunk is high; for smoother framerate consider setting '-d %d' on the command line and reprocessing.\n",
			   tree_depth + 1);
	}

	printf("          output bytes: %10d\n", stats.output_size);
	printf("      bytes/input vert: %10.2f\n", stats.output_size / (float) stats.input_vertices);
	printf("     bytes/output vert: %10.2f\n", stats.output_size / (float) stats.output_vertices);

	printf("        real triangles: %10d\n", stats.output_real_triangles);
	printf("  degenerate triangles: %10d\n", stats.output_degenerate_triangles);
	printf("   degenerate overhead: %10.0f%%\n", stats.output_degenerate_triangles / (float) stats.output_real_triangles * 100);

	return 0;
}


static int	lowest_one(int x)
// Returns the bit position of the lowest 1 bit in the given value.
// If x == 0, returns the number of bits in an integer.
//
// E.g. lowest_one(1) == 0; lowest_one(16) == 4; lowest_one(5) == 0;
{
	int	intbits = sizeof(x) * 8;
	int	i;
	for (i = 0; i < intbits; i++, x = x >> 1) {
		if (x & 1) break;
	}
	return i;
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


struct heightfield_elem {
	float	y;
	float	error;
	Sint8	activation_level;

	heightfield_elem() { y = 0; clear(); }

	void	clear()
	{
		error = 0;
		activation_level = -1;
	}

	void	set_activation_level(int l) {
		assert(l >= 0 && l < 128);
		activation_level = l;
	}

	int	get_activation_level() const { return activation_level; }

	void	activate(int level)
	// Sets the activation_level to the given level, if it's greater than
	// the vert's current activation level.
	{
		if (level > activation_level) {
			if (activation_level == -1) {
				stats.output_vertices++;
			}
			set_activation_level(level);//activation_level = level;
		}
	}
};


struct heightfield {
	int	size;
	int	log_size;
	float	sample_spacing;
	heightfield_elem*	data;

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

	heightfield_elem&	elem(int x, int z)
	// Return a reference to the element at (x, z).
	{
		if (data == 0
		    || x < 0 || z < 0 || x >= size || z >= size)
		{
			throw "heightfield::elem() -- array access out of bounds";
		}

		return data[x + z * size];
	}

	const heightfield_elem&	get_elem(int x, int z) const
	{
		return (const_cast<heightfield*>(this))->elem(x, z);
	}

	int	node_index(int x, int z)
	// Given the coordinates of the center of a quadtree node, this
	// function returns its node index.  The node index is essentially
	// the node's rank in a breadth-first quadtree traversal.  Assumes
	// a [nw, ne, sw, se] traversal order.
	//
	// If the coordinates don't specify a valid node (e.g. if the coords
	// are outside the heightfield) then returns -1.
	{
		if (x < 0 || x >= size || z < 0 || z >= size) {
			return -1;
		}

		int	l1 = lowest_one(x | z);
		int	depth = log_size - l1 - 1;

		int	base = 0x55555555 & ((1 << depth*2) - 1);	// total node count in all levels above ours.
		int	shift = l1 + 1;

		// Effective coords within this node's level.
		int	col = x >> shift;
		int	row = z >> shift;

		return base + (row << depth) + col;
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
//			throw "heightfield::load() -- file format is not BT 1.1 (see vterrain.org for format details)";
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
		data = new heightfield_elem[sample_count];

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
				data[i + (size - 1 - j) * size].y = y;
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

		sample_spacing = -1;	// Overridden later...

		// Allocate storage.
		int	sample_count = size * size;
		data = new heightfield_elem[sample_count];

		// Initialize the data.
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				float	y = 0;

				// Extract a height value from the pixel data.
				Uint8	r, g, b, a;
				ReadPixel(s, imin(i, s->w - 1), imin(j, s->h - 1), &r, &g, &b, &a);
				y = r * 1.0f;	// just using red component for now.

				data[i + (size - 1 - j) * size].y = y;
			}
		}

		SDL_FreeSurface(s);
	}
};


void	update(heightfield& hf, float base_max_error, int ax, int az, int rx, int rz, int lx, int lz);
void	propagate_activation_level(heightfield& hf, int cx, int cz, int level, int target_level);
int	check_propagation(heightfield& hf, int cx, int cz, int level);
void	generate_node_data(SDL_RWops* rw, heightfield& hf, int x0, int z0, int log_size, int level, float vertical_scale);


void	heightfield_chunker(SDL_RWops* in, SDL_RWops* out, int tree_depth, float base_max_error, float spacing, float vertical_scale)
// Generate LOD chunks from the given heightfield.
// 
// tree_depth determines the depth of the chunk quadtree.
// 
// base_max_error specifies the maximum allowed geometric vertex error,
// at the finest level of detail.
//
// Spacing determines the horizontal sample spacing for bitmap
// heightfields only.
{
	heightfield	hf;

	// Load the heightfield data.
	if (hf.load_bt(in) < 0) {
		hf.load_bitmap(in);
	}

	if (hf.sample_spacing == -1) {
		hf.sample_spacing = spacing;
	}

	// Initialize the mesh info -- clear the meshing 
	for (int j = 0; j < hf.size; j++) {
		for (int i = 0; i < hf.size; i++) {
			hf.elem(i, j).clear();
		}
	}

	stats.input_vertices = hf.size * hf.size;

	printf("updating...");

	// Run a view-independent L-K style BTT update on the heightfield, to generate
	// error and activation_level values for each element.
	update(hf, base_max_error, 0, hf.size-1, hf.size-1, hf.size-1, 0, 0);	// sw half of the square
	update(hf, base_max_error, hf.size-1, 0, 0, 0, hf.size-1, hf.size-1);	// ne half of the square

	printf("done.\n");

	printf("propagating...");

	// Propagate the activation_level values of verts to their
	// parent verts, quadtree LOD style.  Gives same result as
	// L-K.
	for (int i = 0; i < hf.log_size; i++) {
		propagate_activation_level(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1, i);
		propagate_activation_level(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1, i);
	}

//	check_propagation(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1);//xxxxx

	printf("done\n");

	// Write a .chu header for the output file.
	SDL_WriteLE32(out, ('C') | ('H' << 8) | ('U' << 16));	// four byte "CHU\0" tag
	SDL_WriteLE16(out, 4);	// file format version.
	SDL_WriteLE16(out, tree_depth);	// depth of the chunk quadtree.
	WriteFloat32(out, base_max_error);	// max geometric error at base level mesh.
	WriteFloat32(out, vertical_scale);	// meters / unit of vertical measurement.
	SDL_WriteLE32(out, 0x55555555 & ((1 << (tree_depth*2)) - 1));	// Chunk count.  Fully populated quadtree.

	printf("meshing...");

	// Write out the node data for the entire chunk tree.
	generate_node_data(out, hf, 0, 0, hf.log_size, tree_depth-1, vertical_scale);

	printf("done\n");
}


void	update(heightfield& hf, float base_max_error, int ax, int az, int rx, int rz, int lx, int lz)
// Given the triangle, computes an error value and activation level
// for its base vertex, and recurses to child triangles.
{
	// Compute the coordinates of this triangle's base vertex.
	int	dx = lx - rx;
	int	dz = lz - rz;
	if (iabs(dx) <= 1 && iabs(dz) <= 1) {
		// We've reached the base level.  There's no base
		// vertex to update, and no child triangles to
		// recurse to.
		return;
	}

	// base vert is midway between left and right verts.
	int	bx = rx + (dx >> 1);
	int	bz = rz + (dz >> 1);

	float	error = hf.elem(bx, bz).y - (hf.elem(lx, lz).y + hf.elem(rx, rz).y) / 2.f;
	hf.elem(bx, bz).error = error;	// Set this vert's error value.
	if (error >= base_max_error) {
		// Compute the mesh level above which this vertex
		// needs to be included in LOD meshes.
		int	activation_level = (int) floor(log2(fabs(error) / base_max_error) + 0.5);

		// Force the base vert to at least this activation level.
		hf.elem(bx, bz).activate(activation_level);
	}

	// Recurse to child triangles.
	update(hf, base_max_error, bx, bz, ax, az, rx, rz);	// base, apex, right
	update(hf, base_max_error, bx, bz, lx, lz, ax, az);	// base, left, apex
}


const float	SQRT_2 = sqrtf(2);


float	height_query(const heightfield& hf, int level, int x, int z, int ax, int az, int rx, int rz, int lx, int lz)
// Returns the height of the query point (x,z) within the triangle (a, r, l),
// as tesselated to the specified LOD.
{
	// If the query is on one of our verts, return that vert's height.
	if ((x == ax && z == az)
		 || (x == rx && z == rz)
		 || (x == lx && z == lz))
	{
		return hf.get_elem(x, z).y;
	}

	// Compute the coordinates of this triangle's base vertex.
	int	dx = lx - rx;
	int	dz = lz - rz;
	if (iabs(dx) <= 1 && iabs(dz) <= 1) {
		// We've reached the base level.  This is an error condition; we should
		// have gotten a successful test earlier.

		// assert(0);
		printf("Error: height_query hit base of heightfield.\n");

		return hf.get_elem(ax, az).y;
	}

	// base vert is midway between left and right verts.
	int	bx = rx + (dx >> 1);
	int	bz = rz + (dz >> 1);

	// compute the length of a side edge.
	float	edge_length_squared = (dx * dx + dz * dz) / 2.f;

	float	sr, sl;	// barycentric coords w/r/t the right and left edges.
	sr = ((x - ax) * (rx - ax) + (z - az) * (rz - az)) / edge_length_squared;
	sl = ((x - ax) * (lx - ax) + (z - az) * (lz - az)) / edge_length_squared;

	int	base_vert_level = hf.get_elem(bx, bz).get_activation_level();
	if (base_vert_level >= level){
		// The mesh is more tesselated at the desired LOD.  Recurse.
		if (sr >= sl) {
			// Query is in right child triangle.
			return height_query(hf, level, x, z, bx, bz, ax, az, rx, rz);	// base, apex, right
		} else {
			// Query is in left child triangle.
			return height_query(hf, level, x, z, bx, bz, lx, lz, ax, az);	// base, left, apex
		}
	}

	float	ay = hf.get_elem(ax, az).y;
	float	dr = hf.get_elem(rx, rz).y - ay;
	float	dl = hf.get_elem(lx, lz).y - ay;

	// This triangle is as far as the desired LOD goes.  Compute the
	// query's height on the triangle.
	return ay + sl * dl + sr * dr;
}


float	get_height_at_LOD(const heightfield& hf, int level, int x, int z)
// Returns the height of the mesh as simplified to the specified level
// of detail.
{
	if (z > x) {
		// Query in SW quadrant.
		return height_query(hf, level, x, z, 0, hf.size-1, hf.size-1, hf.size-1, 0, 0);	// sw half of the square

	} else {	// query in NW quadrant
		return height_query(hf, level, x, z, hf.size-1, 0, 0, 0, hf.size-1, hf.size-1);	// ne half of the square
	}
}


void	propagate_activation_level(heightfield& hf, int cx, int cz, int level, int target_level)
// Does a quadtree descent through the heightfield, in the square with
// center at (cx, cz) and size of (2 ^ (level + 1) + 1).  Descends
// until level == target_level, and then propagates this square's
// child center verts to the corresponding edge vert, and the edge
// verts to the center.  Essentially the quadtree meshing update
// dependency graph as in my Gamasutra article.  Must call this with
// successively increasing target_level to get correct propagation.
{
	int	half_size = 1 << level;
	int	quarter_size = half_size >> 1;

	if (level > target_level) {
		// Recurse to children.
		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 2; i++) {
				propagate_activation_level(hf,
							   cx - quarter_size + half_size * i,
							   cz - quarter_size + half_size * j,
							   level - 1, target_level);
			}
		}
		return;
	}

	// We're at the target level.  Do the propagation on this
	// square.

	// ee == east edge, en = north edge, etc
	heightfield_elem&	ee = hf.elem(cx + half_size, cz);
	heightfield_elem&	en = hf.elem(cx, cz - half_size);
	heightfield_elem&	ew = hf.elem(cx - half_size, cz);
	heightfield_elem&	es = hf.elem(cx, cz + half_size);
	
	if (level > 0) {
		// Propagate child verts to edge verts.
		int	elev = hf.elem(cx + quarter_size, cz - quarter_size).get_activation_level();	// ne
		ee.activate(elev);
		en.activate(elev);

		elev = hf.elem(cx - quarter_size, cz - quarter_size).get_activation_level();	// nw
		en.activate(elev);
		ew.activate(elev);

		elev = hf.elem(cx - quarter_size, cz + quarter_size).get_activation_level();	// sw
		ew.activate(elev);
		es.activate(elev);

		elev = hf.elem(cx + quarter_size, cz + quarter_size).get_activation_level();	// se
		es.activate(elev);
		ee.activate(elev);
	}

	// Propagate edge verts to center.
	heightfield_elem&	c = hf.elem(cx, cz);
	c.activate(ee.get_activation_level());
	c.activate(en.get_activation_level());
	c.activate(es.get_activation_level());
	c.activate(ew.get_activation_level());
}


int	check_propagation(heightfield& hf, int cx, int cz, int level)
// Debugging function -- verifies that activation level dependencies
// are correct throughout the tree.
{
	int	half_size = 1 << level;
	int	quarter_size = half_size >> 1;

	int	max_act = -1;

	// cne = ne child, cnw = nw child, etc.
	int	cne = -1;
	int	cnw = -1;
	int	csw = -1;
	int	cse = -1;
	if (level > 0) {
		// Recurse to children.
		cne = check_propagation(hf, cx + quarter_size, cz - quarter_size, level - 1);
		cnw = check_propagation(hf, cx - quarter_size, cz - quarter_size, level - 1);
		csw = check_propagation(hf, cx - quarter_size, cz + quarter_size, level - 1);
		cse = check_propagation(hf, cx + quarter_size, cz + quarter_size, level - 1);
	}

	// ee == east edge, en = north edge, etc
	int	ee = hf.elem(cx + half_size, cz).get_activation_level();
	int	en = hf.elem(cx, cz - half_size).get_activation_level();
	int	ew = hf.elem(cx - half_size, cz).get_activation_level();
	int	es = hf.elem(cx, cz + half_size).get_activation_level();
	
	if (level > 0) {
		// Propagate child verts to edge verts.
		if (cne > ee || cse > ee) {
			printf("cp error! ee! lev = %d, cx = %d, cz = %d, alev = %d\n", level, cx, cz, ee);	//xxxxx
		}

		if (cne > en || cnw > en) {
			printf("cp error! en! lev = %d, cx = %d, cz = %d, alev = %d\n", level, cx, cz, en);	//xxxxx
		}

		if (cnw > ew || csw > ew) {
			printf("cp error! ew! lev = %d, cx = %d, cz = %d, alev = %d\n", level, cx, cz, ew);	//xxxxx
		}
		
		if (csw > es || cse > es) {
			printf("cp error! es! lev = %d, cx = %d, cz = %d, alev = %d\n", level, cx, cz, es);	//xxxxx
		}
	}

	// Check level of edge verts against center.
	int	c = hf.elem(cx, cz).get_activation_level();
	max_act = imax(max_act, ee);
	max_act = imax(max_act, en);
	max_act = imax(max_act, es);
	max_act = imax(max_act, ew);

	if (max_act > c) {
		printf("cp error! center! lev = %d, cx = %d, cz = %d, alev = %d, max_act = %d ee = %d en = %d ew = %d es = %d err = %f\n", level, cx, cz, c, max_act, ee, en, ew, es, hf.elem(cx, cz).error);	//xxxxx
	}

	return imax(max_act, c);
}


namespace mesh {
	void	clear();
	void	emit_vertex(const heightfield& hf, int ax, int az);	// call this in strip order.
	int	lookup_index(int x, int z);
	void	write(SDL_RWops* rw, const heightfield& hf, int activation_level, float vertical_scale);

	void	add_edge_strip_index(int edge_dir, int index);
	void	add_edge_vertex_lo(int edge_dir, int x, int z);
	void	add_edge_vertex_hi(int edge_dir, int hi_index, int x, int z);
};


void	gen_mesh(heightfield& hf, int level, int ax, int az, int rx, int rz, int lx, int lz);

void	generate_edge_data(SDL_RWops* out, heightfield& hf, int dir, int x0, int z0, int x1, int z1, int level, bool generate_ribbon);


struct gen_state;
void	generate_block(heightfield& hf, int level, int log_size, int cx, int cz);
void	generate_quadrant(heightfield& hf, gen_state* s, int lx, int lz, int tx, int tz, int rx, int rz, int level);


static const char*	spinner = "-\\|/";
static int spin_count = 0;


void	generate_node_data(SDL_RWops* out, heightfield& hf, int x0, int z0, int log_size, int level, float vertical_scale)
// Given a square of data, with northwest corner at (x0, z0) and
// comprising ((1<<log_size)+1) verts along each axis, this function
// generates the mesh using verts which are active at the given level.
//
// If we're not at the base level (level > 0), then also recurses to
// quadtree child nodes and generates their data.
{
	stats.output_chunks++;

	int	size = (1 << log_size);
	int	half_size = size >> 1;
	int	cx = x0 + half_size;
	int	cz = z0 + half_size;

	// Assign a label to this chunk, so edges can reference the chunks.
	int	chunk_label = hf.node_index(cx, cz);
//	printf("chunk_label(%d,%d) = %d\n", cx, cz, chunk_label);//xxxx

	mesh::clear();

	// !!! This needs to be done in propagate, or something (too late now) !!!
	// Make sure our corner verts are activated on this level.
	hf.elem(x0 + size, z0).activate(level);
	hf.elem(x0, z0).activate(level);
	hf.elem(x0, z0 + size).activate(level);
	hf.elem(x0 + size, z0 + size).activate(level);

	// Generate the mesh.
	generate_block(hf, level, log_size, x0 + half_size, z0 + half_size);

	printf("\b%c", spinner[(spin_count++)&3]);

//	// Print some interesting info.
//	printf("chunk: (%d, %d) size = %d\n", x0, z0, size);

	// Write our label.
	SDL_WriteLE32(out, chunk_label);

	// Write the labels of our neighbors.
	SDL_WriteLE32(out, hf.node_index(cx + size, cz));	// EAST
	SDL_WriteLE32(out, hf.node_index(cx, cz - size));	// NORTH
	SDL_WriteLE32(out, hf.node_index(cx - size, cz));	// WEST
	SDL_WriteLE32(out, hf.node_index(cx, cz + size));	// SOUTH

	// Generate data for our edges.
	generate_edge_data(out, hf, 0, cx + half_size, cz - half_size, cx + half_size, cz + half_size, level, level > 0);	// east
	generate_edge_data(out, hf, 1, cx - half_size, cz - half_size, cx + half_size, cz - half_size, level, level > 0);	// north
	generate_edge_data(out, hf, 2, cx - half_size, cz - half_size, cx - half_size, cz + half_size, level, level > 0);	// west
	generate_edge_data(out, hf, 3, cx - half_size, cz + half_size, cx + half_size, cz + half_size, level, level > 0);	// south

	// write out the mesh data.
	mesh::write(out, hf, level, vertical_scale);

	// recurse to child regions, to generate child chunks.
	if (level > 0) {
		int	half_size = (1 << (log_size-1));
		generate_node_data(out, hf, x0, z0, log_size-1, level-1, vertical_scale);	// nw
		generate_node_data(out, hf, x0 + half_size, z0, log_size-1, level-1, vertical_scale);	// ne
		generate_node_data(out, hf, x0, z0 + half_size, log_size-1, level-1, vertical_scale);	// sw
		generate_node_data(out, hf, x0 + half_size, z0 + half_size, log_size-1, level-1, vertical_scale);	// se
	}
}


struct gen_state {
	int	my_buffer[2][2];	// x,z coords of the last two vertices emitted by the generate_ functions.
	int	activation_level;	// for determining whether a vertex is enabled in the block we're working on
	int	ptr;	// indexes my_buffer.
	int	previous_level;	// for keeping track of level changes during recursion.

	bool	in_my_buffer(int x, int z)
	// Returns true if the specified vertex is in my_buffer.
	{
		return ((x == my_buffer[0][0]) && (z == my_buffer[0][1]))
			|| ((x == my_buffer[1][0]) && (z == my_buffer[1][1]));
	}

	void	set_my_buffer(int x, int z)
	// Sets the current my_buffer entry to (x,z)
	{
		my_buffer[ptr][0] = x;
		my_buffer[ptr][1] = z;
	}
};


void	generate_block(heightfield& hf, int activation_level, int log_size, int cx, int cz)
// Generate the mesh for the specified square with the given center.
// This is paraphrased directly out of Lindstrom et al, SIGGRAPH '96.
// It generates a square mesh by walking counterclockwise around four
// triangular quadrants.  The resulting mesh is composed of a single
// continuous triangle strip, with a few corners turned via degenerate
// tris where necessary.
{
	// quadrant corner coordinates.
	int	hs = 1 << (log_size - 1);
	int	q[4][2] = {
		{ cx + hs, cz + hs },	// se
		{ cx + hs, cz - hs },	// ne
		{ cx - hs, cz - hs },	// nw
		{ cx - hs, cz + hs },	// sw
	};

	// Init state for generating mesh.
	gen_state	state;
	state.ptr = 0;
	state.previous_level = 0;
	state.activation_level = activation_level;
	for (int i = 0; i < 4; i++) {
		state.my_buffer[i>>1][i&1] = -1;
	}

	mesh::emit_vertex(hf, q[0][0], q[0][1]);
	state.set_my_buffer(q[0][0], q[0][1]);

	{for (int i = 0; i < 4; i++) {
		if ((state.previous_level & 1) == 0) {
			// tulrich: turn a corner?
			state.ptr ^= 1;
		} else {
			// tulrich: jump via degenerate?
			int	x = state.my_buffer[1 - state.ptr][0];
			int	z = state.my_buffer[1 - state.ptr][1];
			mesh::emit_vertex(hf, x, z);	// or, emit vertex(last - 1);
		}

		// Initial vertex of quadrant.
		mesh::emit_vertex(hf, q[i][0], q[i][1]);
		state.set_my_buffer(q[i][0], q[i][1]);
		state.previous_level = 2 * log_size + 1;

		generate_quadrant(hf,
				  &state,
				  q[i][0], q[i][1],	// q[i][l]
				  cx, cz,	// q[i][t]
				  q[(i+1)&3][0], q[(i+1)&3][1],	// q[i][r]
				  2 * log_size
			);
	}}
	if (state.in_my_buffer(q[0][0], q[0][1]) == false) {
		// finish off the strip.  @@ may not be necessary?
		mesh::emit_vertex(hf, q[0][0], q[0][1]);
	}
}


void	generate_quadrant(heightfield& hf, gen_state* s, int lx, int lz, int tx, int tz, int rx, int rz, int recursion_level)
// Auxiliary function for generate_block().  Generates a mesh from a
// triangular quadrant of a square heightfield block.  Paraphrased
// directly out of Lindstrom et al, SIGGRAPH '96.
{
	if (recursion_level <= 0) return;

	if (hf.elem(tx, tz).get_activation_level() >= s->activation_level) {
		// Find base vertex.
		int	bx = (lx + rx) >> 1;
		int	bz = (lz + rz) >> 1;

		generate_quadrant(hf, s, lx, lz, bx, bz, tx, tz, recursion_level - 1);	// left half of quadrant

		if (s->in_my_buffer(tx,tz) == false) {
			if ((recursion_level + s->previous_level) & 1) {
				s->ptr ^= 1;
			} else {
				int	x = s->my_buffer[1 - s->ptr][0];
				int	z = s->my_buffer[1 - s->ptr][1];
				mesh::emit_vertex(hf, x, z);	// or, emit vertex(last - 1);
			}
			mesh::emit_vertex(hf, tx, tz);
			s->set_my_buffer(tx, tz);
			s->previous_level = recursion_level;
		}

		generate_quadrant(hf, s, tx, tz, bx, bz, rx, rz, recursion_level - 1);
	}
}


void	generate_edge_data(SDL_RWops* out, heightfield& hf, int dir, int x0, int z0, int x1, int z1, int level, bool generate_ribbon)
// Write out the data for an edge of the chunk that was just generated.
// (x0,z0) - (x1,z1) defines the extent of the edge in the heightfield.
// level determines which vertices in the mesh are active.
//
// If generate_ribbon is true, then write a triangle list for the
// ribbon which will stitch this edge with two matching higher-lod
// neighbors.
{
	assert(x0 <= x1);
	assert(z0 <= z1);

	// We're going to write a list of vertices comprising the
	// edge.
	//
	// We're also going to write the index (in this list) of the
	// midpoint vertex of the edge, so the renderer can join
	// t-junctions.

	//
	// Vertices.
	//
	
	// Count the active verts, and find the index of the midpoint vert.
	int	verts = 0;
	int	midpoint_index = 0;

	// Step along the edge.
	int	dx = (x0 < x1) ? 1 : 0;
	int	dz = (z0 < z1) ? 1 : 0;
	int steps = imax(x1 - x0, z1 - z0) + 1;
	int	halfway = steps >> 1;
	for (int i = 0, x = x0, z = z0; i < steps; i++, x += dx, z += dz) {
		if (i == halfway) {
			midpoint_index = verts;
		}

		const heightfield_elem&	e = hf.get_elem(x, z);
		if (e.activation_level >= level) {
			verts++;	// This is an active vert.
		}
	}

	if (generate_ribbon) {
		// if we're not at the base level, generate a triangle
		// mesh which fills the gaps between this edge and a
		// matching edge composed of the facing edge of two
		// higher-LOD neighbors.
		
		// walk the edge, examining low-LOD and high-LOD vertices.
		int	lo_index = 0;
		int	hi_index = verts;
		int	hi_edge_part = 0;	// 0 for the first part of the high LOD edge, 1 for the second part.

		mesh::add_edge_vertex_lo(dir, x0, z0);
		mesh::add_edge_vertex_hi(dir, hi_edge_part, x0, z0);

		{for (int i = 1, x = x0 + dx, z = z0 + dz; i < steps; i++, x += dx, z += dz) {
			const heightfield_elem&	e = hf.get_elem(x, z);
			if (e.activation_level >= level - 1) {
				// high-lod vertex.
				mesh::add_edge_strip_index(dir, lo_index);
				mesh::add_edge_strip_index(dir, hi_index);
				hi_index++;
				mesh::add_edge_strip_index(dir, hi_index);

				mesh::add_edge_vertex_hi(dir, hi_edge_part, x, z);

				if (e.activation_level >= level) {
					// also a low-lod vertex.
					mesh::add_edge_strip_index(dir, lo_index);
					mesh::add_edge_strip_index(dir, hi_index);
					lo_index++;
					mesh::add_edge_strip_index(dir, lo_index);

					mesh::add_edge_vertex_lo(dir, x, z);

					if (lo_index == midpoint_index) {
						// Extra filler triangle, between the two hi-lod edges.
						mesh::add_edge_strip_index(dir, lo_index);
						mesh::add_edge_strip_index(dir, hi_index);
						hi_index++;
						mesh::add_edge_strip_index(dir, hi_index);

						hi_edge_part = 1;
						mesh::add_edge_vertex_hi(dir, hi_edge_part, x, z);
					}
				}
			}
		}}
	}
	else
	{
		// We're at the highest LOD level -- just generate a list of
		// our edge verts, for meshing with chunks at our same level.
		{for (int i = 0, x = x0, z = z0; i < steps; i++, x += dx, z += dz) {
			const heightfield_elem&	e = hf.get_elem(x, z);
			if (e.activation_level >= level) {
				mesh::add_edge_vertex_lo(dir, x, z);
			}
		}}
	}
}



namespace mesh {
// mini module for building up mesh data.

//data:
	struct vert_info {
		Sint16	x, z;

		vert_info() : x(-1), z(-1) {}
		vert_info(int vx, int vz) : x(vx), z(vz) {}
		bool	operator==(const vert_info& v) { return x == v.x && z == v.z; }
	};

	array<vert_info>	vertices;
	array<int>	vertex_indices;
	hash<vert_info, int>	index_table;	// to accelerate get_vertex_index()

	array<int>	edge_strip[4];
	array<vert_info>	edge_lo[4];
	array<vert_info>	edge_hi[4][2];

	vec3	min, max;	// for bounding box.


//code:
	int	get_vertex_index(int x, int z)
	// Return the index of the specified vert.  If the vert isn't
	// in our vertex list, then add it to the end.
	{
		int	index = lookup_index(x, z);

		if (index != -1) {
			// We already have that vert.
			return index;
		}

		index = vertices.size();
		vert_info	v(x, z);
		vertices.push_back(v);
		index_table.add(v, index);

		return index;
	}


	int	lookup_index(int x, int z)
	// Return the index in the current vertex array of the specified
	// vertex.  If the vertex can't be found in the current array,
	// then returns -1.
	{
		int	index;
		if (index_table.get(vert_info(x, z), &index)) {
			// Found it.
			return index;
		}
		// Didn't find it.
		return -1;
	}


	void	clear()
	// Reset and empty all our containers, to start a fresh mesh.
	{
		vertices.clear();
		vertex_indices.clear();
		index_table.clear();

		for (int i = 0; i < 4; i++) {
			edge_strip[i].clear();
			edge_lo[i].clear();
			edge_hi[i][0].clear();
			edge_hi[i][1].clear();
		}

		min = vec3(1000000, 1000000, 1000000);
		max = vec3(-1000000, -1000000, -1000000);
	}

	static void	write_vertex(SDL_RWops* rw, const heightfield& hf,
				     int level, const vec3& box_center, const vec3& compress_factor,
				     float vertical_scale,
				     const vert_info& v)
	// Utility function, to output the quantized data for a vertex.
	{
		const heightfield_elem&	e = hf.get_elem(v.x, v.z);

		Sint16	x, y, z;

		x = (int) floor(((v.x * hf.sample_spacing - box_center.get_x()) * compress_factor.get_x()) + 0.5);
		y = (int) floor(e.y / vertical_scale + 0.5);
		z = (int) floor(((v.z * hf.sample_spacing - box_center.get_z()) * compress_factor.get_z()) + 0.5);

		SDL_WriteLE16(rw, x);
		SDL_WriteLE16(rw, y);
		SDL_WriteLE16(rw, z);

		// Morph info.  Should work out to 0 if the vert is not a morph vert.
		float	lerped_height = get_height_at_LOD(hf, level + 1, v.x, v.z);
		int	morph_delta = floor((lerped_height - e.y) / vertical_scale + 0.5);
		SDL_WriteLE16(rw, (Sint16) morph_delta);
		assert(morph_delta == (Sint16) morph_delta);	// Watch out for overflow.
	}

	void	write(SDL_RWops* rw, const heightfield& hf, int level, float vertical_scale)
	// Write out the current chunk.
	{
		// Write bounding box.  Bounding box also determines the scale
		// and offset for quantizing the verts.
		vec3	box_center = (min + max) * 0.5f;
		vec3	box_extent = (max - min) * 0.5f;
		box_center.write(rw);
		box_extent.write(rw);

		vec3	compress_factor;
		{for (int i = 0; i < 3; i++) {
			compress_factor.set(i, ((1 << 15) - 1) / fmax(1.0, box_extent.get(i)));
		}}

		// Make sure the vertex buffer is not too big.
		if (vertices.size() >= (1 << 16)) {
			printf("error: chunk contains > 64K vertices.  Try processing again, but use\n"
				   "the -d <depth> option to make a deeper chunk tree.\n");
			exit(1);
		}

		// Write vertices.  All verts contain morph info.
		SDL_WriteLE16(rw, vertices.size());
		for (int i = 0; i < vertices.size(); i++) {
			write_vertex(rw, hf, level, box_center, compress_factor, vertical_scale,
				     vertices[i]);
		}

		{
			// Write triangle-strip vertex indices.
			SDL_WriteLE32(rw, vertex_indices.size());
			for (int i = 0; i < vertex_indices.size(); i++) {
				SDL_WriteLE16(rw, vertex_indices[i]);
			}
		}

		// Count the real triangles in the main chunk.
		{
			int	tris = 0;
			for (int i = 0; i < vertex_indices.size() - 2; i++) {
				if (vertex_indices[i] != vertex_indices[i+1]
					&& vertex_indices[i] != vertex_indices[i+2])
				{
					// Real triangle.
					tris++;
				}
			}

			stats.output_real_triangles += tris;
			stats.output_degenerate_triangles += (vertex_indices.size() - 2) - tris;

			// Write real triangle count.
			SDL_WriteLE32(rw, tris);
		}

//		// Print some stats.
//		printf("\tverts = %d, tris = %d\n", vertices.size(), (vertex_indices.size() - 2));

		//
		// Output our edge data.
		//
		for (int edge_dir = 0; edge_dir < 4; edge_dir++) {
			// Strip indices.
			// @@ TODO: THIS IS ACTUALLY AN INDEXED MESH, NOT STRIP.  FIX!
			assert(edge_strip[edge_dir].size() < (1 << 16));
			SDL_WriteLE16(rw, edge_strip[edge_dir].size());
			{for (int i = 0; i < edge_strip[edge_dir].size(); i++) {
				SDL_WriteLE16(rw, edge_strip[edge_dir][i]);
			}}

			// Vertex counts.
			assert(edge_lo[edge_dir].size() < (1 << 16));
			SDL_WriteLE16(rw, edge_lo[edge_dir].size());
			assert(edge_hi[edge_dir][0].size() < (1 << 16));
			SDL_WriteLE16(rw, edge_hi[edge_dir][0].size());
			assert(edge_hi[edge_dir][1].size() < (1 << 16));
			SDL_WriteLE16(rw, edge_hi[edge_dir][1].size());

			{for (int i = 0; i < edge_lo[edge_dir].size(); i++) {
				write_vertex(rw, hf, level, box_center, compress_factor, vertical_scale,
					     edge_lo[edge_dir][i]);
			}}
			{for (int j = 0; j < 2; j++) {
				{for (int i = 0; i < edge_hi[edge_dir][j].size(); i++) {
					write_vertex(rw, hf, level-1, box_center, compress_factor, vertical_scale,
						     edge_hi[edge_dir][j][i]);
				}}
			}}
		}
	}


	void	emit_vertex(const heightfield& hf, int x, int z)
	// Call this in strip order.  Inserts the given vert into the current strip.
	{
		int	index = get_vertex_index(x, z);
		vertex_indices.push_back(index);

		// Check coordinates and update bounding box.
		vec3	v(x * hf.sample_spacing, hf.get_elem(x, z).y, z * hf.sample_spacing);
		for (int i = 0; i < 3; i++) {
			if (v.get(i) < min.get(i)) {
				min.set(i, v.get(i));
			}
			if (v.get(i) > max.get(i)) {
				max.set(i, v.get(i));
			}
		}

		// Peephole optimization: if the strip begins with three of
		// the same vert in a row, as generate_block() often causes,
		// then the first two are a no-op, so strip them away.
		if (vertex_indices.size() == 3
			&& vertex_indices[0] == vertex_indices[1]
			&& vertex_indices[0] == vertex_indices[2])
		{
			vertex_indices.resize(1);
		}
	}

	void	add_edge_strip_index(int edge_dir, int index)
	// Add a vertex to the edge strip which joins this chunk to
	// two neighboring higher-LOD chunks.
	{
		assert(edge_dir >= 0 && edge_dir < 4);
		edge_strip[edge_dir].push_back(index);
	}

	void	add_edge_vertex_lo(int edge_dir, int x, int z)
	// Adds a low-LOD edge vertex to our list.
	{
		assert(edge_dir >= 0 && edge_dir < 4);
		edge_lo[edge_dir].push_back(vert_info(x, z));
	}

	void	add_edge_vertex_hi(int edge_dir, int hi_index, int x, int z)
	// Adds a high-LOD edge vertex to one of our lists.  Each edge
	// has two high-LOD sub-edges.
	{
		assert(edge_dir >= 0 && edge_dir < 4);
		assert(hi_index >= 0 && hi_index < 2);
		edge_hi[edge_dir][hi_index].push_back(vert_info(x, z));
	}
};

