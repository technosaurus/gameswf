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


struct vertex_info {
// Structure for storing vertex-buffer mesh info.
	int	vertex_count;
	vec3*	vertices;

	int	index_count;
	Uint16*	indices;

	// mesh type (GL_TRIANGLE_STRIP vs GL_TRIANGLES)
	// texture id
	// other?

	void	load(SDL_RWops* in);
	void	render();
};


struct morph_info {
// Structure for storing info about vertices which need to be morphed.
// This info includes vertex indices that refer to an external
// vertex_info struct.
	int	vertex_count;
	struct vertex {
		Uint16	index;
		float	y0, delta;
	};
	vertex*	vertices;

	void	load(SDL_RWops* in);
};


struct lod_edge;
struct view_info;

struct lod_chunk {
	lod_chunk*	parent;
	Uint16	lod;	// LOD of this chunk.  high byte never changes; low byte is the morph parameter.
	bool	enabled;	// true if this node should be rendered
	bool	tree_enabled;	// true if any descendent is enabled

	// These are the edges we directly border on.
	int	edge_count;
	lod_edge**	edges;

	int	child_count;
	lod_chunk**	children;

	int	neighbor_count;
	lod_chunk**	neighbors;

//	// child edges are the edges *internal* to this chunk;
//	// i.e. the edges between our child chunks.
//	int	child_edge_count;
//	lod_edge**	child_edges;

	// AABB, used for deciding when to enable.
	vec3	box_center;
	vec3	box_extent;
	
	vertex_info	verts;	// actual rendering info; vertex array, indices, texture id
	morph_info	morph_verts;	// extra info for CPU morphing of verts

//methods:
	// needs a destructor!

	void	clear(int level);
	void	update(const lod_chunk_tree& c, const vec3& viewpoint);
	void	force_parent_enabled(const vec3& viewpoint);
	int	render(const view_state& v, cull::result_info cull_info, render_options opt);

	void	load(SDL_RWops* in, int level, lod_chunk_tree* tree);
	void	compute_bounding_box();

	void	add_edge(lod_edge* e);
};


void	lod_chunk::add_edge(lod_edge* e)
// Adds the given edge to our collection; the given edge should fill
// the crack between this chunk and a neighbor.  When we render
// ourself, we'll call ->render() on this edge.
{
	edge_count++;
	if (edges) {
		edges = (lod_edge**) realloc(edges, sizeof(lod_edge*) * edge_count);
	} else {
		edges = (lod_edge**) malloc(sizeof(lod_edge*) * edge_count);
	}
	edges[edge_count-1] = e;
}


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


void	vertex_info::load(SDL_RWops* in)
// Read vert info from the given file.
{
	// Load verts.
	vertex_count = SDL_ReadLE16(in);
	vertices = (vec3*) ogl::allocate_vertex_memory( vertex_count * sizeof(vec3) );
	for (int i = 0; i < vertex_count; i++) {
		vertices[i].set(0, ReadFloat32(in));
		vertices[i].set(1, ReadFloat32(in));
		vertices[i].set(2, ReadFloat32(in));
	}
		
	// Load indices.
	index_count = SDL_ReadLE32(in);
	indices = new Uint16[ index_count ];
	{for (int i = 0; i < index_count; i++) {
		indices[i] = SDL_ReadLE16(in);
	}}

//	printf("vertex_info::load() -- vertex_count = %d, index_count = %d\n", vertex_count, index_count);//xxxxxxxx
}


void	morph_info::load(SDL_RWops* in)
// Load info about a set of morph vertices.
{
	vertex_count = SDL_ReadLE16(in);
	vertices = new vertex[ vertex_count ];
	for (int i = 0; i < vertex_count; i++) {
		vertices[i].index = SDL_ReadLE16(in);
		vertices[i].y0 = ReadFloat32(in);
		vertices[i].delta = ReadFloat32(in);
	}

//	printf("morph_info::load() -- vertex_count = %d\n", vertex_count);//xxxxxxxx
}


void	vertex_info::render()
// Render this vertex buffer.
{
	glVertexPointer(3, GL_FLOAT, 0, (float*) &(vertices[0]));
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, indices);

