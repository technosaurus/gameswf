// chunklod.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Hardware-friendly chunked LOD.  Chunks are aware of edges shared by
// their neighbors and fill appropriately.  Chunks are organized in a
// tree, with each successive level having half the max vertex error
// of its parent.  Collapsed vertices in each chunk are morphed
// according to the view distance of the chunk.  Morphing is just
// simple lerping and can be done by the CPU, or with a vertex shader
// (at the expense of doubling the size of the vertex data in general,
// or upping it to 4 coords/vert for heightfield terrain, where vertex
// morphing is always vertical).


#include <stdlib.h>
#include <string.h>

#include <engine/ogl.h>
#include <engine/utility.h>

#include "chunklod.h"


#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI


float	morph_curve(float f)
// Given a value from 0 to 1 that represents the progress of a chunk
// from its minimum view distance to its maximum view distance, return
// a morph value from 0 to 1 that determines how much to morph the
// verts.
//
// A function with flat sections near 0 and 1 may help avoid popping
// when chunks switch LODs.  Such popping is only an issue under some
// more extreme parameters of allowed pixel error and chunk size.
// Need to analyze this further to understand the conditions better.
//
// Alternatively, a time-based morph might work.  Or a morph
// computation that looks around in the tree to make sure it
// distributes the morph range properly around splits.  Needs some
// thought...
{
//	return fclamp((f - 0.5f) * 2 + 0.5f, 0, 1);
	return f;
}


static ogl::vertex_stream*	s_stream = NULL;


static float	s_vertical_scale = 1.0;


struct vertex_info {
// Structure for storing morphable vertex mesh info.
	int	vertex_count;
	struct vertex {
		Sint16	x[3];
		Sint16	y_delta;	// delta, to get to morphed y

		void	read(SDL_RWops* in)
		{
			x[0] = SDL_ReadLE16(in);
			x[1] = SDL_ReadLE16(in);
			x[2] = SDL_ReadLE16(in);
			y_delta = SDL_ReadLE16(in);
		}
	};
	vertex*	vertices;

	int	index_count;
	Uint16*	indices;

	int	triangle_count;

	// mesh type (GL_TRIANGLE_STRIP vs GL_TRIANGLES)
	// texture id
	// other?

	void	read(SDL_RWops* in);

	// TODO: destructor.
};


enum direction {
	EAST = 0,
	NORTH,
	WEST,
	SOUTH
};


struct lod_edge {
//	int	vertex_count;
//	int	midpoint_index;
//	Uint16*	indices;

	int	ribbon_index_count;
	Uint16*	ribbon_indices;

	int	lo_vertex_count;
	int	hi_vertex_count[2];
	vertex_info::vertex*	edge_verts;

	void	read(SDL_RWops* in);

	// @@ constructor, destructor
};


void	lod_edge::read(SDL_RWops* in)
// Initialize this edge structure from the specified stream.
{
#if 0
	midpoint_index = SDL_ReadLE16(in);
	vertex_count = SDL_ReadLE16(in);
	indices = new Uint16[vertex_count];

	for (int i = 0; i < vertex_count; i++) {
		indices[i] = SDL_ReadLE16(in);
	}

	// @@ hm, might be nice to have array<>::read()/write()...
#endif // 0

	ribbon_index_count = SDL_ReadLE16(in);
	assert(ribbon_index_count % 3 == 0);	// this is a triangle list; should be 3n indices.
	ribbon_indices = new Uint16[ribbon_index_count];
	{for (int i = 0; i < ribbon_index_count; i++) {
		ribbon_indices[i] = SDL_ReadLE16(in);
	}}

	// Read the vertex info.
	lo_vertex_count = SDL_ReadLE16(in);
	hi_vertex_count[0] = SDL_ReadLE16(in);
	hi_vertex_count[1] = SDL_ReadLE16(in);
	int	total_verts = lo_vertex_count + hi_vertex_count[0] + hi_vertex_count[1];
	edge_verts = new vertex_info::vertex[total_verts];
	{for (int i = 0; i < total_verts; i++) {
		edge_verts[i].read(in);
	}}
}


