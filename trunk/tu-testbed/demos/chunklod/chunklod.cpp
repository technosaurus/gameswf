// chunklod.cpp	-- tu@tulrich.com Copyright 2001 by Thatcher Ulrich

// hardware-friendly chunked LOD.  chunks are aware of edges shared by
// their neighbors and fill appropriately.  chunks are organized in a
// tree, with each successive level having half the max vertex error
// of its parent.  collapsed vertices in each chunk are morphed
// according to the view distance of the chunk.  morphing is just
// simple lerping and can be done by the CPU, or with a vertex shader
// (at the expense of doubling the size of the vertex data in general,
// or upping it to 4 coords/vert for heightfield terrain, where vertex
// morphing is always vertical).


#include <stdlib.h>

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

	// child edges are the edges *internal* to this chunk;
	// i.e. the edges between our child chunks.
	int	child_edge_count;
	lod_edge**	child_edges;

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
	void	morph_vertices(vertex_info* verts, const morph_info& morph_verts, float f);
	int	render(const plane_info frustum[6], cull::result_info cull_info, render_options opt);

	void	load(SDL_RWops* in, int level);
	void	compute_bounding_box();
};


struct lod_edge {
	lod_edge*	parent;
	lod_edge*	children[2];

	lod_chunk*	neighbors[2];	// pointers to neighboring chunks.

	vec3*	verts;	// two copies of LOD verts, one copy of LOD-1 verts
	morph_info	morph_verts[4];	// info for CPU morphing the edge verts

//methods:
	int	render();
	void	clear();
};


static void	draw_box(const vec3& min, const vec3& max)
// Draw the specified axis-aligned box.
{
	glColor3f(0, 1, 0);
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
	glColor3f(1, 1, 1);

	glVertexPointer(3, GL_FLOAT, 0, (float*) &(vertices[0]));
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, indices);

//	glBegin(GL_TRIANGLES);
//	for (int i = 0; i < index_count; i++) {
//		glVertex3fv((float*) &(vertices[indices[i]]));
//	}
//	glEnd();
}


void	lod_chunk::clear(int level)
// Clears lod_fraction and enabled values throughout our subtree.
// Do this before doing update().
{
//	lod = level << 8;
	enabled = false;
	tree_enabled = false;

	if (level > 0) {
		for (int i = 0; i < child_count; i++) {
			children[i]->clear(level - 1);
		}
		
		{for (int i = 0; i < child_edge_count; i++) {
			child_edges[i]->clear();
		}}
	}
}


