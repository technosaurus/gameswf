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
#include <engine/tqt.h>
#include "SDL/SDL_thread.h"

#include "chunklod.h"


#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI


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

//			// xxxxxx TEST QUANTIZATION xxxxxxx
//			// lose the bottom 8 bits by rounding
//			y_delta = (Sint16) iclamp((y_delta + 128) & ~0x0FF, -32768, 32767);
//			// xxxxxx TEST QUANTIZATION xxxxxxx
		}
	};
	vertex*	vertices;

	int	index_count;
	Uint16*	indices;

	int	triangle_count;	// for statistics.

//code:
	vertex_info()
		: vertex_count(0),
		  vertices(0),
		  index_count(0),
		  indices(0),
		  triangle_count(0)
	{
	}

	void	read(SDL_RWops* in);

	~vertex_info()
	{
		if (vertices) {
			delete [] vertices;
			vertices = NULL;
		}
		if (indices) {
			delete [] indices;
			indices = NULL;
		}
	}
};


struct lod_chunk;


// Vertex/mesh data for a chunk.  Can get paged in/out on demand.
struct lod_chunk_data {
	vertex_info	m_verts;	// vertex and mesh info; vertex array w/ morph targets, indices, texture id
	unsigned int	m_texture_id;		// OpenGL texture id for this chunk's texture map.

	//	lod_chunk_data* m_next_data;
	//	lod_chunk_data* m_prev_data;


	lod_chunk_data(SDL_RWops* in, unsigned int texture_id)
	// Constructor.  Read our data & set our texture id.
		: m_texture_id(texture_id)
	{
		// Load the main chunk data.
		m_verts.read(in);
	}


	~lod_chunk_data()
	// Destructor.  Release our texture.
	{
		// @@ should put this texture id on a free list, to be
		// reused for a new texture via glCopyTexSubImage.
		if (m_texture_id) {
			glDeleteTextures(1, &m_texture_id);
			m_texture_id = 0;
		}
	}

	int	render(const lod_chunk_tree& c, const lod_chunk& chunk, const view_state& v, cull::result_info cull_info, render_options opt,
		       const vec3& box_center, const vec3& box_extent);
};


struct lod_chunk {
//data:
	lod_chunk*	m_parent;
	lod_chunk*	m_children[4];

	union {
		int	m_label;
		lod_chunk*	m_chunk;
	} m_neighbor[4];

	// Chunk "address" (its position in the quadtree).
	Uint16	m_x, m_z;
	Uint8	m_level;

	bool	m_split;	// true if this node should be rendered by descendents.  @@ pack this somewhere as a bitflag.  LSB of lod?
	Uint16	lod;	// LOD of this chunk.  high byte never changes; low byte is the morph parameter.

	// Vertical bounds, for constructing bounding box.
	Sint16	m_min_y, m_max_y;

	long	m_data_file_position;
	lod_chunk_data*	m_data;

//methods:
	~lod_chunk()
	{
		if (m_data) {
			delete m_data;
			m_data = 0;
		}
	}

	void	clear();
	void	update(lod_chunk_tree* tree, const vec3& viewpoint);

	void	do_split(lod_chunk_tree* tree, const vec3& viewpoint);
	bool	can_split(lod_chunk_tree* tree);	// return true if this chunk can split.  Also, request the necessary data for future, if not.
//	void	load_data(SDL_RWops* src, const tqt* texture_quadtree);
	void	unload_data();

	void	warm_up_data(lod_chunk_tree* tree, float priority);
	void	request_unload_subtree(lod_chunk_tree* tree);

	int	render(const lod_chunk_tree& c, const view_state& v, cull::result_info cull_info, render_options opt);

	void	read(SDL_RWops* in, int level, lod_chunk_tree* tree);
	void	lookup_neighbors(lod_chunk_tree* tree);

	// Utilities.

	bool	has_resident_data()
	{
		return m_data != NULL;
	}

	bool	has_children() const
	{
		return m_children[0] != NULL;
	}