struct lod_chunk {
//data:
	lod_chunk*	parent;
	Uint16	lod;	// LOD of this chunk.  high byte never changes; low byte is the morph parameter.
	bool	split;	// true if this node should be rendered by descendents.

	// Our edges.
	lod_edge	edge[4];

	int	child_count;
	lod_chunk**	children;

	union {
		int	label;
		lod_chunk*	chunk;
	} neighbor[4];

	// AABB, used for deciding when to enable.
	vec3	box_center;
	vec3	box_extent;
	
	vertex_info	verts;	// vertex and mesh info; vertex array w/ morph targets, indices, texture id

//methods:
	// needs a destructor!

	void	clear();
	void	update(const lod_chunk_tree& c, const vec3& viewpoint);
	void	do_split(const lod_chunk_tree& base, const vec3& viewpoint);
	int	render(const lod_chunk_tree& c, const view_state& v, cull::result_info cull_info, render_options opt);
	int	render_edge(direction dir, render_options opt);

	void	read(SDL_RWops* in, int level, lod_chunk_tree* tree);
	void	lookup_neighbors(lod_chunk_tree* tree);
};


static void	draw_box(const vec3& min, const vec3& max)
// Draw the specified axis-aligned box.
{
	glBegin(GL_LINES);
	glVertex3f(min.get_x(), min.get_y(), min.get_z());
	glVertex3f(min.get_x(), max.get_y(), min.get_z());
	glVertex3f(min.get_x(), min.get_y(), max.get_z());
	glVertex3f(min.get_x(), max.get_y(), max.get_z());
	glVertex3f(max.get_x(), min.get_y(), min.get_z());
	glVertex3f(max.get_x(), max.get_y(), min.get_z());
	glVertex3f(max.get_x(), min.get_y(), max.get_z());
	glVertex3f(max.get_x(), max.get_y(), max.get_z());
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex3f(min.get_x(), min.get_y(), min.get_z());
	glVertex3f(min.get_x(), min.get_y(), max.get_z());
	glVertex3f(max.get_x(), min.get_y(), max.get_z());
	glVertex3f(max.get_x(), min.get_y(), min.get_z());
	glVertex3f(min.get_x(), min.get_y(), min.get_z());
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex3f(min.get_x(), max.get_y(), min.get_z());
	glVertex3f(min.get_x(), max.get_y(), max.get_z());
	glVertex3f(max.get_x(), max.get_y(), max.get_z());
	glVertex3f(max.get_x(), max.get_y(), min.get_z());
	glVertex3f(min.get_x(), max.get_y(), min.get_z());
	glEnd();
}


void	vertex_info::read(SDL_RWops* in)
// Read vert info from the given file.
{
	vertex_count = SDL_ReadLE16(in);
	vertices = new vertex[vertex_count];
	for (int i = 0; i < vertex_count; i++) {
		vertices[i].read(in);
	}
		
	// Load indices.
	index_count = SDL_ReadLE32(in);
	if (index_count > 0) {
		indices = new Uint16[ index_count ];
	} else {
		indices = NULL;
	}
	{for (int i = 0; i < index_count; i++) {
		indices[i] = SDL_ReadLE16(in);
	}}

	// Load the real triangle count, for computing statistics.
	triangle_count = SDL_ReadLE32(in);

//	printf("vertex_info::read() -- vertex_count = %d, index_count = %d\n", vertex_count, index_count);//xxxxxxxx
}


void	lod_chunk::clear()
// Clears the enabled values throughout the subtree.  If this node is
// enabled, then the recursion does not continue to the child nodes,
// since their enabled values should be false.
//
// Do this before calling update().
{
	if (split) {
		split = false;

		// Recurse to children.
		for (int i = 0; i < child_count; i++) {
			children[i]->clear();
		}
	}
}