//	glBegin(GL_TRIANGLES);
//	for (int i = 0; i < index_count; i++) {
//		glVertex3fv((float*) &(vertices[indices[i]]));
//	}
//	glEnd();
}


struct lod_edge {
	lod_edge*	child[2];

	lod_chunk*	neighbor[2];	// pointers to neighboring chunks.

	int	midpoint_index;	// Index of the vert which is the midpoint of the edge.
	int	vertex_count;	// Number of verts along this edge.
	vec3*	verts;	// Two copies of the verts along this edge.  array size == vertex_count * 2
	morph_info	morph_verts;
	Uint16*	connecting_strip;	// Simple strip for connecting two copies of the edge (which are morphed differently).
	int	rendered_frame;

//methods:
	void	load(SDL_RWops* in, lod_chunk_tree* tree);
	int	render(const view_state& v, render_options opt);
	void	clear();
};


void	lod_chunk::clear(int level)
// Clears lod_fraction and enabled values throughout our subtree.
// Do this before doing update().
{
	enabled = false;
	tree_enabled = false;

	if (level > 0) {
		for (int i = 0; i < child_count; i++) {
			children[i]->clear(level - 1);
		}
	}
		
	for (int i = 0; i < edge_count; i++) {
		edges[i]->clear();
	}
}


void	lod_edge::clear()
// Reset, before rendering.
{
	rendered_frame = 0;

#if 0
	// recurse to children.
	for (int i = 0; i < 2; i++) {
		if (child[i]) {
			child[i]->clear();
		}
	}
#endif // 0
}


void	lod_chunk::update(const lod_chunk_tree& base, const vec3& viewpoint)
// Computes 'lod' and 'enabled' values for this chunk and its
// subtree, based on the given camera parameters and the parameters
// stored in 'base'.  Traverses the tree and forces neighbor
// chunks to a valid LOD, and updates its contained edges to prevent
// cracks.
//
// !!!  For correct results, the tree must have been clear()ed before
// calling update() !!!
{
	tree_enabled = true;

	Uint16	desired_lod = base.compute_lod(box_center, box_extent, viewpoint);
//	printf("desired_lod = %x, lod = %x\n", desired_lod, lod);//xxxxx

	if (desired_lod <= (lod | 0x0FF) || child_count == 0) {
		// We're good... this chunk can represent its region within the max error tolerance.
		enabled = true;
		tree_enabled = true;

		lod = iclamp(desired_lod, lod & 0xFF00, lod | 0x0FF);
		
		// make sure neighbors are subdivided enough.
		for (int i = 0; i < neighbor_count; i++) {
			neighbors[i]->force_parent_enabled(viewpoint);
		}

		// make sure ancestors aren't enabled...
		for (lod_chunk* p = parent; p; p = p->parent) {
			p->enabled = false;
			p->tree_enabled = true;
		}
	} else {
		// Recurse to children.
		for (int i = 0; i < child_count; i++) {
			children[i]->update(base, viewpoint);
		}
	}
}


void	lod_chunk::force_parent_enabled(const vec3& viewpoint)
// Forces our parent chunk to be enabled if it isn't already.  Computes
// the LOD parameter according to the distance from the viewpoint.
{
	// xxxxx
}


static void	morph_vertices(vec3* verts, const morph_info& morph_verts, float f)
// Adjust the positions of our morph vertices according to f, the
// given morph parameter.  verts contains the vertex buffer info to be
// adjusted.  morph_verts contains the information about which verts
// to adjust, and their deltas.
{
	for (int i = 0; i < morph_verts.vertex_count; i++) {
		const morph_info::vertex&	v = morph_verts.vertices[i];
		verts[v.index].set(1, v.y0 + v.delta * f);	// lerp the y value of the vert.
	}
}


