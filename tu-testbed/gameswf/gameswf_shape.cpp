// gameswf_shape.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier outline shapes, the basis for most SWF rendering.


#include "gameswf_shape.h"

#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_render.h"
#include "gameswf_stream.h"
#include "gameswf_tesselate.h"

#include "base/tu_file.h"

#include <float.h>


namespace gameswf
{
	//
	// edge
	//

	edge::edge()
		:
		m_cx(0), m_cy(0),
		m_ax(0), m_ay(0)
	{}

	edge::edge(float cx, float cy, float ax, float ay)
		:
		m_cx(cx), m_cy(cy),
		m_ax(ax), m_ay(ay)
	{
	}

	void	edge::tesselate_curve() const
	// Send this segment to the tesselator.
	{
		tesselate::add_curve_segment(m_cx, m_cy, m_ax, m_ay);
	}


	//
	// path
	//


	path::path()
		:
		m_new_shape(false)
	{
		reset(0, 0, 0, 0, 0);
	}

	path::path(float ax, float ay, int fill0, int fill1, int line)
	{
		reset(ax, ay, fill0, fill1, line);
	}


	void	path::reset(float ax, float ay, int fill0, int fill1, int line)
	// Reset all our members to the given values, and clear our edge list.
	{
		m_ax = ax;
		m_ay = ay;
		m_fill0 = fill0;
		m_fill1 = fill1;
		m_line = line;

		m_edges.resize(0);

		assert(is_empty());
	}


	bool	path::is_empty() const
	// Return true if we have no edges.
	{
		return m_edges.size() == 0;
	}


	bool	path::point_test(float x, float y)
	// Point-in-shape test.  Return true if the query point is on the filled
	// interior of this shape.
	{
		assert(0);	// this routine isn't finished

		if (m_fill0 == 0 && m_fill1 == 0)
		{
			// Not a filled shape.
			return false;
		}

		// Look for the nearest point on an edge,
		// to the left of the query point.  If that
		// edge is a fill edge, then the query point
		// is inside the shape.
		bool	inside = false;
		//float	closest_point = 1e6;

		for (int i = 0; i < m_edges.size(); i++)
		{
			// edges[i].solve_at_y(hit?, x?, slope up/down?, y);
			// if (hit && x - x? < closest_point) {
			//   inside = up/down ? (m_fill0 > -1) : (m_fill1 > -1);
			// }
		}

		return inside;
	}


	void	path::tesselate() const
	// Push this path into the tesselator.
	{
		tesselate::begin_path(
			m_fill0 - 1,
			m_fill1 - 1,
			m_line - 1,
			m_ax, m_ay);
		for (int i = 0; i < m_edges.size(); i++)
		{
			m_edges[i].tesselate_curve();
		}
		tesselate::end_path();
	}


	// Utility.


	void	write_point_array(tu_file* out, const array<point>& pt_array)
	// Dump the given point array into the given stream.
	{
		int	n = pt_array.size();

		out->write_le32(n);
		for (int i = 0; i < n; i++)
		{
			out->write_float32(pt_array[i].m_x);
			out->write_float32(pt_array[i].m_y);
		}
	}


	void	read_point_array(tu_file* in, array<point>* pt_array)
	// Read the point array data from the stream into *pt_array.
	{
		int	n = in->read_le32();

		pt_array->resize(n);
		for (int i = 0; i < n; i ++)
		{
			(*pt_array)[i].m_x = in->read_float32();
			(*pt_array)[i].m_y = in->read_float32();
		}
	}


	//
	// mesh
	//

	
	mesh::mesh()
	{
	}


	void	mesh::add_triangle(const point& a, const point& b, const point& c)
	{
		m_triangle_list.push_back(a);
		m_triangle_list.push_back(b);
		m_triangle_list.push_back(c);
	}


	void	mesh::display(const fill_style& style) const
	{
		// pass mesh to renderer.
		if (m_triangle_list.size() > 0)
		{
			style.apply(0);
			render::draw_mesh(&m_triangle_list[0].m_x, m_triangle_list.size());
		}
	}


	void	mesh::output_cached_data(tu_file* out)
	// Dump our data to *out.
	{
		write_point_array(out, m_triangle_list);
	}

	
	void	mesh::input_cached_data(tu_file* in)
	// Slurp our data from *out.
	{
		read_point_array(in, &m_triangle_list);
	}


	//
	// line_strip
	//