	void	compute_bounding_box(const lod_chunk_tree& tree, vec3* box_center, vec3* box_extent)
	{
		box_center->y() = (m_max_y + m_min_y) * 0.5f * tree.m_vertical_scale;
		box_extent->y() = (m_max_y - m_min_y) * 0.5f * tree.m_vertical_scale;

		box_center->x() = (m_x + 0.5f) * (1 << m_level) * tree.m_base_chunk_dimension;
		box_center->z() = (m_z + 0.5f) * (1 << m_level) * tree.m_base_chunk_dimension;

		const float	EXTRA_BOX_SIZE = 1e-3f;	// this is to make chunks overlap by about a millimeter, to avoid cracks.
		box_extent->x() = (1 << m_level) * tree.m_base_chunk_dimension * 0.5f + EXTRA_BOX_SIZE;
		box_extent->z() = box_extent->get_x();
	}
};


//
// chunk_tree_loader -- helper for lod_chunk_tree that handles the
// background loader thread.
//


class chunk_tree_loader
{
public:
	chunk_tree_loader(lod_chunk_tree* tree, SDL_RWops* src);	// @@ should make tqt* a parameter.
	~chunk_tree_loader();

	void	sync_loader_thread();	// called by lod_chunk_tree::update(), to implement any changes that are ready.
	void	request_chunk_load(lod_chunk* chunk, float urgency);
	void	request_chunk_unload(lod_chunk* chunk);

	SDL_RWops*	get_source() { return m_source_stream; }

	int	loader_thread();	// thread function!
private:

	struct pending_load_request
	{
		lod_chunk*	m_chunk;
		float	m_priority;

		pending_load_request() : m_chunk(NULL), m_priority(0.0f) {}
		
		pending_load_request(lod_chunk* chunk, float priority)
			: m_chunk(chunk), m_priority(priority)
		{
		}

		static int	compare(const void* r1, const void* r2)
		// Comparison function for qsort.  Sort based on priority.
		{
			float	p1 = ((pending_load_request*) r1)->m_priority;
			float	p2 = ((pending_load_request*) r2)->m_priority;
			
			if (p1 < p2) { return -1; }
			else if (p1 > p2) { return 1; }
			else { return 0; }
		}
	};

	struct retire_info
	// A struct that associates a chunk with its newly loaded
	// data.  For communicating between m_loader_thread and the
	// main thread.
	{
		lod_chunk*	m_chunk;
		lod_chunk_data*	m_chunk_data;
		SDL_Surface*	m_texture_image;

		retire_info() : m_chunk(0), m_chunk_data(0), m_texture_image(0) {}
	};

	lod_chunk_tree*	m_tree;
	SDL_RWops*	m_source_stream;

	// These two are for the main thread's use only.  For update()
	// to communicate with sync_loader_thread().
	array<lod_chunk*>	m_unload_queue;
	array<pending_load_request>	m_load_queue;

	// These two are for the main thread to communicate with the
	// loader thread & vice versa.
#define REQUEST_BUFFER_SIZE 2
	lod_chunk* volatile	m_request_buffer[REQUEST_BUFFER_SIZE];	// chunks waiting to be loaded; filled will NULLs otherwise.
	retire_info volatile	m_retire_buffer[REQUEST_BUFFER_SIZE];	// chunks waiting to be united with their loaded data.

	// Loader thread stuff.
	SDL_Thread*	m_loader_thread;
	volatile bool	m_kill_loader;	// tell the loader to die.
	SDL_mutex*	m_mutex;
};


chunk_tree_loader::chunk_tree_loader(lod_chunk_tree* tree, SDL_RWops* src)
// Constructor.  Retains internal copies of the given pointers.
{
	m_tree = tree;
	m_source_stream = src;

	for (int i = 0; i < REQUEST_BUFFER_SIZE; i++)
	{
		m_request_buffer[i] = NULL;
		m_retire_buffer[i].m_chunk = NULL;
	}

	// Set up thread communication stuff.
	m_mutex = SDL_CreateMutex();
	assert(m_mutex);
	m_kill_loader = false;

	// start the background loader worker thread
	struct wrapper {
		static int	thread_wrapper(void* loader)
		{
			return ((chunk_tree_loader*) loader)->loader_thread();
		}
	};
	m_loader_thread = SDL_CreateThread(wrapper::thread_wrapper, this);
}


chunk_tree_loader::~chunk_tree_loader()
// Destructor.  Make sure thread is done.
{
	// signal loader thread to quit.  Wait til it quits.
	m_kill_loader = true;
	SDL_WaitThread(m_loader_thread, NULL);

	SDL_DestroyMutex(m_mutex);
}