void	lod_chunk::update(const lod_chunk_tree& base, const vec3& viewpoint)
// Computes 'lod' and split values for this chunk and its subtree,
// based on the given camera parameters and the parameters stored in
// 'base'.  Traverses the tree and forces neighbor chunks to a valid
// LOD, and updates its contained edges to prevent cracks.
//
// !!!  For correct results, the tree must have been clear()ed before
// calling update() !!!
{
	Uint16	desired_lod = base.compute_lod(box_center, box_extent, viewpoint);

	if (child_count > 0 && desired_lod > (lod | 0x0FF))
	{
		do_split(base, viewpoint);

		// Recurse to children.
		for (int i = 0; i < child_count; i++) {
			children[i]->update(base, viewpoint);
		}
	} else {
		// We're good... this chunk can represent its region within the max error tolerance.
		if ((lod & 0xFF00) == 0) {
			// Root chunk -- make sure we have valid morph value.
			lod = iclamp(desired_lod, lod & 0xFF00, lod | 0x0FF);
		}
	}
}


void	lod_chunk::do_split(const lod_chunk_tree& base, const vec3& viewpoint)
// Enable this chunk.  Use the given viewpoint to decide what morph
// level to use.
{
	if (split == false) {
		split = true;

		// Make sure children have a valid lod value.
		{for (int i = 0; i < child_count; i++) {
			lod_chunk*	c = children[i];
			Uint16	desired_lod = base.compute_lod(c->box_center, c->box_extent, viewpoint);
			c->lod = iclamp(desired_lod, c->lod & 0xFF00, c->lod | 0x0FF);
		}}

		// make sure ancestors are split...
		for (lod_chunk* p = parent; p && p->split == false; p = p->parent) {
			p->do_split(base, viewpoint);
		}

		// make sure neighbors are subdivided enough.
		{for (int i = 0; i < 4; i++) {
			lod_chunk*	n = neighbor[i].chunk;
			if (n && n->parent && n->parent->split == false) {
				n->parent->do_split(base, viewpoint);
			}
		}}
	}
}


static void	morph_vertices(float* verts, const vertex_info& morph_verts, const vec3& box_center, const vec3& box_extent, float f)
// Adjust the positions of our morph vertices according to f, the
// given morph parameter.  verts is the output buffer for processed
// verts.
//
// The input is in chunk-local 16-bit signed coordinates.  The given
// box_center/box_extent parameters are used to produce the correct
// world-space coordinates.  The quantizing to 16 bits is a way to
// compress the input data.
//
// @@ This morphing/decompression functionality should be shifted into
// a vertex program for the GPU where possible.
{
	// Do quantization decompression, output floats.

	float	sx = box_extent.get_x() / ((1 << 15) - 1);
	float	sz = box_extent.get_z() / ((1 << 15) - 1);

	int	offsetx = box_center.get_x();
	int	offsetz = box_center.get_z();

	float	one_minus_f = 1.0 - f;

	for (int i = 0; i < morph_verts.vertex_count; i++) {
		const vertex_info::vertex&	v = morph_verts.vertices[i];
		verts[i*3 + 0] = offsetx + v.x[0] * sx;
		verts[i*3 + 1] = (v.x[1] + v.y_delta * one_minus_f) * s_vertical_scale;	// lerp the y value of the vert.
		verts[i*3 + 2] = offsetz + v.x[2] * sz;
	}
#if 0
	// With a vshader, this routine would be replaced by an initial agp_alloc() & memcpy()/memmap().
#endif // 0
}