int	lod_chunk::render(const view_state& v, cull::result_info cull_info, render_options opt)
// Draws the given lod tree.  Uses the current state stored in the
// tree w/r/t enabled & LOD level.
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

	if (enabled) {
		if (opt.morph) {
			// morph our morph verts.
			float	f = (lod & 255) / 255.0f;
			morph_vertices(verts.vertices, morph_verts, f);
		}

		if (opt.show_box) {
			// draw bounding box.
			glColor3f(0, 1, 0);
			draw_box(box_center - box_extent, box_center + box_extent);
		}

		if (opt.show_geometry) {
			// draw this chunk.
			glColor3f(1, 1, 1);
			verts.render();
			triangle_count += verts.index_count / 3;
		}

		if (opt.show_edges) {
			for (int i = 0; i < edge_count; i++) {
				triangle_count += edges[i]->render(v, opt);
			}
		}
	} else {
		// recurse to children.  some subset of our descendants will be rendered in our stead.
		for (int i = 0; i < child_count; i++) {
			triangle_count += children[i]->render(v, cull_info, opt);
		}
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


int	lod_edge::render(const view_state& v, render_options opt)
// Draw the edge, according to the enabled & LOD info stored in the
// given tree.
// 
// Returns the number of triangles rendered.
//
// The given frame number is used as a guard to prevent rendering edges more than once
// in a single rendering pass.
{
	// Avoid rendering more than once in a single pass.
	if (rendered_frame == v.m_frame_number) {
		return 0;
	}
	rendered_frame = v.m_frame_number;

	int	triangle_count = 0;
	if (neighbor[0]->enabled && neighbor[1]->enabled) {
		// joining two chunks at the same level.
		if (opt.morph) {
			morph_vertices(&verts[0], morph_verts, (neighbor[0]->lod & 255) / 255.0f);	// First copy of verts gets neighbor[0]'s morph.
			morph_vertices(&verts[vertex_count], morph_verts, (neighbor[1]->lod & 255) / 255.0f);	// Second copy of verts gets neighbor[1]'s morph.
		}

		// Use a precomputed strip to zip these edges together.
		glVertexPointer(3, GL_FLOAT, 0, verts[0]);
		glDrawElements(GL_TRIANGLE_STRIP, (vertex_count - 1) * 2, GL_UNSIGNED_SHORT, connecting_strip);

#if 0
		// Make a strip that joins the two rows of matched verts.
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < vertex_count - 1; i++) {
			glVertex3fv(verts[i]);
			glVertex3fv(verts[vertex_count + i + 1]);
		}
		glEnd();
#endif // 0

		triangle_count += 2 * (vertex_count - 1) - 2;
	}
	else if (neighbor[0]->enabled && child[0] && child[1] && child[0]->neighbor[1]->enabled)
	{
		// This is a t-junction.  neighbor[0] has the low LOD, while child[0]->neighbor[1] and
		// child[1]->neighbor[1] have the matching high LOD edges.
		if (opt.morph) {
			morph_vertices(&verts[0], morph_verts, (neighbor[0]->lod & 255) / 255.0f);
			morph_vertices(&(child[0]->verts[0]), child[0]->morph_verts, (child[0]->neighbor[1]->lod & 255) / 255.0f);
			morph_vertices(&(child[1]->verts[0]), child[1]->morph_verts, (child[1]->neighbor[1]->lod & 255) / 255.0f);
		}

		// generate weirdo mesh to join this stuff.  Would be nicer
		// to precompute this strip and make sure it makes some sense.

//		int	midway_index = vertex_count / 2;	// should precompute this for real.
		triangle_count += join_rows(&verts[0], midpoint_index + 1, &(child[0]->verts[0]), child[0]->vertex_count);
		triangle_count += join_rows(&verts[midpoint_index], vertex_count - midpoint_index, &(child[1]->verts[0]), child[1]->vertex_count);
	}
	else if (neighbor[1]->enabled && child[0] && child[1] && child[0]->neighbor[0]->enabled)
	{
		// This is a t-junction.  neighbor[0] has the low LOD, while child[0]->neighbor[1] and
		// child[1]->neighbor[1] have the matching high LOD edges.
		if (opt.morph) {
			morph_vertices(&verts[0], morph_verts, (neighbor[1]->lod & 255) / 255.0f);
			morph_vertices(&(child[0]->verts[0]), child[0]->morph_verts, (child[0]->neighbor[0]->lod & 255) / 255.0f);
			morph_vertices(&(child[1]->verts[0]), child[1]->morph_verts, (child[1]->neighbor[0]->lod & 255) / 255.0f);
		}

		// generate weirdo mesh to join this stuff.  Would be nicer
		// to precompute this strip and make sure it makes some sense.

//		int	midway_index = vertex_count / 2;	// should precompute this for real.
		triangle_count += join_rows(&verts[0], midpoint_index + 1, &(child[0]->verts[0]), child[0]->vertex_count);
		triangle_count += join_rows(&verts[midpoint_index], vertex_count - midpoint_index, &(child[1]->verts[0]), child[1]->vertex_count);
	}

	return triangle_count;
}


