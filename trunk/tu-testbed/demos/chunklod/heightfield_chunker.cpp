// heightfield_chunker.cpp	-- tu@tulrich.com Copyright 2001 by Thatcher Ulrich

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


void	heightfield_chunker(SDL_RWops* rwin, SDL_RWops* out, int tree_depth, float base_max_error);


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
		   "usage: heightfield_chunker [-d depth] [-e error] <input_filename> <output_filename>\n"
		   "\n"
		   "\tThe input filename should either be a .BT format terrain file with\n"
		   "\t(2^N+1) x (2^N+1) datapoints, or a grayscale bitmap.\n"
		   "\n"
		   "\t'depth' gives the depth of the quadtree of chunks to generate; default = 6\n"
		   "\t'error' gives the maximum geometric error to allow at full LOD; default = 0.5\n"
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


#undef main	// @@ some crazy SDL/WIN32 thing that I don't understand.
int	main(int argc, char* argv[])
// Reads the given .BT terrain file or grayscale bitmap, and generates
// a quadtree-chunked LOD data file, suitable for viewing by the
// 'chunklod' program.
{
	// Default processing parameters.
	int	tree_depth = 6;
	float	max_geometric_error = 0.5f;

	// Process command-line options.
	char*	infile = NULL; //argv[1];
	char*	outfile = NULL; //argv[2];

	for ( int arg = 1; arg < argc; arg++ ) {
		if ( argv[arg][0] == '-' ) {
			// command-line switch.
			
			switch ( argv[arg][1] ) {
			case 'h':
			case '?':
				print_usage();
				exit( 1 );
				break;

			case 'd':
				// Set the tree depth.
				arg++;
				if ( arg < argc ) {
					tree_depth = atoi( argv[ arg ] );

				} else {
					printf( "error: -d option must be followed by an integer for the tree depth\n" );
					print_usage();
					exit( 1 );
				}
				break;

			case 'e':
				// Set the max geometric error.
				arg++;
				if ( arg < argc ) {
					max_geometric_error = atof( argv[ arg ] );

				} else {
					printf( "error: -e option must be followed by a value for the maximum geometric error\n" );
					print_usage();
					exit( 1 );
				}
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
	printf( "depth = %d, max error = %f\n", tree_depth, max_geometric_error );

	// Process the data.
	heightfield_chunker( in, out, tree_depth, max_geometric_error );

	stats.output_size = SDL_RWtell(out);

	SDL_RWclose(in);
	SDL_RWclose(out);

	// Print some stats.
	printf("========================================\n");
	printf("                chunks: %10d\n", stats.output_chunks);
	printf("           input verts: %10d\n", stats.input_vertices);
	printf("          output verts: %10d\n", stats.output_vertices);

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
	{
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

		sample_spacing = 1.0;	// TODO: parameterize this

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
void	generate_node_data(SDL_RWops* rw, heightfield& hf, int x0, int z0, int log_size, int level);


void	heightfield_chunker(SDL_RWops* in, SDL_RWops* out, int tree_depth, float base_max_error)
// Generate LOD chunks from the given heightfield.
// 
// tree_depth determines the depth of the chunk quadtree.
// 
// base_max_error specifies the maximum allowed geometric vertex error,
// at the finest level of detail.
{
	heightfield	hf;

	// Load the heightfield data.
	if (hf.load_bt(in) < 0) {
		hf.load_bitmap(in);
	}

	// Initialize the mesh info -- clear the meshing 
	for (int j = 0; j < hf.size; j++) {
		for (int i = 0; i < hf.size; i++) {
			hf.elem(i, j).clear();
		}
	}

	stats.input_vertices = hf.size * hf.size;

	// Run a view-independent L-K style BTT update on the heightfield, to generate
	// error and activation_level values for each element.
	update(hf, base_max_error, 0, hf.size-1, hf.size-1, hf.size-1, 0, 0);	// sw half of the square
	update(hf, base_max_error, hf.size-1, 0, 0, 0, hf.size-1, hf.size-1);	// ne half of the square

	// Propagate the activation_level values of verts to their
	// parent verts, quadtree LOD style.  Gives same result as
	// L-K.
	for (int i = 0; i < hf.log_size; i++) {
		propagate_activation_level(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1, i);
		propagate_activation_level(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1, i);
	}

	check_propagation(hf, hf.size >> 1, hf.size >> 1, hf.log_size - 1);//xxxxx

	// Write a .chu header for the output file.
	SDL_WriteLE32(out, ('C') | ('H' << 8) | ('U' << 16));	// four byte "CHU\0" tag
	SDL_WriteLE16(out, 3);	// file format version.
	SDL_WriteLE16(out, tree_depth);	// depth of the chunk quadtree.
	WriteFloat32(out, base_max_error);	// max geometric error at base level mesh.
	SDL_WriteLE32(out, 0x55555555 & ((1 << (tree_depth*2)) - 1));	// Chunk count.  Fully populated quadtree.

	// Write out the node data for the entire chunk tree.
	generate_node_data(out, hf, 0, 0, hf.log_size, tree_depth-1);
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


const float	SQRT_2 = sqrtf( 2 );


float	height_query( const heightfield& hf, int level, int x, int z, int ax, int az, int rx, int rz, int lx, int lz )
// Returns the height of the query point (x,z) within the triangle (a, r, l),
// as tesselated to the specified LOD.
{
	// If the query is on one of our verts, return that vert's height.
	if ( ( x == ax && z == az )
		 || ( x == rx && z == rz )
		 || ( x == lx && z == lz ) )
	{
		return hf.get_elem( x, z ).y;
	}

	// Compute the coordinates of this triangle's base vertex.
	int	dx = lx - rx;
	int	dz = lz - rz;
	if (iabs(dx) <= 1 && iabs(dz) <= 1) {
		// We've reached the base level.  This is an error condition; we should
		// have gotten a successful test earlier.

		// assert(0);
		printf( "Error: height_query hit base of heightfield.\n" );

		return hf.get_elem( ax, az ).y;
	}

	// base vert is midway between left and right verts.
	int	bx = rx + (dx >> 1);
	int	bz = rz + (dz >> 1);

	// compute the length of a side edge.
	float	edge_length_squared = ( dx * dx + dz * dz ) / 2.f;

	float	sr, sl;	// barycentric coords w/r/t the right and left edges.
	sr = ( ( x - ax ) * ( rx - ax ) + ( z - az ) * (rz - az ) ) / edge_length_squared;
	sl = ( ( x - ax ) * ( lx - ax ) + ( z - az ) * (lz - az ) ) / edge_length_squared;

	int	base_vert_level = hf.get_elem( bx, bz ).get_activation_level();
	if ( base_vert_level >= level ){
		// The mesh is more tesselated at the desired LOD.  Recurse.
		if ( sr >= sl ) {
			// Query is in right child triangle.
			return height_query( hf, level, x, z, bx, bz, ax, az, rx, rz);	// base, apex, right
		} else {
			// Query is in left child triangle.
			return height_query( hf, level, x, z, bx, bz, lx, lz, ax, az);	// base, left, apex
		}
	}

	float	ay = hf.get_elem( ax, az ).y;
	float	dr = hf.get_elem( rx, rz ).y - ay;
	float	dl = hf.get_elem( lx, lz ).y - ay;

	// This triangle is as far as the desired LOD goes.  Compute the
	// query's height on the triangle.
	return ay + sl * dl + sr * dr;
}


float	get_height_at_LOD( const heightfield& hf, int level, int x, int z )
// Returns the height of the mesh as simplified to the specified level
// of detail.
{
	if ( z > x ) {
		// Query in SW quadrant.
		return height_query( hf, level, x, z, 0, hf.size-1, hf.size-1, hf.size-1, 0, 0);	// sw half of the square

	} else {	// query in NW quadrant
		return height_query( hf, level, x, z, hf.size-1, 0, 0, 0, hf.size-1, hf.size-1);	// ne half of the square
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
	void	emit_vertex(int ax, int az);	// call this in strip order.
	void	write(SDL_RWops* rw, const heightfield& hf, int activation_level, int chunk_label);
};


void	gen_mesh(heightfield& hf, int level, int ax, int az, int rx, int rz, int lx, int lz);

void	generate_edge(SDL_RWops* out, heightfield& hf, int level, int ax, int az, int bx, int bz);


struct gen_state;
void	generate_block(heightfield& hf, int level, int log_size, int cx, int cz);
void	generate_quadrant(heightfield& hf, gen_state* s, int lx, int lz, int tx, int tz, int rx, int rz, int level);


void	generate_node_data(SDL_RWops* out, heightfield& hf, int x0, int z0, int log_size, int level)
// Given a square of data, with center at (x0, z0) and comprising
// ((1<<log_size)+1) verts along each axis, this function generates
// the mesh using verts which are active at the given level.
//
// If we're not at the base level (level > 0), then also recurses to
// quadtree child nodes and generates their data.
{
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

	// Ensure our center vert is activated, too.
	hf.elem(x0 + half_size, z0 + half_size).activate(level);

	// Generate the mesh.
	generate_block(hf, level, log_size, x0 + half_size, z0 + half_size);

//	// Print some interesting info.
//	printf("chunk: (%d, %d) size = %d\n", x0, z0, size);

	// write out the mesh data.
	mesh::write(out, hf, level, chunk_label);

	// recurse to child regions, to generate child chunks.
	if (level > 0) {
		int	half_size = (1 << (log_size-1));
		generate_node_data(out, hf, x0, z0, log_size-1, level-1);	// nw
		generate_node_data(out, hf, x0 + half_size, z0, log_size-1, level-1);	// ne
		generate_node_data(out, hf, x0, z0 + half_size, log_size-1, level-1);	// sw
		generate_node_data(out, hf, x0 + half_size, z0 + half_size, log_size-1, level-1);	// se
	}

	// Generate data for the four internal edges.
	if (level > 0) {
		// Four internal edges.  They connect the center with each of
		// the four outside edges, and stitch together our subnodes.
		// The "edges" are a polyline made up of all the active
		// vertices along the border between the relevant active
		// nodes.
		//
		// Each edge is actually the root of a binary tree of edges,
		// which join all of the subchunks of the quadtree along the
		// root edge.  Tip o' the hat to Sean Barrett for "LOD edge"
		// == "binary tree" concept.
		WriteByte(out, 4);	// edge count.
		generate_edge(out, hf, level - 1, cx, cz, cx + half_size, cz /* e edge */);
		generate_edge(out, hf, level - 1, cx, cz, cx, cz - half_size /* n edge */);
		generate_edge(out, hf, level - 1, cx, cz, cx - half_size, cz /* w edge */);
		generate_edge(out, hf, level - 1, cx, cz, cx, cz + half_size /* s edge */);
	} else {
		// No subnodes --> no internal edges.
		WriteByte(out, 0);	// edge count.
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

	mesh::clear();

	mesh::emit_vertex(q[0][0], q[0][1]);
	state.set_my_buffer(q[0][0], q[0][1]);

	{for (int i = 0; i < 4; i++) {
		if ((state.previous_level & 1) == 0) {
			// tulrich: turn a corner?
			state.ptr ^= 1;
		} else {
			// tulrich: jump via degenerate?
			int	x = state.my_buffer[1 - state.ptr][0];
			int	z = state.my_buffer[1 - state.ptr][1];
			mesh::emit_vertex(x, z);	// or, emit vertex(last - 1);
		}

		// Initial vertex of quadrant.
		mesh::emit_vertex(q[i][0], q[i][1]);
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
		mesh::emit_vertex(q[0][0], q[0][1]);
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
				mesh::emit_vertex(x, z);	// or, emit vertex(last - 1);
			}
			mesh::emit_vertex(tx, tz);
			s->set_my_buffer(tx, tz);
			s->previous_level = recursion_level;
		}

		generate_quadrant(hf, s, tx, tz, bx, bz, rx, rz, recursion_level - 1);
	}
}


void	generate_edge(SDL_RWops* out, heightfield& hf, int level, int ax, int az, int bx, int bz)
// Takes the coordinates of the endpoints of a N<-->S or E<-->W line
// in the heightfield, and writes out the lod_edge data for that line.
// That means the vertices along that line which are active at the
// specified level, with morphing info, along with the labels of the
// bordering heightfield nodes on either side.
//
// Also, does binary subdivision of the edge, and writes lod_edge data
// for all the descendant edges.
{
	// Compute the half-delta of the edge coords.  Useful for finding the midpoint of the edge,
	// and the center of neighboring chunks.
	int	hdx = (bx - ax) >> 1;
	int	hdz = (bz - az) >> 1;

	// We must write out the chunk labels of the nodes that this edge
	// connects.  This is essential information for the chunk
	// renderer, so it can locate the chunks to attach the edge to.
	// In this program we're generating a LOD chunk tree from a
	// quadtree heightfield, which is completely regular, so the chunk
	// labels are implicit given the location of the chunk, just by
	// using the breadth-first quadtree traversal index (depth-first
	// works too, like swizzling, but the index computation needs to
	// know the total tree depth).  We must write it out explicitly
	// because we want the renderer to be capable of dealing with more
	// general mesh structures.
	int	neighbor0 = hf.node_index(ax + hdx + hdz, az + hdz + hdx);	// center of neighbor chunk.
	int	neighbor1 = hf.node_index(ax + hdx - hdz, az + hdz - hdx);	// center of the other neighbor chunk.

	// If we always sort the labels, then the edge ordering should be
	// coherent throughout the tree.  I.e. neighbor0 of my child edges
	// should be the child chunks of my neighbor0.  (This is important
	// to the renderer, when stitching a "t junction" of LODs.)
	if (neighbor0 > neighbor1) {
		int	temp = neighbor0;	// swap.
		neighbor0 = neighbor1;
		neighbor1 = temp;
	}

	SDL_WriteLE32(out, neighbor0);
	SDL_WriteLE32(out, neighbor1);

	//
	// Vertices.
	//
	
	// Count the active verts, and find the index of the midpoint vert.
	int	verts = 0;
	int	midpoint_index = 0;

	// Step along the edge.
	int	dx = iclamp(hdx, -1, 1);
	int	dz = iclamp(hdz, -1, 1);
	int steps = imax(iabs(bx - ax), iabs(bz - az)) + 1;
	for (int i = 0, x = ax, z = az; i < steps; i++, x += dx, z += dz) {
		if (x == ax + hdx && z == az + hdz) {
			midpoint_index = verts;
		}

		const heightfield_elem&	e = hf.get_elem(x, z);
		if (e.activation_level >= level) {
			verts++;	// This is an active vert.
		}
	}

	SDL_WriteLE16(out, midpoint_index);

	// TODO: scale and offset, before quantizing...
	// xxxx write dummy offset & scale for now.
	WriteFloat32(out, 0);
	WriteFloat32(out, 0);
	WriteFloat32(out, 0);
	WriteFloat32(out, 1);

	// Write the active verts.
	assert(verts < (1 << 16));
	SDL_WriteLE16(out, verts);	// vertex count.
	{for (int i = 0, x = ax, z = az; i < steps; i++, x += dx, z += dz) {
		const heightfield_elem&	e = hf.get_elem(x, z);
		int	l = e.activation_level;
		if (e.activation_level >= level) {
			SDL_WriteLE16(out, (int) (x * hf.sample_spacing));
			SDL_WriteLE16(out, (int) (e.y));
			SDL_WriteLE16(out, (int) (z * hf.sample_spacing));

			// Morph info.  Works out to 0 for non-morphing verts.
			float	lerped_height = get_height_at_LOD(hf, level + 1, x, z);
			SDL_WriteLE16(out, lerped_height - e.y);	// delta, to get to y value of parent mesh.
		}
	}}

	// TODO: compute the correct strips and write them out.  Currently
	// they're generated on the fly in chunkdemo.
	SDL_WriteLE32(out, 0);	// xxx for now, a zero-length strip...

	// real triangle count == 0; will get recomputed by renderer.
	SDL_WriteLE32(out, 0);

//	// Print some stats.
//	printf("\t\tedge, verts = %d, mverts = %d\n", verts, morph_verts);
	
	// Write out child edges, if we have them.
	if (level > 0) {
		WriteByte(out, 1);	// "children present" flag.
		generate_edge(out, hf, level - 1, ax, az, ax + hdx, az + hdz);
		generate_edge(out, hf, level - 1, ax + hdx, az + hdz, bx, bz);
	} else {
		WriteByte(out, 0);	// "no children" flag.
	}
}


namespace mesh {
// mini module for building up mesh data.

	struct vert_info {
		int	x, z;

		vert_info(int vx, int vz) : x(vx), z(vz) {}
	};

	array<vert_info>	vertices;
	array<int>	vertex_indices;

	int	get_vertex_index(int x, int z)
	// Return the index of the specified vert.  If the vert isn't
	// in our vertex list, then add it to the end.
	{
		// Look for the vert in our vertex list.  Should
		// probably use a map or something instead of linear
		// search.
		for (int i = 0; i < vertices.size(); i++) {
			if (vertices[i].x == x && vertices[i].z == z) {
				// Found our vert.  Return the index.
				return i;
			}
		}

		vertices.push_back(vert_info(x, z));

		return vertices.size() - 1;
	}

	void	clear()
	// Reset and empty all our containers, to start a fresh mesh.
	{
		vertices.resize(0);
		vertex_indices.resize(0);
	}


	void	write(SDL_RWops* rw, const heightfield& hf, int level, int chunk_label)
	{
		// Label the chunk, so edge datasets can specify the chunks they're associated with.
		SDL_WriteLE32(rw, chunk_label);

		// Write origin and scale.
		// TODO: fix this.  should also write bounding box to the file.
		WriteFloat32(rw, 0);
		WriteFloat32(rw, 0);
		WriteFloat32(rw, 0);
		WriteFloat32(rw, 1);

		// TODO: scale and offset, before quantizing...

		// Write vertices.  All verts contain morph info.
		SDL_WriteLE16(rw, vertices.size());
		for (int i = 0; i < vertices.size(); i++) {
			const vert_info&	v = vertices[i];
			const heightfield_elem&	e = hf.get_elem(v.x, v.z);
			SDL_WriteLE16(rw, (int) (v.x * hf.sample_spacing));
			SDL_WriteLE16(rw, (int) e.y);	// true y value
			SDL_WriteLE16(rw, (int) (v.z * hf.sample_spacing));

			// Morph info.  Should work out to 0 if the vert is not a morph vert.
			float	lerped_height = get_height_at_LOD(hf, level + 1, v.x, v.z);
			SDL_WriteLE16(rw, (int) (lerped_height - e.y));	// delta, to get to y value of parent mesh.
		}

		{
			// Write triangle-strip vertex indices.
			SDL_WriteLE32(rw, vertex_indices.size());
			for (int i = 0; i < vertex_indices.size(); i++) {
				SDL_WriteLE16(rw, vertex_indices[i]);
			}
		}

		// Count the real triangles.
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
	}


	void	emit_vertex(int x, int z)
	// Call this in strip order.  Inserts the given vert into the current strip.
	{
		int	index = get_vertex_index(x, z);
		vertex_indices.push_back(index);

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
};