void	chunk_tree_loader::sync_loader_thread()
// Call this periodically, to implement previously requested changes
// to the lod_chunk_tree.  Most of the work in preparing changes is
// done in a background thread, so this call is intended to be
// low-latency.
//
// The chunk_tree_loader is not allowed to make any changes to the
// lod_chunk_tree, except in this call.
{
	// Unload data.
	for (int i = 0; i < m_unload_queue.size(); i++) {
		lod_chunk*	c = m_unload_queue[i];
		// Only unload the chunk if it's not currently in use.
		// Sometimes a chunk will be marked for unloading, but
		// then is still being used due to a dependency in a
		// neighboring part of the hierarchy.  We want to
		// ignore the unload request in that case.
		if (c->m_parent != NULL
		    && c->m_parent->m_split == false)
		{
			m_unload_queue[i]->unload_data();
		}
	}
	m_unload_queue.resize(0);

	// mutex section
	SDL_LockMutex(m_mutex);
	{
		// Retire any serviced requests.
		for (int i = 0; i < REQUEST_BUFFER_SIZE; i++)
		{
			retire_info&	r = const_cast<retire_info&>(m_retire_buffer[i]);	// cast away 'volatile' (we're inside the mutex section)
			if (r.m_chunk)
			{
				assert(r.m_chunk->m_data == NULL);
				assert(r.m_chunk->m_parent == NULL || r.m_chunk->m_parent->m_data != NULL);

				// Connect the chunk with its data!
				r.m_chunk_data->m_texture_id = tqt::make_texture_id(r.m_texture_image);	// @@ this actually could cause some bad latency, because we build mipmaps...
				r.m_chunk->m_data = r.m_chunk_data;
			}
			// Clear out this entry.
			r.m_chunk = 0;
			r.m_chunk_data = 0;
			r.m_texture_image = NULL;
		}

		// Pass new requests to the loader thread.  Go in
		// order of priority, and only take a few.

		// Wipe out stale requests.
		{for (int i = 0; i < REQUEST_BUFFER_SIZE; i++) {
			m_request_buffer[i] = NULL;
		}}

		// Fill in new requests.
		int	qsize = m_load_queue.size();
		if (qsize > 0)
		{
			int	req_count = 0;

			qsort(&m_load_queue[0], qsize, sizeof(m_load_queue[0]), pending_load_request::compare);
			{for (int i = 0; i < qsize; i++)
			{
//				m_load_queue[qsize - 1 - i].m_chunk->load_data(m_source_stream, texture_quadtree);
				lod_chunk*	c = m_load_queue[qsize - 1 - i].m_chunk;	// Do the higher priority requests first.
				// Must make sure the chunk wasn't just retired.
				if (c->m_data == NULL) {
					// Request this chunk.
					m_request_buffer[req_count++] = c;
					if (req_count >= REQUEST_BUFFER_SIZE) {
						break;
					}
				}
			}}
		
			m_load_queue.resize(0);	// forget this frame's requests; we'll generate a fresh list during the next update()
		}

	}
	SDL_UnlockMutex(m_mutex);
}


void	chunk_tree_loader::request_chunk_load(lod_chunk* chunk, float urgency)
// Request that the specified chunk have its data loaded.  May
// take a while; data doesn't actually show up & get linked in
// until some future call to sync_loader_thread().
{
	assert(chunk);
	assert(chunk->m_data == NULL);

	// Don't schedule for load unless our parent already has data.
	if (chunk->m_parent == NULL
	    || chunk->m_parent->m_data != NULL)
	{
		m_load_queue.push_back(pending_load_request(chunk, urgency));
	}
}


void	chunk_tree_loader::request_chunk_unload(lod_chunk* chunk)
// Request that the specified chunk have its data unloaded;
// happens within short latency.
{
	m_unload_queue.push_back(chunk);
}


