// chunklod.h	-- tu@tulrich.com Copyright 2001 by Thatcher Ulrich

// Header declaring data structures for chunk LOD rendering.

#ifndef CHUNKLOD_H
#define CHUNKLOD_H


#include <engine/geometry.h>
#include <engine/view_state.h>


struct lod_chunk;

struct render_options {
	bool	show_box;
	bool	show_geometry;
	bool	morph;
	bool	show_edges;

	render_options()
		:
		show_box(false),
		show_geometry(true),
		morph(true),
		show_edges(true)
	{
	}
};


class lod_chunk_tree {
// Use this class as the UI to a chunked-LOD object.
// !!! turn this into an interface class and get the data into the .cpp file !!!
public:
	// External interface.
	lod_chunk_tree(SDL_RWops* src);
	void	set_parameters(float max_pixel_error, float screen_width, float horizontal_FOV_degrees);
	void	update(const vec3& viewpoint);
	int	render(const view_state& v, render_options opt);

	void	get_bounding_box(vec3* box_center, vec3* box_extent);

	// Used by our contained chunks.
	Uint16	compute_lod(const vec3& center, const vec3& extent, const vec3& viewpoint) const;

//data:
	lod_chunk*	m_root;
	int	m_tree_depth;	// from chunk data.
	float	m_error_LODmax;	// from chunk data.
	float	m_distance_LODmax;	// computed from chunk data params and set_parameters() inputs.
	float	m_vertical_scale;	// from chunk data; displayed_height = y_data * m_vertical_scale
	int	m_chunk_count;
	lod_chunk**	m_chunk_table;
};


#endif // CHUNKLOD_H