	line_strip::line_strip()
	// Default constructor, for array<>.
		:
		m_style(-1)
	{}


	line_strip::line_strip(int style, const point coords[], int coord_count)
	// Construct the line strip (polyline) made up of the given sequence of points.
		:
		m_style(style)
	{
		assert(style >= 0);
		assert(coords != NULL);
		assert(coord_count > 1);

		m_coords.resize(coord_count);
		memcpy(&m_coords[0], coords, coord_count * sizeof(coords[0]));
	}


	void	line_strip::display(const line_style& style) const
	// Render this line strip in the given style.
	{
		assert(m_coords.size() > 1);

		style.apply();
		render::draw_line_strip(&m_coords[0].m_x, m_coords.size());
	}


	void	line_strip::output_cached_data(tu_file* out)
	// Dump our data to *out.
	{
		out->write_le32(m_style);
		write_point_array(out, m_coords);
	}

	
	void	line_strip::input_cached_data(tu_file* in)
	// Slurp our data from *out.
	{
		m_style = in->read_le32();
		read_point_array(in, &m_coords);
	}


	//
	// mesh_set
	//


	mesh_set::mesh_set()
		:
//		m_last_frame_rendered(-1),
		m_error_tolerance(0)	// invalid -- don't use this constructor; it's only here for array (@@ fix array)
	{
	}




	mesh_set::mesh_set(const shape_character* sh, float error_tolerance)
	// Tesselate the shape's paths into a different mesh for each fill style.
		:
//		m_last_frame_rendered(0),
		m_error_tolerance(error_tolerance)
	{
		struct collect_traps : public tesselate::trapezoid_accepter
		{
			mesh_set*	m;	// the mesh_set that receives trapezoids.

			collect_traps(mesh_set* set) : m(set) {}

			// Overrides from trapezoid_accepter
			virtual void	accept_trapezoid(int style, const tesselate::trapezoid& tr)
			{
				// Add the trapezoid to our mesh set.
				m->add_triangle(
					style,
					point(tr.m_lx0, tr.m_y0),
					point(tr.m_lx1, tr.m_y1),
					point(tr.m_rx1, tr.m_y1));
				m->add_triangle(
					style,
					point(tr.m_lx0, tr.m_y0),
					point(tr.m_rx0, tr.m_y0),
					point(tr.m_rx1, tr.m_y1));
			}

			void	accept_line_strip(int style, const point coords[], int coord_count)
			// Remember this line strip in our mesh set.
			{
				m->add_line_strip(style, coords, coord_count);
			}
		};
		collect_traps	accepter(this);

		sh->tesselate(error_tolerance, &accepter);

		// triangles should be collected now into the meshes for each fill style.
	}


//	int	mesh_set::get_last_frame_rendered() const { return m_last_frame_rendered; }
//	void	mesh_set::set_last_frame_rendered(int frame_counter) { m_last_frame_rendered = frame_counter; }


	void	mesh_set::display(
		const display_info& di,
		const array<fill_style>& fills,
		const array<line_style>& line_styles) const
	// Throw our meshes at the renderer.
	{
		assert(m_error_tolerance > 0);

		// Setup transforms.
		render::set_matrix(di.m_matrix);
		render::set_cxform(di.m_color_transform);

		// Dump meshes into renderer, one mesh per style.
		{for (int i = 0; i < m_meshes.size(); i++)
		{
			m_meshes[i].display(fills[i]);
		}}

		// Dump line-strips into renderer.
		for (int i = 0; i < m_line_strips.size(); i++)
		{
			int	style = m_line_strips[i].get_style();
			m_line_strips[i].display(line_styles[style]);
		}
	}


	void	mesh_set::add_triangle(int style, const point& a, const point& b, const point& c)
	// Add the specified triangle to the mesh associated with the
	// given fill style.
	{
		assert(style >= 0);
		assert(style < 1000);	// sanity check

		// Expand our mesh list if necessary.
		if (style >= m_meshes.size())
		{
			m_meshes.resize(style + 1);
		}

		m_meshes[style].add_triangle(a, b, c);
	}

	
	void	mesh_set::add_line_strip(int style, const point coords[], int coord_count)
	// Add the specified line strip to our list of things to render.
	{
		assert(style >= 0);
		assert(style < 1000);	// sanity check
		assert(coords != NULL);
		assert(coord_count > 1);

		m_line_strips.push_back(line_strip(style, coords, coord_count));
	}