int	lod_chunk::render(const lod_chunk_tree& c, const view_state& v, cull::result_info cull_info, render_options opt)
// Draws the given lod tree.  Uses the current state stored in the
// tree w/r/t split & LOD level.
//
// Returns the number of triangles rendered.
{
	// Frustum culling.
	if (cull_info.active_planes) {
		cull_info = cull::compute_box_visibility(box_center, box_extent, v.m_frustum, cull_info);
		if (cull_info.culled) {
			// Bounding box is not visible; no need to draw this node or its children.
			return 0;
		}
	}

	int	triangle_count = 0;

	if (split) {
		// Recurse to children.  Some subset of our descendants will be rendered in our stead.
		for (int i = 0; i < child_count; i++) {
			triangle_count += children[i]->render(c, v, cull_info, opt);
		}
	} else {
		if (opt.show_box) {
			// draw bounding box.
			glDisable(GL_TEXTURE_2D);
			float	f = (lod & 255) / 255.0;	//xxx
			glColor3f(f, 1 - f, 0);
			draw_box(box_center - box_extent, box_center + box_extent);
		}

		if (opt.show_geometry || opt.show_edges) {
			glEnable(GL_TEXTURE_2D);

			float	xsize = c.m_root->box_extent.get_x() * 2;
			float	zsize = c.m_root->box_extent.get_x() * 2;

#if 0
			// Set up texgen for this tile.
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			float	p[4] = { 0, 0, 0, 0 };
			p[0] = 1.0f / xsize;
			glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
			p[0] = 0;
			
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			p[2] = 1.0f / zsize;
			glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
			
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
#endif // 0
		}

		// Grab some space to put processed verts.
		assert(s_stream);
		float*	output_verts = (float*) s_stream->reserve_memory(sizeof(float) * 3 * verts.vertex_count);

		// Process our vertices into the output buffer.
		float	f = morph_curve((lod & 255) / 255.0f);
		if (opt.morph == false) {
			f = 0;
		}
		morph_vertices(output_verts, verts, box_center, box_extent, f);

		if (opt.show_geometry) {
			// draw this chunk.
			glColor3f(1, 1, 1);
			glVertexPointer(3, GL_FLOAT, 0, output_verts);
			glDrawElements(GL_TRIANGLE_STRIP, verts.index_count, GL_UNSIGNED_SHORT, verts.indices);
			triangle_count += verts.triangle_count;
		}

		if (opt.show_edges) {
			for (int i = 0; i < 4; i++) {
				triangle_count += render_edge((direction) i, opt);
			}
		}
	}

	return triangle_count;
}


static int	join_rows(vec3* v0, int count0, vec3* v1, int count1);