int	chunk_tree_loader::loader_thread()
// Thread function for the loader thread.  Sit and load chunk data
// from the request queue, until we get killed.
{
	while (m_kill_loader == false)
	{
		// Grab a request.
		lod_chunk*	chunk_to_load = NULL;
		SDL_LockMutex(m_mutex);
		{
			// Get first request that's not already in the
			// retire buffer.
			for (int req = 0; req < REQUEST_BUFFER_SIZE; req++)
			{
				chunk_to_load = m_request_buffer[0];	// (could be NULL)

				// shift requests down.
				int	i;
				for (i = 0; i < REQUEST_BUFFER_SIZE - 1; i++)
				{
					m_request_buffer[i] = m_request_buffer[i + 1];
				}
				m_request_buffer[i] = NULL;	// fill empty slot with NULL

				if (chunk_to_load == NULL) break;
				
				// Make sure the request is not in the retire buffer.
				bool	in_retire_buffer = false;
				{for (int i = 0; i < REQUEST_BUFFER_SIZE; i++) {
					if (m_retire_buffer[i].m_chunk == chunk_to_load) {
						// This request has already been serviced.  Don't
						// service it again.
						in_retire_buffer = true;
					}
				}}
				if (in_retire_buffer == false) break;
			}
		}
		SDL_UnlockMutex(m_mutex);

		if (chunk_to_load == NULL)
		{
			// There's no request to service right now.
			// Go to sleep for a little while and check
			// again later.
			SDL_Delay(10);
			continue;
		}

		assert(chunk_to_load->m_data == NULL);
		assert(chunk_to_load->m_parent == NULL || chunk_to_load->m_parent->m_data != NULL);
		
		// Service the request by loading the chunk's data.
		// This could take a while, and involves wating on IO,
		// so we do it with the mutex unlocked so the main
		// update/render thread can hopefully get some work
		// done.
		lod_chunk_data*	loaded_data = NULL;
		SDL_Surface*	texture_image = NULL;

		// Texture.
		const tqt*	qt = m_tree->m_texture_quadtree;
		if (qt) {
			texture_image = qt->load_image(qt->get_depth() - 1 - chunk_to_load->m_level,
						       chunk_to_load->m_x,
						       chunk_to_load->m_z);
		}

		// Geometry.
		SDL_RWseek(m_source_stream, chunk_to_load->m_data_file_position, SEEK_SET);
		loaded_data = new lod_chunk_data(m_source_stream, 0);

		// "Retire" the request.  Must do this with the mutex
		// locked.  The main thread will do
		// "chunk_to_load->m_data = loaded_data".
		SDL_LockMutex(m_mutex);
		{
			for (int i = 0; i < REQUEST_BUFFER_SIZE; i++)
			{
				if (m_retire_buffer[i].m_chunk == 0)
				{
					// empty slot; put the info here.
					m_retire_buffer[i].m_chunk = chunk_to_load;
					m_retire_buffer[i].m_chunk_data = loaded_data;
					m_retire_buffer[i].m_texture_image = texture_image;
					break;
				}
			}
		}
		SDL_UnlockMutex(m_mutex);
	}

	return 0;
}


//
// lod_chunk stuff
//


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

	const float	sx = box_extent.get_x() / (1 << 14);
	const float	sz = box_extent.get_z() / (1 << 14);

	const float	offsetx = box_center.get_x();
	const float	offsetz = box_center.get_z();

	const float	one_minus_f = (1.0f - f);

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


int	lod_chunk_data::render(const lod_chunk_tree& c, const lod_chunk& chunk, const view_state& v, cull::result_info cull_info, render_options opt,
			       const vec3& box_center, const vec3& box_extent)