	void	mesh_set::output_cached_data(tu_file* out)
	// Dump our data to the output stream.
	{
		out->write_float32(m_error_tolerance);

		int	mesh_n = m_meshes.size();
		out->write_le32(mesh_n);
		for (int i = 0; i < mesh_n; i++)
		{
			m_meshes[i].output_cached_data(out);
		}

		int	lines_n = m_line_strips.size();
		out->write_le32(lines_n);
		{for (int i = 0; i < lines_n; i++)
		{
			m_line_strips[i].output_cached_data(out);
		}}
	}


	void	mesh_set::input_cached_data(tu_file* in)
	// Grab our data from the input stream.
	{
		m_error_tolerance = in->read_float32();

		int	mesh_n = in->read_le32();
		m_meshes.resize(mesh_n);
		for (int i = 0; i < mesh_n; i++)
		{
			m_meshes[i].input_cached_data(in);
		}

		int	lines_n = in->read_le32();
		m_line_strips.resize(lines_n);
		{for (int i = 0; i < lines_n; i++)
		{
			m_line_strips[i].input_cached_data(in);
		}}
	}


	//
	// helper functions.
	//


	static void	read_fill_styles(array<fill_style>* styles, stream* in, int tag_type, movie* m)
	// Read fill styles, and push them onto the given style array.
	{
		assert(styles);

		// Get the count.
		int	fill_style_count = in->read_u8();
		if (tag_type > 2)
		{
			if (fill_style_count == 0xFF)
			{
				fill_style_count = in->read_u16();
			}
		}

		IF_VERBOSE_PARSE(log_msg("rfs: fsc = %d\n", fill_style_count));

		// Read the styles.
		for (int i = 0; i < fill_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type, m);
		}
	}


	static void	read_line_styles(array<line_style>* styles, stream* in, int tag_type)
	// Read line styles and push them onto the back of the given array.
	{
		// Get the count.
		int	line_style_count = in->read_u8();

		IF_VERBOSE_PARSE(log_msg("rls: lsc = %d\n", line_style_count));

		// @@ does the 0xFF flag apply to all tag types?
		// if (tag_type > 2)
		// {
			if (line_style_count == 0xFF)
			{
				line_style_count = in->read_u16();
			}
		// }

		IF_VERBOSE_PARSE(log_msg("rls: lsc2 = %d\n", line_style_count));

		// Read the styles.
		for (int i = 0; i < line_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type);
		}
	}


	//
	// shape_character
	//


	shape_character::shape_character()
	{
	}


	shape_character::~shape_character()
	{
		// Free our mesh_sets.
		for (int i = 0; i < m_cached_meshes.size(); i++)
		{
			delete m_cached_meshes[i];
		}
	}


	void	shape_character::read(stream* in, int tag_type, bool with_style, movie* m)
	{
		if (with_style)
		{
			m_bound.read(in);
			read_fill_styles(&m_fill_styles, in, tag_type, m);
			read_line_styles(&m_line_styles, in, tag_type);
		}

		//
		// SHAPE
		//
		int	num_fill_bits = in->read_uint(4);
		int	num_line_bits = in->read_uint(4);

		IF_VERBOSE_PARSE(log_msg("scr: nfb = %d, nlb = %d\n", num_fill_bits, num_line_bits));

		// These are state variables that keep the
		// current position & style of the shape
		// outline, and vary as we read the edge data.
		//
		// At the moment we just store each edge with
		// the full necessary info to render it, which
		// is simple but not optimally efficient.
		int	fill_base = 0;
		int	line_base = 0;
		float	x = 0, y = 0;
		path	current_path;

#define SHAPE_LOG 0
		// SHAPERECORDS
		for (;;) {
			int	type_flag = in->read_uint(1);
			if (type_flag == 0)
			{
				// Parse the record.
				int	flags = in->read_uint(5);
				if (flags == 0) {
					// End of shape records.

					// Store the current path if any.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
					}

					break;
				}
				if (flags & 0x01)
				{
					// move_to = 1;

					// Store the current path if any, and prepare a fresh one.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
					}

					int	num_move_bits = in->read_uint(5);
					int	move_x = in->read_sint(num_move_bits);
					int	move_y = in->read_sint(num_move_bits);

					x = (float) move_x;
					y = (float) move_y;

					// Set the beginning of the path.
					current_path.m_ax = x;
					current_path.m_ay = y;

					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: moveto %4g %4g\n", x, y));
				}
				if ((flags & 0x02)
					&& num_fill_bits > 0)
				{
					// fill_style_0_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_fill_bits);
					if (style > 0)
					{
						style += fill_base;
					}
					current_path.m_fill0 = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: fill0 = %d\n", current_path.m_fill0));
				}
				if ((flags & 0x04)
					&& num_fill_bits > 0)
				{
					// fill_style_1_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_fill_bits);
					if (style > 0)
					{
						style += fill_base;
					}
					current_path.m_fill1 = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: fill1 = %d\n", current_path.m_fill1));
				}
				if ((flags & 0x08)
					&& num_line_bits > 0)
				{
					// line_style_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_line_bits);
					if (style > 0)
					{
						style += line_base;
					}
					current_path.m_line = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: line = %d\n", current_path.m_line));
				}
				if (flags & 0x10) {
					assert(tag_type >= 22);

					IF_VERBOSE_PARSE(log_msg("scr: more fill styles\n"));

					// Store the current path if any.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);

						// Clear styles.
						current_path.m_fill0 = -1;
						current_path.m_fill1 = -1;
						current_path.m_line = -1;
					}
					// Tack on an empty path signalling a new shape.
					// @@ need better understanding of whether this is correct??!?!!
					// @@ i.e., we should just start a whole new shape here, right?
					m_paths.push_back(path());
					m_paths.back().m_new_shape = true;

					fill_base = m_fill_styles.size();
					line_base = m_line_styles.size();
					read_fill_styles(&m_fill_styles, in, tag_type, m);
					read_line_styles(&m_line_styles, in, tag_type);
					num_fill_bits = in->read_uint(4);
					num_line_bits = in->read_uint(4);
				}
			}
			else
			{
				// EDGERECORD
				int	edge_flag = in->read_uint(1);
				if (edge_flag == 0)
				{
					// curved edge
					int num_bits = 2 + in->read_uint(4);
					float	cx = x + in->read_sint(num_bits);
					float	cy = y + in->read_sint(num_bits);
					float	ax = cx + in->read_sint(num_bits);
					float	ay = cy + in->read_sint(num_bits);

					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", x, y, cx, cy, ax, ay));

					current_path.m_edges.push_back(edge(cx, cy, ax, ay));	

					x = ax;
					y = ay;
				}
				else
				{
					// straight edge
					int	num_bits = 2 + in->read_uint(4);
					int	line_flag = in->read_uint(1);
					float	dx = 0, dy = 0;
					if (line_flag)
					{
						// General line.
						dx = (float) in->read_sint(num_bits);
						dy = (float) in->read_sint(num_bits);
					}
					else
					{
						int	vert_flag = in->read_uint(1);
						if (vert_flag == 0) {
							// Horizontal line.
							dx = (float) in->read_sint(num_bits);
						} else {
							// Vertical line.
							dy = (float) in->read_sint(num_bits);
						}
					}

					if (SHAPE_LOG) IF_VERBOSE_PARSE(log_msg("scr: straight edge = %4g %4g - %4g %4g\n", x, y, x + dx, y + dy));

					current_path.m_edges.push_back(edge(x + dx/2, y + dy/2, x + dx, y + dy));

					x += dx;
					y += dy;
				}
			}
		}
	}


	void	shape_character::display(const display_info& di)
	// Draw the shape using our own inherent styles.
	{
		display(di, m_fill_styles, m_line_styles);
	}

		
	void	shape_character::display(
		const display_info& di,
		const array<fill_style>& fill_styles,
		const array<line_style>& line_styles) const
	// Display our shape.  Use the fill_styles arg to
	// override our default set of fill styles (e.g. when
	// rendering text).
	{
		assert(di.m_movie);

		// Compute the error tolerance in object-space.
		float	object_space_max_error = 20.0f / di.m_matrix.get_max_scale() / di.m_movie->get_pixel_scale();

		// See if we have an acceptable mesh available; if so then render with it.
		bool rendered = false;
		for (int i = 0; i < m_cached_meshes.size(); i++)
		{
			if (object_space_max_error > m_cached_meshes[i]->get_error_tolerance() * 3.0f)
			{
				// Mesh is too high-res; the remaining
				// meshes are higher res, so give up.
				break;
			}

			if (object_space_max_error > m_cached_meshes[i]->get_error_tolerance())
			{
				// Do it.
				m_cached_meshes[i]->display(di, fill_styles, line_styles);
//				m_cached_meshes[i]->set_last_frame_rendered(di.m_display_number);
				rendered = true;
				break;
			}
		}

		if (rendered == false)
		{
			// Construct a new mesh to handle this error tolerance.
			mesh_set*	m = new mesh_set(this, object_space_max_error * 0.75f);
			m_cached_meshes.push_back(m);
			m->display(di, fill_styles, line_styles);
//			m->set_last_frame_rendered(di.m_display_number);

			sort_and_clean_meshes(di.m_display_number);
		}
	}


	static int	sort_by_decreasing_error(const void* A, const void* B)
	{
		const mesh_set*	a = (const mesh_set*) A;
		const mesh_set*	b = (const mesh_set*) B;

		if (a->get_error_tolerance() > b->get_error_tolerance())
		{
			return 1;
		}
		else if (a->get_error_tolerance() < b->get_error_tolerance())
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}


	void	shape_character::sort_and_clean_meshes(int display_number) const
	// Maintain cached meshes.  Clean out mesh_sets that haven't
	// been used recently, and make sure they're sorted from high
	// error to low error.
	{
// @@ I think it's bad policy to dump cached meshes.  Maybe I'm wrong,
// so keep the code around for now...
#if 0
		// Remove meshes that haven't been used in a long
		// time.
		for (int i = 0; i < m_cached_meshes.size(); i++)
		{
			mesh_set*	m = m_cached_meshes[i];
			if (display_number - m->get_last_frame_rendered() > 60)
			{
				// We haven't used this mesh in a while, so toss it.
				m_cached_meshes[i] = m_cached_meshes.back();
				m_cached_meshes.resize(m_cached_meshes.size() - 1);
				delete m;
				i--;
			}
		}
#endif // 0

		// Re-sort.
		if (m_cached_meshes.size() > 0)
		{
			qsort(
				&m_cached_meshes[0],
				m_cached_meshes.size(),
				sizeof(m_cached_meshes[0]),
				sort_by_decreasing_error);
		}
	}

		
	void	shape_character::tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const
	// Push our shape data through the tesselator.
	{
		tesselate::begin_shape(accepter, error_tolerance);
		for (int i = 0; i < m_paths.size(); i++)
		{
			if (m_paths[i].m_new_shape == true)
			{
				// Hm; should handle separate sub-shapes in a less lame way.
				tesselate::end_shape();
				tesselate::begin_shape(accepter, error_tolerance);
			}
			else
			{
				m_paths[i].tesselate();
			}
		}
		tesselate::end_shape();
	}


	bool	shape_character::point_test(float x, float y)
	// Return true if the specified point is on the interior of our shape.
	{
		if (m_bound.point_test(x, y) == false)
		{
			// Early out.
			return false;
		}

#if 0
		// Try each of the paths.
		for (int i = 0; i < m_paths.size(); i++)
		{
			if (m_paths[i].point_test(x, y))
			{
				return true;
			}
		}

		return false;
#endif // 0
		return true;
	}


	void	shape_character::compute_bound(rect* r) const
	// Find the bounds of this shape, and store them in
	// the given rectangle.
	{
		r->m_x_min = FLT_MAX;
		r->m_y_min = FLT_MAX;
		r->m_x_max = -FLT_MAX;
		r->m_y_max = -FLT_MAX;

		for (int i = 0; i < m_paths.size(); i++)
		{
			const path&	p = m_paths[i];
			r->expand_to_point(p.m_ax, p.m_ay);
			for (int j = 0; j < p.m_edges.size(); j++)
			{
				r->expand_to_point(p.m_edges[j].m_ax, p.m_edges[j].m_ay);
//					r->expand_to_point(p.m_edges[j].m_cx, p.m_edges[j].m_cy);
			}
		}
	}


	void	shape_character::output_cached_data(tu_file* out)
	// Dump our precomputed mesh data to the given stream.
	{
		int	n = m_cached_meshes.size();
		out->write_le32(n);

		for (int i = 0; i < n; i++)
		{
			m_cached_meshes[i]->output_cached_data(out);
		}
	}


	void	shape_character::input_cached_data(tu_file* in)
	// Initialize our mesh data from the given stream.
	{
		int	n = in->read_le32();

		m_cached_meshes.resize(n);

		for (int i = 0; i < n; i++)
		{
			mesh_set*	ms = new mesh_set();
			ms->input_cached_data(in);
			m_cached_meshes[i] = ms;
		}
	}

	
}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