int	lod_chunk::render_edge(direction dir, render_options opt)
// Draw the ribbon connecting the edge of this chunk to the edge(s) of
// the neighboring chunk(s) in the specified direction.  Returns the
// number of triangles rendered.
{
	int	facing_dir = direction((int(dir) + 2) & 3);

	int	triangle_count = 0;

	//
	// Figure out which of the three cases we have:
	//
	// 1) our neighbor is a chunk at our same LOD (but probably has a
	// different morph value).  We only handle this if this is the
	// EAST or SOUTH edge of our chunk.  Otherwise, the other chunk is
	// responsible for rendering (this prevents double-rendering of
	// the edge.)
	//
	// To render, all we have to do is zip up the corresponding verts.
	//
	//  |           |
	//  |    us     |
	//  +-*-*-*-*-*-+
	//  |/|/|/|/|/|/|  <-- simple zig-zag tri strip
	//  +-*-*-*-*-*-+
	//  |  neighbor |
	//  |           |
	//
	//
	// 2) our neighbors are actually two chunks, at the next higher
	// LOD.  We have to stitch together our verts with the verts of
	// the two neighbors.
	//
	//  |           |
	//  |    us     |
	//  +-*-*-*-*-*-+
	//  |/|\|\|\|/|\|  <-- more complex connecting ribbon
	//  +***********+
	//  |  n0 |  n1 |
	//  |     |     |
	//
	// 3) same as 2, but we're one of the high LOD chunks.  In this
	// case we don't render anything; the lower-LOD chunk is
	// responsible for rendering the edge.

	lod_chunk*	n = neighbor[(int) dir].chunk;
	if (n == NULL) {
		// No neighbor, so no edge to worry about.
		return triangle_count;
	}
	if (n->split == false && n->parent && n->parent->split == true) {
		// two matching edges.
		if (dir == EAST || dir == SOUTH) {
			float	f0 = morph_curve((lod & 255) / 255.0f);
			float	f1 = morph_curve((n->lod & 255) / 255.0f);
			if (opt.morph == false) {
				f0 = f1 = 0;
			}
			int	c0 = edge[dir].lo_vertex_count;
			float*	output_verts = (float*) s_stream->reserve_memory(sizeof(float) * 3 * c0 * 2);
			vertex_info	vi;
			vi.vertex_count = c0;
			vi.vertices = edge[dir].edge_verts;
			morph_vertices(output_verts, vi, box_center, box_extent, f0);
			// Same verts, different morph value.
			morph_vertices(output_verts + c0 * 3, vi, box_center, box_extent, f1);

			// Draw the connecting ribbon.  Just a zig-zag strip between
			// the two edges.
			glVertexPointer(3, GL_FLOAT, 0, output_verts);
			glBegin(GL_TRIANGLE_STRIP);
			for (int i = 0; i < c0; i++) {
				glArrayElement(i);
				glArrayElement(i + c0);
			}
			glEnd();
		
			triangle_count += c0 * 2 - 2;
		}

	} else if (n->split) {
		// we have the low LOD edge; children of our neighbor have the high LOD edges.

		// Find the neighbors we need to mesh with.

		// table indexed by the direction of our big low-LOD edge.
		int	child_index[4][2] = {
			{ 0, 2 },	// EAST
			{ 2, 3 },	// NORTH
			{ 1, 3 },	// WEST
			{ 0, 1 }	// SOUTH
		};
		lod_chunk*	n0 = n->children[child_index[dir][0]];
		lod_chunk*	n1 = n->children[child_index[dir][1]];

		assert(n0);
		assert(n1);

		float	f = morph_curve((lod & 255) / 255.0f);
		float	f0 = morph_curve((n0->lod & 255) / 255.0f);
		float	f1 = morph_curve((n1->lod & 255) / 255.0f);
		if (opt.morph == false) {
			f = f0 = f1 = 0;
		}

		const lod_edge&	e = edge[dir];

		float*	output_verts = (float*) s_stream->reserve_memory(
			sizeof(float) * 3 * (e.lo_vertex_count + e.hi_vertex_count[0] + e.hi_vertex_count[1]));

		vertex_info	vi;
		vi.vertices = e.edge_verts;
		vi.vertex_count = e.lo_vertex_count;
		morph_vertices(output_verts, vi, box_center, box_extent, f);
		vi.vertices = e.edge_verts + e.lo_vertex_count;
		vi.vertex_count = e.hi_vertex_count[0];
		morph_vertices(output_verts + 3 * e.lo_vertex_count, vi, box_center, box_extent, f0);
		vi.vertices = e.edge_verts + e.lo_vertex_count + e.hi_vertex_count[0];
		vi.vertex_count = e.hi_vertex_count[1];
		morph_vertices(output_verts + 3 * (e.lo_vertex_count + e.hi_vertex_count[0]), vi, box_center, box_extent, f1);

		// Draw the connecting ribbon.

		assert(e.ribbon_index_count);
		glVertexPointer(3, GL_FLOAT, 0, output_verts);
		glDrawElements(GL_TRIANGLES, e.ribbon_index_count, GL_UNSIGNED_SHORT, e.ribbon_indices);
		triangle_count += e.ribbon_index_count / 3;

	} else {
		// Our neighbor is responsible for this case.
	}

	return triangle_count;
}