void	lod_chunk::load(SDL_RWops* in, int level, lod_chunk_tree* tree)
// Read chunk data from the given file and initialize this chunk with it.
// Recursively loads child chunks for level > 0.
{
	// Get this chunk's label, and add it to the table.
	int	chunk_label = SDL_ReadLE32(in);
	assert_else(chunk_label < tree->chunk_count) {
		throw "invalid chunk label";
	}
	assert(tree->chunk_table[chunk_label] == 0);
	tree->chunk_table[chunk_label] = this;

	// Load the data.
	verts.load(in);
	morph_verts.load(in);

	neighbor_count = 0;
	child_count = 0;
	edge_count = 0;
	edges = 0;
//	child_edge_count = 0;

	// Recurse to child chunks.
	if (level > 0) {
		child_count = 4;
		children = new lod_chunk*[4];
		for (int i = 0; i < 4; i++) {
			children[i] = new lod_chunk;
			children[i]->lod = lod + 0x100;
			children[i]->parent = this;
			children[i]->load(in, level-1, tree);
		}
	}

	// Load data for internal edges.
	int	ct = ReadByte(in);
	for (int i = 0; i < ct; i++) {
		lod_edge*	e = new lod_edge;
		e->load(in, tree);
	}
}


void	lod_edge::load(SDL_RWops* in, lod_chunk_tree* tree)
// Initialize this edge from the given input file.  Also creates and
// initializes all child edges of this edge.  This edge is an internal
// edge of the given parent, and should only border on chunks which
// are descendants of *parent.  *tree contains a lookup table for
// getting the pointers to the chunks associated with an edge.  This
// function modifies the chunks referenced by the edges, by adding the
// edges to the chunks.
{
	rendered_frame = 0;

	// Get pointers to the two neighbor chunks.
	for (int i = 0; i < 2; i++) {
		int	label = SDL_ReadLE32(in);
		assert_else(label >= 0 && label < tree->chunk_count && tree->chunk_table[label] != NULL) {
			throw "can't load edge data; refers to undefined chunk.";
		}
		neighbor[i] = tree->chunk_table[label];

		// Attach ourselves to each neighbor.
		neighbor[i]->add_edge(this);
	}

	// Load vertices.
	midpoint_index = SDL_ReadLE16(in);
	vertex_count = SDL_ReadLE16(in);
	verts = (vec3*) ogl::allocate_vertex_memory(vertex_count * sizeof(vec3) * 2);	// 2 copies of the vert data!!
	{for (int i = 0; i < vertex_count; i++) {
		verts[i].set(0, ReadFloat32(in));
		verts[i].set(1, ReadFloat32(in));
		verts[i].set(2, ReadFloat32(in));

		// Copy this data to the second set of verts.
		verts[vertex_count + i] = verts[i];
	}}

	morph_verts.load(in);

	// Initialize the indices of a connecting strip, to use when
	// connecting the two differently-morphed versions of this edge.
	// It's just a plain zig-zag.
	connecting_strip = new Uint16[(vertex_count - 1) * 2];
	{for (int i = 0, index = 0; i < vertex_count - 1; i++) {
		connecting_strip[index++] = i;
		connecting_strip[index++] = vertex_count + i + 1;
	}}

	// Initialize & load child edges.
	bool	load_children = ReadByte(in);
	if (load_children) {
		for (int i = 0; i < 2; i++) {
			child[i] = new lod_edge;
			child[i]->load(in, tree);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			child[i] = NULL;
		}
	}
}

	
void	lod_chunk::compute_bounding_box()
// Computes the dimensions of our AABB based on our vertex info as well as
// our child nodes, if any.
{
	// Initialize an AABB based on the vertex info.  !!! should precompute this and store it in the file !!!
	vec3	Min(1000000, 1000000, 1000000),
		Max(-1000000, -1000000, -1000000);

	// Check vertices for Min/Max coords.
	for (int i = 0; i < verts.vertex_count; i++) {
		for (int j = 0; j < 3; j++) {
			float	f = verts.vertices[i].get(j);
			if (f < Min.get(j)) Min.set(j, f);
			if (f > Max.get(j)) Max.set(j, f);
		}
	}

	// Check morph verts for additional y Min/Max.
	{for (int i = 0; i < morph_verts.vertex_count; i++) {
		float	f = morph_verts.vertices[i].y0;
		if (f < Min.get(1)) Min.set(1, f);
		if (f > Max.get(1)) Max.set(1, f);

		f += morph_verts.vertices[i].delta;
		if (f < Min.get(1)) Min.set(1, f);
		if (f > Max.get(1)) Max.set(1, f);
	}}

	// Compute, and check, bounds of child chunks.
	{for (int i = 0; i < child_count; i++) {
		children[i]->compute_bounding_box();

		vec3	child_Min = children[i]->box_center - children[i]->box_extent;
		vec3	child_Max = children[i]->box_center + children[i]->box_extent;
		for (int j = 0; j < 3; j++) {
			float	f = child_Min.get(j);
			if (f < Min.get(j)) Min.set(j, f);

			f = child_Max.get(j);
			if (f > Max.get(j)) Max.set(j, f);
		}
	}}

	// Compute AABB center & extent based on Min/Max.
	box_center = (Max + Min) * 0.5;
	box_extent = (Max - Min) * 0.5;
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
		assert(0);
		throw "Input file is not in .CHU format";
	}

	int	format_version = SDL_ReadLE16(src);
	assert_else(format_version == 2) {
		throw "Input format has non-matching version number";
	}

	tree_depth = SDL_ReadLE16(src);
	error_LODmax = ReadFloat32(src);
	chunk_count = SDL_ReadLE32(src);

	// Create a lookup table of chunk labels.
	chunk_table = new lod_chunk*[chunk_count];
	memset(chunk_table, 0, sizeof(chunk_table[0]) * chunk_count);

	// Compute a sane default value for distance_LODmax, in case the client code
	// neglects to call set_parameters().
	set_parameters(5.0f, 640.0f, 90.0f);

	// Load the chunk tree.
	root = new lod_chunk;
	root->lod = 0;
	root->parent = 0;
	root->load(src, tree_depth-1, this);

	root->compute_bounding_box();
}