// Render a chunk.
{
	if (opt.show_geometry || opt.show_edges) {
		glEnable(GL_TEXTURE_2D);

		if (m_texture_id) {
			glBindTexture(GL_TEXTURE_2D, m_texture_id);
		
			float	xsize = box_extent.get_x() * 2 * (257.0f / 256.0f);
			float	zsize = box_extent.get_z() * 2 * (257.0f / 256.0f);
			float	x0 = box_center.get_x() - box_extent.get_x() - (xsize / 256.0f) * 0.5f;
			float	z0 = box_center.get_z() - box_extent.get_z() - (xsize / 256.0f) * 0.5f;

//			//xxxxx
//			xsize = box_extent.get_x() * 2 - 10.0f;
//			zsize = box_extent.get_z() * 2 - 10.0f;
//			x0 = box_center.get_x() - box_extent.get_x() + 5.0f;
//			z0 = box_center.get_z() - box_extent.get_z() + 5.0f;
//			//xxxxx

			// Set up texgen for this tile.
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			float	p[4] = { 0, 0, 0, 0 };
			p[0] = 1.0f / xsize;
			p[3] = -x0 / xsize;
			glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
			p[0] = 0;
			
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			p[2] = 1.0f / zsize;
			p[3] = -z0 / zsize;
			glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
			
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
		}
	}


	int	triangle_count = 0;

	// Grab some space to put processed verts.
	assert(s_stream);
	float*	output_verts = (float*) s_stream->reserve_memory(sizeof(float) * 3 * m_verts.vertex_count);

	// Process our vertices into the output buffer.
	float	f = (chunk.lod & 255) / 255.0f;
	if (opt.morph == false) {
		f = 0;
	}
	morph_vertices(output_verts, m_verts, box_center, box_extent, f);

	if (opt.show_geometry) {
		// draw this chunk.
		glColor3f(1, 1, 1);
		glVertexPointer(3, GL_FLOAT, 0, output_verts);
		glDrawElements(GL_TRIANGLE_STRIP, m_verts.index_count, GL_UNSIGNED_SHORT, m_verts.indices);
		triangle_count += m_verts.triangle_count;
	}

	return triangle_count;
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
	if (m_split) {
		m_split = false;

		// Recurse to children.
		if (has_children()) {
			for (int i = 0; i < 4; i++) {
				m_children[i]->clear();
			}
		}
	}
}


void	lod_chunk::update(lod_chunk_tree* tree, const vec3& viewpoint)
// Computes 'lod' and split values for this chunk and its subtree,
// based on the given camera parameters and the parameters stored in
// 'base'.  Traverses the tree and forces neighbor chunks to a valid
// LOD, and updates its contained edges to prevent cracks.
//
// !!!  For correct results, the tree must have been clear()ed before
// calling update() !!!
{
	vec3	box_center, box_extent;
	compute_bounding_box(*tree, &box_center, &box_extent);

	Uint16	desired_lod = tree->compute_lod(box_center, box_extent, viewpoint);

	if (has_children()
		&& desired_lod > (lod | 0x0FF)
		&& can_split(tree))
	{
		do_split(tree, viewpoint);

		// Recurse to children.
		for (int i = 0; i < 4; i++) {
			m_children[i]->update(tree, viewpoint);
		}
	} else {
		// We're good... this chunk can represent its region within the max error tolerance.
		if ((lod & 0xFF00) == 0) {
			// Root chunk -- make sure we have valid morph value.
			lod = iclamp(desired_lod, lod & 0xFF00, lod | 0x0FF);
//			// tbp: and that we also have something to display :)
//			load_data(tree->m_loader->get_source(), texture_quadtree);
		}

		// Request residency for our children, and request our
		// grandchildren and further descendents be unloaded.
		if (has_children()) {
			float	priority = 0;
			if (desired_lod > (lod & 0xFF00)) {
				priority = (lod & 0x0FF) / 255.0f;
			}

			for (int i = 0; i < 4; i++) {
				m_children[i]->warm_up_data(tree, priority);
			}
		}
	}
}


void	lod_chunk::do_split(lod_chunk_tree* tree, const vec3& viewpoint)
// Enable this chunk.  Use the given viewpoint to decide what morph
// level to use.
//
// If able to split this chunk, then the function returns true.  If
// unable to split this chunk, then the function doesn't change the
// tree, and returns false.
{
	if (m_split == false) {
		assert(this->can_split(tree));
		assert(has_resident_data());

		m_split = true;

		if (has_children()) {
			// Make sure children have a valid lod value.
			{for (int i = 0; i < 4; i++) {
				lod_chunk*	c = m_children[i];
				vec3	box_center, box_extent;
				c->compute_bounding_box(*tree, &box_center, &box_extent);
				Uint16	desired_lod = tree->compute_lod(box_center, box_extent, viewpoint);
				c->lod = iclamp(desired_lod, c->lod & 0xFF00, c->lod | 0x0FF);
			}}
		}

		// make sure ancestors are split...
		for (lod_chunk* p = m_parent; p && p->m_split == false; p = p->m_parent) {
			p->do_split(tree, viewpoint);
		}
	}
}