static int	join_rows(vec3* v0, int count0, vec3* v1, int count1)
// Join two rows of vertices with a mesh.  Assumption is that the two
// rows have different numbers of verts, but approximate the same
// curve in space.  They also are assumed to share start & end points.
//
// Return the number of triangles generated.
{
	// @@ is there a reasonable way to strip this, using degenerates?
	int	i0 = 1;
	int	i1 = 1;

	glBegin(GL_TRIANGLES);

	// Beginning triangle.
	glVertex3fv(v0[0]);
	glVertex3fv(v0[1]);
	glVertex3fv(v1[1]);

	while (i0 < count0 - 1 && i1 < count1 - 1) {
		glVertex3fv(v0[i0]);
		glVertex3fv(v1[i1]);
		if (i0 * count1 < i1 * count0) {	// @@ There's probably a slick DDA-ish way to do this.
			// row 0 is behind, so use one of its verts.
			i0++;
			glVertex3fv(v0[i0]);
		} else {
			// row 1 is behind, so use one of its verts.
			i1++;
			glVertex3fv(v1[i1]);
		}
	}
	glEnd();

	return count0 + count1 - 4;
}


void	lod_chunk::read(SDL_RWops* in, int level, lod_chunk_tree* tree)
// Read chunk data from the given file and initialize this chunk with it.
// Recursively loads child chunks for level > 0.
{
	split = false;

	// Get this chunk's label, and add it to the table.
	int	chunk_label = SDL_ReadLE32(in);
	assert_else(chunk_label < tree->m_chunk_count) {
		printf("invalid chunk_label: %d, level = %d\n", chunk_label, level);
		exit(1);
	}
	assert(tree->m_chunk_table[chunk_label] == 0);
	tree->m_chunk_table[chunk_label] = this;

	// Initialize neighbor links.
	{for (int i = 0; i < 4; i++) {
		neighbor[i].label = SDL_ReadLE32(in);
	}}

	// Initialize our bounding box.
	box_center.read(in);
	box_extent.read(in);

	// Load the main chunk data.
	verts.read(in);

	// Read the edge data.
	{for (int i = 0; i < 4; i++) {
		edge[i].read(in);
	}}

	child_count = 0;

	// Recurse to child chunks.
	if (level > 0) {
		child_count = 4;
		children = new lod_chunk*[4];
		for (int i = 0; i < 4; i++) {
			children[i] = new lod_chunk;
			children[i]->lod = lod + 0x100;
			children[i]->parent = this;
			children[i]->read(in, level-1, tree);
		}
	} else {
		child_count = 0;
		children = NULL;
	}
}


void	lod_chunk::lookup_neighbors(lod_chunk_tree* tree)
// Convert our neighbor labels to neighbor pointers.  Recurse to child chunks.
{
	for (int i = 0; i < 4; i++) {
		assert_else(neighbor[i].label >= -1 && neighbor[i].label < tree->m_chunk_count) {
			neighbor[i].label = -1;
		}
		if (neighbor[i].label == -1) {
			neighbor[i].chunk = NULL;
		} else {
			neighbor[i].chunk = tree->m_chunk_table[neighbor[i].label];
		}
	}

	{for (int i = 0; i < child_count; i++) {
		children[i]->lookup_neighbors(tree);
	}}
}



//
// lod_chunk_tree implementation.  lod_chunk_tree is the external
// interface to chunked LOD.
//