void	lod_chunk_tree::update(const vec3& viewpoint)
// Initializes tree state, so it can be rendered.  The given viewpoint
// is used to do distance-based LOD switching on our contained chunks.
{
	root->clear(tree_depth - 1);
	root->update(*this, viewpoint);
}


int	lod_chunk_tree::render(const view_state& v, render_options opt)
// Displays our model, using the LOD state computed during the last
// call to update().
//
// Returns the number of triangles rendered.
{
	int	triangle_count = 0;

//	glEnableClientState(GL_VERTEX_ARRAY);

	triangle_count = root->render(v, cull::result_info(), opt);

//	glDisableClientState(GL_VERTEX_ARRAY);

	return triangle_count;
}


void	lod_chunk_tree::set_parameters(float max_pixel_error, float screen_width_pixels, float horizontal_FOV_degrees)
// Initializes some internal parameters which are used to compute
// which chunks are enabled during update().
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
	distance_LODmax = error_LODmax * screen_width_pixels / (tan_half_FOV * max_pixel_error);
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

//	printf("c_lod(): d = %f, dLm = %f, vx = %f vy = %f vz = %f\n", d, distance_LODmax, viewpoint.get_x(), viewpoint.get_y(), viewpoint.get_z());//xxxxxxxx

	return iclamp(((tree_depth << 8) - 1) - int(log2(fmax(1, d / distance_LODmax)) * 256), 0, 0x0FFFF);
}