void	lod_edge::clear()
// Clears lod_fraction and enabled state throughout our subtree.
// Called by the lod_chunk::clear().
{
//	lod_fraction = 255;
//	enabled = false;

	// recurse to children.
	for (int i = 0; i < 2; i++) {
		if (children[i]) {
			children[i]->clear();
		}
	}
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


void	lod_chunk::morph_vertices(vertex_info* verts, const morph_info& morph_verts, float f)
// Adjust the positions of our morph vertices according to f, the
// given morph parameter.  verts contains the vertex buffer info to be
// adjusted.  morph_verts contains the information about which verts
// to adjust, and their deltas.
{
	for (int i = 0; i < morph_verts.vertex_count; i++) {
		const morph_info::vertex&	v = morph_verts.vertices[i];
		verts->vertices[v.index].set(1, v.y0 + v.delta * f);	// lerp the y value of the vert.
	}
}


int	lod_chunk::render(const plane_info frustum[6], cull::result_info cull_info, render_options opt)
// Draws the given lod tree.  Uses the current state stored in the
// tree w/r/t enabled & LOD level.
//
// Returns the number of triangles rendered.
{
//	// Frustum culling.
//	if (cull_info.active_planes) {
//		cull_info = compute_box_visibility(box_center, box_extent, frustum, cull_info);
//		if (cull_info.culled) {
//			// Bounding box is not visible; no need to draw this node or its children.
//			return;
//		}
//	}

	int	triangle_count = 0;

	if (enabled) {
		if (opt.morph) {
			// morph our morph verts.
			float	f = (lod & 255) / 255.0f;
			morph_vertices(&verts, morph_verts, f);
		}

		if (opt.show_box) {
			// draw bounding box.
			draw_box(box_center - box_extent, box_center + box_extent);
		}

		if (opt.show_geometry) {
			// draw this chunk.
			verts.render();
			triangle_count += verts.index_count / 3;
		}
	} else {
		// recurse to children.  some subset of our descendants will be rendered in our stead.
		for (int i = 0; i < child_count; i++) {
			triangle_count += children[i]->render(frustum, cull_info, opt);
		}

		// recurse to child edges, to make sure the cracks get filled between our child chunks.
		{for (int i = 0; i < child_edge_count; i++) {
			triangle_count += child_edges[i]->render();
		}}
	}

	return triangle_count;
}


int	lod_edge::render()
// Draw the edge, according to the enabled & LOD info stored in the
// given tree.
// 
// Returns the number of triangles rendered.
{
	int	triangle_count = 0;
/*
	if (either neighbor enabled) {
		if (t_junction) {
			// joining three chunks; two at finer LOD and one at current LOD
			morph our three sets of verts;
			render_verts(v, verts, t_junction_mesh);
			triangle_count += something;
		} else {
			// joining two chunks at the same level.
			morph our morph verts;
			render_verts(v, verts, plain_junction_mesh);
			triangle_count += something;
		}
	} else {
		// recurse to child edges.  some subset of our descendents will be rendered.
		for (int i = 0; i < 2; i++) {
			if (children[i]) {
				triangle_count += children[i]->render();
			}
		}
	}
*/
	return triangle_count;
}


void	lod_chunk::load(SDL_RWops* in, int level)
// Read chunk data from the given file and initialize this chunk with it.
// Recursively loads child chunks for level > 0.
{
	verts.load(in);
	morph_verts.load(in);

	neighbor_count = 0;
	child_count = 0;
	edge_count = 0;
	child_edge_count = 0;

	if (level > 0) {
		child_count = 4;
		children = new lod_chunk*[4];
		for (int i = 0; i < 4; i++) {
			children[i] = new lod_chunk;
			children[i]->lod = lod + 0x100;
			children[i]->parent = this;
			children[i]->load(in, level-1);
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
	// xxx read some kind of header ID or something.

	int	format_version = SDL_ReadLE16(src);

	tree_depth = SDL_ReadLE16(src);
	error_LODmax = ReadFloat32(src);

	// Compute a sane default value for distance_LODmax, in case the client code
	// neglects to call set_parameters().
	set_parameters(5.0f, 640.0f, 90.0f);

	// Load the chunk tree.
	root = new lod_chunk;
	root->lod = 0;
	root->parent = 0;
	root->load(src, tree_depth-1);

	root->compute_bounding_box();
}


void	lod_chunk_tree::update(const vec3& viewpoint)
// Initializes tree state, so it can be rendered.  The given viewpoint
// is used to do distance-based LOD switching on our contained chunks.
{
	root->clear(tree_depth - 1);
	root->update(*this, viewpoint);
}


int	lod_chunk_tree::render(const plane_info frustum[6], render_options opt)
// Displays our model, using the LOD state computed during the last
// call to update().
//
// Returns the number of triangles rendered.
{
	int	triangle_count = 0;

//	glEnableClientState(GL_VERTEX_ARRAY);

	triangle_count = root->render(frustum, cull::result_info(), opt);

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

	disp.set(1, 0);	//xxxxxxx just do calc in 2D, for debugging

	float	d = 0;
	d = sqrt(disp * disp);

//	printf("c_lod(): d = %f, dLm = %f, vx = %f vy = %f vz = %f\n", d, distance_LODmax, viewpoint.get_x(), viewpoint.get_y(), viewpoint.get_z());//xxxxxxxx

	return iclamp(((tree_depth << 8) - 1) - int(log2(fmax(1, d / distance_LODmax)) * 256), 0, 0x0FFFF);
}