bool	lod_chunk::can_split(lod_chunk_tree* tree)
// Return true if this chunk can be split.  Also, requests the
// necessary data for the chunk children and its dependents.
//
// A chunk won't be able to be split if it doesn't have vertex data,
// or if any of the dependent chunks don't have vertex data.
{
	if (m_split) {
		// Already split.  Also our data & dependents' data is already
		// freshened, so no need to request it again.
		return true;
	}

	if (has_children() == false) {
		// Can't ever split.  No data to request.
		return false;
	}

	bool	can_split = true;

	// Check the data of the children.
	{for (int i = 0; i < 4; i++) {
		lod_chunk*	c = m_children[i];
		if (c->has_resident_data() == false) {
			tree->m_loader->request_chunk_load(c, 1.0f);
			can_split = false;
		}
	}}

	// Make sure ancestors have data...
	for (lod_chunk* p = m_parent; p && p->m_split == false; p = p->m_parent) {
		if (p->can_split(tree) == false) {
			can_split = false;
		}
	}

	// Make sure neighbors have data at a close-enough level in the tree.
	{for (int i = 0; i < 4; i++) {
		lod_chunk*	n = m_neighbor[i].m_chunk;

		const int	MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE = 2;	// allow up to two levels of difference between chunk neighbors.
		{for (int count = 0;
		     n && count < MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE;
		     count++)
		{
			n = n->m_parent;
		}}

		if (n && n->can_split(tree) == false) {
			can_split = false;
		}
	}}

	return can_split;
}


#if 0
void	lod_chunk::load_data(SDL_RWops* src, const tqt* texture_quadtree)
// Immediately load our data.  Use loader->request_chunk_load() to
// schedule a similar operation in the background.
{
	if (m_data == NULL) {
		assert(m_parent == NULL || m_parent->m_data != NULL);

		// Load the data.
		
		// Texture.
		int	texture_id = 0;
		if (texture_quadtree) {
			texture_id = texture_quadtree->get_texture_id(texture_quadtree->get_depth() - 1 - m_level, m_x, m_z);
		}

		// Geometry.
		SDL_RWseek(src, m_data_file_position, SEEK_SET);
		m_data = new lod_chunk_data(src, texture_id);
	}
}
#endif // 0


void	lod_chunk::unload_data()
// Immediately unload our data.
{
	assert(m_parent != NULL && m_parent->m_split == false);
	assert(m_split == false);

	// debug check -- we should only unload data from the leaves
	// upward.
	if (has_children()) {
		for (int i = 0; i < 4; i++) {
			assert(m_children[i]->m_data == NULL);
		}
	}

	// Do the unloading.
	if (m_data) {
		delete m_data;
		m_data = NULL;
	}
}


void	lod_chunk::warm_up_data(lod_chunk_tree* tree, float priority)
// Schedule this node's data for loading at the given priority.  Also,
// schedule our child/descendent nodes for unloading.
{
	assert(tree);

	if (m_data == NULL)
	{
		// Request our data.
		tree->m_loader->request_chunk_load(this, priority);
	}

	// Request unload.  Skip a generation.
	if (has_children()) {
		for (int i = 0; i < 4; i++) {
			lod_chunk*	c = m_children[i];
			if (c->has_children()) {
				for (int j = 0; j < 4; j++) {
					c->m_children[j]->request_unload_subtree(tree);
				}
			}
		}
	}
}


void	lod_chunk::request_unload_subtree(lod_chunk_tree* tree)
// If we have any data, request that it be unloaded.  Make the same
// request of our descendants.
{
	if (m_data) {
		// Put descendents in the queue first, so they get
		// unloaded first.
		if (has_children()) {
			for (int i = 0; i < 4; i++) {
				m_children[i]->request_unload_subtree(tree);
			}
		}

		tree->m_loader->request_chunk_unload(this);
	}
}


