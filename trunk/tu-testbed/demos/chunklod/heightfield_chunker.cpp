// heightfield_chunker.cpp	-- tu@tulrich.com Copyright 2001 by Thatcher Ulrich

// Program to take heightfield input and generate a chunked-lod
// structure as output.  Uses quadtree decomposition, with a
// Lindstrom-Koller-esque decimation algorithm used to decimate
// the individual chunks.


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utility.h"


void	heightfield_chunker(SDL_RWops* rwin, SDL_RWops* rwout, int tree_depth, float base_max_error);


int	main(int argc, char* argv[])
// Reads the given .BT terrain file, and generate a quadtree-chunked LOD data file,
// suitable for viewing by the 'chunklod' program.
{
	// look for input filename.
	if (argc <= 2
	    || ((argv[1][0] == '-' && (argv[1][1] == 'h' || argv[1][1] == '?'))))
	{
		// no args, or -h or -?.  print usage.
		printf("heightfield_chunker: program for processing terrain data and generating\n"
		       "a chunked LOD data file suitable for viewing by 'chunklod'.\n\n"
		       "usage: heightfield_chunker <input_filename> <output_filename>\n"
		       "\tThe input filename should be a .BT format terrain file, and must be\n"
		       "\tsquare with (2^N+1) x (2^N+1) datapoints.\n"
			);
		exit(1);
	}

	char*	infile = argv[1];
	char*	outfile = argv[2];

	printf("infile: %s\n", infile);
	printf("outfile: %s\n", outfile);
	SDL_RWops*	in = SDL_RWFromFile(infile, "rb");
	SDL_RWops*	out = SDL_RWFromFile(outfile, "wb");

	heightfield_chunker(in, out, 6, 1.0);

	SDL_RWclose(in);
	SDL_RWclose(out);

	return 0;
}


struct heightfield_elem {
	float	y;
	float	error;
	Sint8	activation_level;
public:

	heightfield_elem() { y = 0; clear(); }

	void	clear()
	{
		error = 0;
		activation_level = -1;
	}

	void	set_activation_level(int l) {
		activation_level = l;

		if (l < 0 || l > 127) {
			printf("set_activation_level(%d)!!!\n", l);
		}
	}

	int	get_activation_level() const { return activation_level; }