lod_chunk_tree::lod_chunk_tree(SDL_RWops* src)
// Construct and initialize a tree of LOD chunks, using data from the given
// source.  Uses a special .chu file format which is a pretty direct
// encoding of the chunk data.
{
	// Read and verify a "CHU\0" header tag.
	Uint32	tag = SDL_ReadLE32(src);
	if (tag != (('C') | ('H' << 8) | ('U' << 16))) {
		throw "Input file is not in .CHU format";
	}

	int	format_version = SDL_ReadLE16(src);
	assert_else(format_version == 4) {
		throw "Input format has non-matching version number";
	}

	m_tree_depth = SDL_ReadLE16(src);
	m_error_LODmax = ReadFloat32(src);
	m_vertical_scale = ReadFloat32(src);
	m_chunk_count = SDL_ReadLE32(src);

	// Create a lookup table of chunk labels.
	m_chunk_table = new lod_chunk*[m_chunk_count];
	memset(m_chunk_table, 0, sizeof(m_chunk_table[0]) * m_chunk_count);

	// Compute a sane default value for distance_LODmax, in case the client code
	// neglects to call set_parameters().
	set_parameters(5.0f, 640.0f, 90.0f);

	// Load the chunk tree.
	m_root = new lod_chunk;
	m_root->lod = 0;
	m_root->parent = 0;
	m_root->read(src, m_tree_depth-1, this);
	m_root->lookup_neighbors(this);
}


void	lod_chunk_tree::update(const vec3& viewpoint)
// Initializes tree state, so it can be rendered.  The given viewpoint
// is used to do distance-based LOD switching on our contained chunks.
{
	m_root->clear();
	m_root->update(*this, viewpoint);
}


int	lod_chunk_tree::render(const view_state& v, render_options opt)
// Displays our model, using the LOD state computed during the last
// call to update().
//
// Returns the number of triangles rendered.
{
	// Make sure we have a vertex stream.
	if (s_stream == NULL) {
		s_stream = new ogl::vertex_stream(2 << 20);
	}

	int	triangle_count = 0;

//	glPushMatrix();
//	glScalef(1.0, m_vertical_scale, 1.0);	// scale the vertical axis.  Input is 16-bit integers; transform to world coords.
	s_vertical_scale = m_vertical_scale;
	{
		triangle_count = m_root->render(*this, v, cull::result_info(), opt);
	}
//	glPopMatrix();

	return triangle_count;
}


void	lod_chunk_tree::get_bounding_box(vec3* box_center, vec3* box_extent)
// Returns the bounding box of the data in this chunk tree.
{
	assert(m_root);
	*box_center = m_root->box_center;
	*box_extent = m_root->box_extent;
}


void	lod_chunk_tree::set_parameters(float max_pixel_error, float screen_width_pixels, float horizontal_FOV_degrees)
// Initializes some internal parameters which are used to compute
// which chunks are split during update().
//
// Given a screen width and horizontal field of view, the
// lod_chunk_tree when properly updated guarantees a screen-space
// vertex error of no more than max_pixel_error (at the center of the
// viewport) when rendered.
{
	const float	tan_half_FOV = tan(0.5 * horizontal_FOV_degrees * M_PI / 180.0f);

	// distance_LODmax is the distance below which we need to be
	// at the maximum LOD.  It's used in compute_lod(), which is
	// called by the chunks during update().
	m_distance_LODmax = m_error_LODmax * screen_width_pixels / (tan_half_FOV * max_pixel_error);
}


Uint16	lod_chunk_tree::compute_lod(const vec3& center, const vec3& extent, const vec3& viewpoint) const
// Given an AABB and the viewpoint, this function computes a desired
// LOD level, based on the distance from the viewpoint to the nearest
// point on the box.  So, desired LOD is purely a function of
// distance and the chunk tree parameters.
{
	vec3	disp = viewpoint - center;
	disp.set(0, fmax(0, fabs(disp.get(0)) - extent.get(0)));
	disp.set(1, fmax(0, fabs(disp.get(1)) - extent.get(1)));
	disp.set(2, fmax(0, fabs(disp.get(2)) - extent.get(2)));

//	disp.set(1, 0);	//xxxxxxx just do calc in 2D, for debugging

	float	d = 0;
	d = sqrt(disp * disp);

	return iclamp(((m_tree_depth << 8) - 1) - int(log2(fmax(1, d / m_distance_LODmax)) * 256), 0, 0x0FFFF);
}