int	lod_chunk::render(const lod_chunk_tree& c, const view_state& v, cull::result_info cull_info, render_options opt)
// Draws the given lod tree.  Uses the current state stored in the
// tree w/r/t split & LOD level.
//
// Returns the number of triangles rendered.
{
	assert(m_data != NULL);

	vec3	box_center, box_extent;
	compute_bounding_box(c, &box_center, &box_extent);

	// Frustum culling.
	if (cull_info.active_planes) {
		cull_info = cull::compute_box_visibility(box_center, box_extent, v.m_frustum, cull_info);
		if (cull_info.culled) {
			// Bounding box is not visible; no need to draw this node or its children.
			return 0;
		}
	}

	int	triangle_count = 0;

	if (m_split) {
		assert(has_children());

		// Recurse to children.  Some subset of our descendants will be rendered in our stead.
		for (int i = 0; i < 4; i++) {
			static const bool	explode = false;
			// EXPLODE
			if (explode) {
				int	level = c.m_tree_depth - ((this->lod) >> 8);
				float	offset = 30.f * ((1 << level) / float(1 << c.m_tree_depth));

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				switch (i) {
				default:
				case 0:
					glTranslatef(-offset, 0, -offset);
					break;
				case 1:
					glTranslatef(offset, 0, -offset);
					break;
				case 2:
					glTranslatef(-offset, 0, offset);
					break;
				case 3:
					glTranslatef(offset, 0, offset);
					break;
				}
			}

			triangle_count += m_children[i]->render(c, v, cull_info, opt);

			// EXPLODE
			if (explode) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	} else {
		if (opt.show_box) {
			// draw bounding box.
			glDisable(GL_TEXTURE_2D);
			float	f = (lod & 255) / 255.0f;	//xxx
			glColor3f(f, 1 - f, 0);
			draw_box(box_center - box_extent, box_center + box_extent);
		}

		// Display our data.
		triangle_count += m_data->render(c, *this, v, cull_info, opt, box_center, box_extent);
	}

	return triangle_count;
}


void	lod_chunk::read(SDL_RWops* in, int level, lod_chunk_tree* tree)
// Read chunk data from the given file and initialize this chunk with it.
// Recursively loads child chunks for level > 0.
{
	m_split = false;

	// Get this chunk's label, and add it to the table.
	int	chunk_label = SDL_ReadLE32(in);
	if (chunk_label > tree->m_chunk_count)
	{
		assert(0);
		printf("invalid chunk_label: %d, level = %d\n", chunk_label, level);
		exit(1);
	}
	assert(tree->m_chunk_table[chunk_label] == 0);
	tree->m_chunk_table[chunk_label] = this;

	// Initialize neighbor links.
	{for (int i = 0; i < 4; i++) {
		m_neighbor[i].m_label = SDL_ReadLE32(in);
	}}

	// Read our chunk address.  We can reconstruct our bounding box
	// from this info, along with the root tree info.
	m_level = ReadByte(in);
	m_x = SDL_ReadLE16(in);
	m_z = SDL_ReadLE16(in);

	m_min_y = SDL_ReadLE16(in);
	m_max_y = SDL_ReadLE16(in);

	// Skip the chunk data but remember our filepos, so we can load it
	// when it's demanded.
	int	chunk_size = SDL_ReadLE32(in);
	m_data_file_position = SDL_RWtell(in);
	SDL_RWseek(in, chunk_size, SEEK_CUR);
	m_data = NULL;

	assert(m_data_file_position + chunk_size == SDL_RWtell(in));

	// Recurse to child chunks.
	if (level > 0) {
		for (int i = 0; i < 4; i++) {
			m_children[i] = &tree->m_chunks[tree->m_chunks_allocated++];
			m_children[i]->lod = lod + 0x100;
			m_children[i]->m_parent = this;
			m_children[i]->read(in, level-1, tree);
		}
	} else {
		for (int i = 0; i < 4; i++) {
			m_children[i] = NULL;
		}
	}
}


void	lod_chunk::lookup_neighbors(lod_chunk_tree* tree)
// Convert our neighbor labels to neighbor pointers.  Recurse to child chunks.
{
	for (int i = 0; i < 4; i++) {
		if (m_neighbor[i].m_label < -1 || m_neighbor[i].m_label >= tree->m_chunk_count)
		{
			assert(0);
			m_neighbor[i].m_label = -1;
		}
		if (m_neighbor[i].m_label == -1) {
			m_neighbor[i].m_chunk = NULL;
		} else {
			m_neighbor[i].m_chunk = tree->m_chunk_table[m_neighbor[i].m_label];
		}
	}

	if (has_children()) {
		{for (int i = 0; i < 4; i++) {
			m_children[i]->lookup_neighbors(tree);
		}}
	}
}


//
// lod_chunk_tree implementation.  lod_chunk_tree is the external
// interface to chunked LOD.
//


lod_chunk_tree::lod_chunk_tree(SDL_RWops* src, const tqt* texture_quadtree)
// Construct and initialize a tree of LOD chunks, using data from the given
// source.  Uses a special .chu file format which is a pretty direct
// encoding of the chunk data.
{
	m_texture_quadtree = texture_quadtree;

	// Read and verify a "CHU\0" header tag.
	Uint32	tag = SDL_ReadLE32(src);
	if (tag != (('C') | ('H' << 8) | ('U' << 16))) {
		throw "Input file is not in .CHU format";
	}

	int	format_version = SDL_ReadLE16(src);
	if (format_version != 7)
	{
		assert(0);
		throw "Input format has non-matching version number";
	}

	m_tree_depth = SDL_ReadLE16(src);
	m_error_LODmax = ReadFloat32(src);
	m_vertical_scale = ReadFloat32(src);
	m_base_chunk_dimension = ReadFloat32(src);
	m_chunk_count = SDL_ReadLE32(src);

	// Create a lookup table of chunk labels.
	m_chunk_table = new lod_chunk*[m_chunk_count];
	memset(m_chunk_table, 0, sizeof(m_chunk_table[0]) * m_chunk_count);

	// Compute a sane default value for distance_LODmax, in case the client code
	// neglects to call set_parameters().
	set_parameters(5.0f, 640.0f, 90.0f);

	// Load the chunk tree (not the actual data).
	m_chunks_allocated = 0;
	m_chunks = new lod_chunk[m_chunk_count];

	m_chunks_allocated++;
	m_chunks[0].lod = 0;
	m_chunks[0].m_parent = 0;
	m_chunks[0].read(src, m_tree_depth-1, this);
	m_chunks[0].lookup_neighbors(this);

	// Set up our loader.
	m_loader = new chunk_tree_loader(this, src);
}


lod_chunk_tree::~lod_chunk_tree()
// Destructor.
{
	delete [] m_chunks;
	delete [] m_chunk_table;
	delete m_loader;
}


void	lod_chunk_tree::update(const vec3& viewpoint)
// Initializes tree state, so it can be rendered.  The given viewpoint
// is used to do distance-based LOD switching on our contained chunks.
{
	if (m_chunks[0].m_data == NULL)
	{
		// Get root-node data!
		m_loader->request_chunk_load(&m_chunks[0], 1.0f);
	}

	m_chunks[0].clear();
	m_chunks[0].update(this, viewpoint);
	m_loader->sync_loader_thread();
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

	s_vertical_scale = m_vertical_scale;

	if (m_chunks[0].m_data == NULL)
	{
		// No data in the root node; we can't really do
		// anything.  This should only happen briefly at the
		// very start of the program, until the loader thread
		// kicks in.
	}
	else
	{
		// Render the chunked LOD tree.
		triangle_count += m_chunks[0].render(*this, v, cull::result_info(), opt);
	}

	return triangle_count;
}


void	lod_chunk_tree::get_bounding_box(vec3* box_center, vec3* box_extent)
// Returns the bounding box of the data in this chunk tree.
{
	assert(m_chunks_allocated > 0);

	m_chunks[0].compute_bounding_box(*this, box_center, box_extent);
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
	const float	tan_half_FOV = tanf(0.5f * horizontal_FOV_degrees * (float) M_PI / 180.0f);

	// distance_LODmax is the distance below which we need to be
	// at the maximum LOD.  It's used in compute_lod(), which is
	// called by the chunks during update().
	m_distance_LODmax = m_error_LODmax * screen_width_pixels / (2 * tan_half_FOV * max_pixel_error);
}


Uint16	lod_chunk_tree::compute_lod(const vec3& center, const vec3& extent, const vec3& viewpoint) const
// Given an AABB and the viewpoint, this function computes a desired
// LOD level, based on the distance from the viewpoint to the nearest
// point on the box.  So, desired LOD is purely a function of
// distance and the chunk tree parameters.
{
	vec3	disp = viewpoint - center;
	disp.set(0, fmax(0, fabsf(disp.get(0)) - extent.get(0)));
	disp.set(1, fmax(0, fabsf(disp.get(1)) - extent.get(1)));
	disp.set(2, fmax(0, fabsf(disp.get(2)) - extent.get(2)));

//	disp.set(1, 0);	//xxxxxxx just do calc in 2D, for debugging

	float	d = 0;
	d = sqrtf(disp * disp);

	return iclamp(((m_tree_depth << 8) - 1) - int(log2(fmax(1, d / m_distance_LODmax)) * 256), 0, 0x0FFFF);
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