	void	activate(int level)
	// Sets the activation_level to the given level, if it's greater than
	// the vert's current activation level.
	{
		if (level > activation_level) {
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

	void	load(SDL_RWops* in)
	// Load .BT format heightfield data from the given file and
	// initializes heightfield.
	{
		clear();

		// Read .BT header.

		// File-type marker.
		char	buf[11];
		SDL_RWread(in, buf, 1, 10);
		buf[10] = 0;
		if (strcmp(buf, "binterr1.1") != 0) {
			// Bad input file format.  Must be BT 1.1.
			throw "heightfield::load() -- file format is not BT 1.1 (see vterrain.org for format details)";
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
		SDL_RWseek(in, 256, SEEK_SET);

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
	}
};


void	update(heightfield& hf, float base_max_error, int ax, int az, int rx, int rz, int lx, int lz);
void	propagate_activation_level(heightfield& hf, int cx, int cz, int level, int target_level);
int	check_propagation(heightfield& hf, int cx, int cz, int level);
void	generate_node_data(SDL_RWops* rw, heightfield& hf, int x0, int z0, int log_size, int level);


void	heightfield_chunker(SDL_RWops* rwin, SDL_RWops* rwout, int tree_depth, float base_max_error)
// Generate LOD chunks from the given heightfield.
// 
// tree_depth determines the depth of the chunk quadtree.
// 
// base_max_error specifies the maximum allowed geometry vertex error,
// at the finest level of detail.
{
	heightfield	hf;

	// Load the heightfield data.
	hf.load(rwin);

	// Initialize the mesh info -- clear the meshing 
	for (int j = 0; j < hf.size; j++) {
		for (int i = 0; i < hf.size; i++) {
			hf.elem(i, j).clear();
		}
	}

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
	SDL_WriteLE16(rwout, 1);	// file format version.
	SDL_WriteLE16(rwout, tree_depth);	// depth of the chunk quadtree.
	WriteFloat32(rwout, base_max_error);	// max geometric error at base level mesh.

	// Write out the node data for the entire chunk tree.
	generate_node_data(rwout, hf, 0, 0, hf.log_size, tree_depth-1);
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
	void	add_triangle(int ax, int az, int bx, int bz, int cx, int cz);
	void	add_morph_vertex(int x, int z);
	void	write(SDL_RWops* rw, const heightfield& hf, int activation_level);
};


void	gen_mesh(heightfield& hf, int level, int ax, int az, int rx, int rz, int lx, int lz);


void	generate_node_data(SDL_RWops* rw, heightfield& hf, int x0, int z0, int log_size, int level)
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

	mesh::clear();

	// !!! This needs to be done in propagate, or something (too late now) !!!
	// Make sure our corner verts are activated on this level.
	hf.elem(x0 + size, z0).activate(level);
	hf.elem(x0, z0).activate(level);
	hf.elem(x0, z0 + size).activate(level);
	hf.elem(x0 + size, z0 + size).activate(level);

	// check for morphing of corner verts.
	if (hf.elem(x0 + size, z0).get_activation_level() == level) {
		mesh::add_morph_vertex(x0 + size, z0);
	}
	if (hf.elem(x0, z0).get_activation_level() == level) {
		mesh::add_morph_vertex(x0, z0);
	}
	if (hf.elem(x0, z0 + size).get_activation_level() == level) {
		mesh::add_morph_vertex(x0, z0 + size);
	}
	if (hf.elem(x0 + size, z0 + size).get_activation_level() == level) {
		mesh::add_morph_vertex(x0 + size, z0 + size);
	}

	// Generate the mesh.  Decide what direction to go based on where we are -- the
	// primary split direction alternates in a checkerboard pattern.
	if (((cx ^ cz) & (1 << (log_size-1))) == 0) {
		gen_mesh(hf, level, x0, z0 + size, x0 + size, z0 + size, x0, z0);	// sw half
		gen_mesh(hf, level, x0 + size, z0, x0, z0, x0 + size, z0 + size);	// ne half
	} else {
		gen_mesh(hf, level, x0, z0, x0, z0 + size, x0 + size, z0);	// nw half
		gen_mesh(hf, level, x0 + size, z0 + size, x0 + size, z0, x0, z0 + size);	// se half
	}

	// Print some interesting info.
	printf("chunk: (%d, %d) size = %d\n", x0, z0, size);

	// write out the mesh data.
	mesh::write(rw, hf, level);

	// recurse to child regions, to generate child chunks.
	if (level > 0) {
		int	half_size = (1 << (log_size-1));
		generate_node_data(rw, hf, x0, z0, log_size-1, level-1);	// nw
		generate_node_data(rw, hf, x0 + half_size, z0, log_size-1, level-1);	// ne
		generate_node_data(rw, hf, x0, z0 + half_size, log_size-1, level-1);	// sw
		generate_node_data(rw, hf, x0 + half_size, z0 + half_size, log_size-1, level-1);	// se

		// generate data for the four internal edges
	}
}


void	gen_mesh(heightfield& hf, int level, int ax, int az, int rx, int rz, int lx, int lz)
// Takes vertex coordinates of a right triangle: apex (ax, az), right
// (rx, rz), and left (lx, lz) verts.  Looks at the base vertex of the
// triangle, which is midway between the left and right verts.  If the
// base is active at this level, then subdivides; otherwise this
// triangle is part of the desired mesh, so we generate the triangle.
{
	// Compute the coordinates of this triangle's base vertex.
	int	dx = lx - rx;
	int	dz = lz - rz;
	if (iabs(dx) <= 1 && iabs(dz) <= 1) {
		// We've reached the base level.  Generate the triangle;
		// don't try to subdivide.
		mesh::add_triangle(ax, az, rx, rz, lx, lz);	// apex, right, left
		return;
	}

	// base vert is midway between left and right verts.
	int	bx = rx + (dx >> 1);
	int	bz = rz + (dz >> 1);

	if (hf.elem(bx, bz).get_activation_level() >= level) {
		if (hf.elem(bx, bz).get_activation_level() == level
		    && hf.elem(bx, bz).error != 0.f)
		{
			// this is a morph vert.
			mesh::add_morph_vertex(bx, bz);
		}

		// recurse to generate submesh.
		gen_mesh(hf, level, bx, bz, ax, az, rx, rz);	// base, apex, right
		gen_mesh(hf, level, bx, bz, lx, lz, ax, az);	// base, left, apex
	} else {
		// base vert is not active in this level's mesh, so
		// just generate the parent triangle.
		mesh::add_triangle(ax, az, rx, rz, lx, lz);	// apex, right, left
	}
}


#include <vector>
using namespace std;


namespace mesh {
// mini module for building up mesh data.

	struct vert_info {
		int	x, z;

		vert_info(int vx, int vz) : x(vx), z(vz) {}
	};

	vector<vert_info>	vertices;
	vector<int>	morph_vertices;
	vector<int>	vertex_indices;

	int	get_vertex_index(int x, int z)
	// Return the index of the specified vert.  If the vert isn't
	// in our vertex list, then add it to the end.
	{
		// Look for the vert in our vertex list.  Should
		// probably use a map or something instead of linear
		// search.
		int 	index = 0;
		for (vector<vert_info>::iterator it = vertices.begin(); it != vertices.end(); ++it) {
			if ((*it).x == x && (*it).z == z) {
				// Found our vert.  Return the index.
				return index;
			}

			index++;
		}

		vertices.push_back(vert_info(x, z));

		return index;
	}

	void	clear()
	// Reset and empty all our containers, to start a fresh mesh.
	{
		vertices.clear();
		morph_vertices.clear();
		vertex_indices.clear();
	}


	void	write(SDL_RWops* rw, const heightfield& hf, int level)
	{
		// Write vertices.
		SDL_WriteLE16(rw, vertices.size());
		for (vector<vert_info>::iterator it = vertices.begin(); it != vertices.end(); ++it) {
			WriteFloat32(rw, (*it).x * hf.sample_spacing);
			WriteFloat32(rw, hf.get_elem((*it).x, (*it).z).y);
			WriteFloat32(rw, (*it).z * hf.sample_spacing);
		}

		{
			// Write triangle vertex indices.
			SDL_WriteLE32(rw, vertex_indices.size());
			for (vector<int>::iterator it = vertex_indices.begin(); it != vertex_indices.end(); ++it) {
				SDL_WriteLE16(rw, (*it));
			}
		}

		{
			// Write morph verts.
			SDL_WriteLE16(rw, morph_vertices.size());
			for (vector<int>::iterator it = morph_vertices.begin(); it != morph_vertices.end(); ++it) {
				int	index = *it;
				SDL_WriteLE16(rw, index);	// vert index.

				// TODO: this query needs to return the real height of the vert, and
				// the height as represented at a specific LOD level.
				// Could query parent mesh, instead of querying heightfield?
				const heightfield_elem&	e = hf.get_elem(vertices[index].x, vertices[index].z);
				float	lerped_height = get_height_at_LOD( hf, level + 1, vertices[index].x, vertices[index].z );
				WriteFloat32(rw, lerped_height /* e.y - e.error */);	// y value of parent mesh.
				WriteFloat32(rw, e.y - lerped_height /* e.error */);	// delta, to get to true y value.
			}
		}

		// Print some stats.
		printf("\tverts = %d, tris = %d, mverts = %d\n", vertices.size(), vertex_indices.size() / 3, morph_vertices.size());
	}


	void	add_morph_vertex(int x, int z)
	// Add the specified vertex to our list of morph vertices.
	{
		int	index = get_vertex_index(x, z);

		for (vector<int>::iterator it = morph_vertices.begin(); it != morph_vertices.end(); ++it) {
			if (*it == index) {
				// This vert is already listed as a morph vert.
				return;
			}
		}

		// Add to our list of morph verts.
		morph_vertices.push_back(index);
	}


	void	add_triangle(int ax, int az, int bx, int bz, int cx, int cz)
	// Inserts the three specified verts into our triangle list.
	{
		int	index_a = get_vertex_index(ax, az);
		int	index_b = get_vertex_index(bx, bz);
		int	index_c = get_vertex_index(cx, cz);

		vertex_indices.push_back(index_a);
		vertex_indices.push_back(index_b);
		vertex_indices.push_back(index_c);
	}
};

