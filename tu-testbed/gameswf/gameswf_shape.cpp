// gameswf_shape.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier outline shapes, the basis for most SWF rendering.


#include "gameswf_shape.h"
#include "gameswf_impl.h"
#include "gameswf_render.h"
#include "gameswf_stream.h"

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

	void	edge::emit_curve() const
	// Send this segment to the renderer.
	{
		gameswf::render::add_curve_segment(m_cx, m_cy, m_ax, m_ay);
	}


	//
	// path
	//


	path::path()
		:
		m_new_shape(false)
	{
		reset(0, 0, -1, -1, -1);
	}

	path::path(float ax, float ay, int fill0, int fill1, int line)
	{
		reset(ax, ay, fill0, fill1, line);
	}


	void	path::reset(float ax, float ay, int fill0, int fill1, int line)
	// Reset all our members to the given values, and clear our edge list.
	{
		m_ax = ax;
		m_ay = ay,
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

		
	void	path::display(
		const display_info& di,
		const array<fill_style>& fill_styles,
		const array<line_style>& line_styles) const
	// Render this path.
	{
		if (m_fill0 > 0)
		{
			fill_styles[m_fill0 - 1].apply(0);
		}
		else 
		{
			gameswf::render::fill_style_disable(0);
		}

		if (m_fill1 > 0)
		{
			fill_styles[m_fill1 - 1].apply(1);
		}
		else
		{
			gameswf::render::fill_style_disable(1);
		}

		if (m_line > 0)
		{
			line_styles[m_line - 1].apply();
		}
		else
		{
			gameswf::render::line_style_disable();
		}

		gameswf::render::begin_path(m_ax, m_ay);
		for (int i = 0; i < m_edges.size(); i++)
		{
			m_edges[i].emit_curve();
		}
		gameswf::render::end_path();
	}

		
	bool	path::point_test(float x, float y)
	// Point-in-shape test.  Return true if the query point is on the filled
	// interior of this shape.
	{
		assert(0);	// this routine isn't finished

		if (m_fill0 == -1 && m_fill1 == -1)
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

		IF_DEBUG(printf("rfs: fsc = %d\n", fill_style_count));

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

		IF_DEBUG(printf("rls: lsc = %d\n", line_style_count));

		// @@ does the 0xFF flag apply to all tag types?
		// if (tag_type > 2)
		// {
			if (line_style_count == 0xFF)
			{
				line_style_count = in->read_u16();
			}
		// }

		IF_DEBUG(printf("rls: lsc2 = %d\n", line_style_count));

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

		IF_DEBUG(printf("scr: nfb = %d, nlb = %d\n", num_fill_bits, num_line_bits));

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

					x = move_x;
					y = move_y;

					// Set the beginning of the path.
					current_path.m_ax = x;
					current_path.m_ay = y;

					if (SHAPE_LOG) IF_DEBUG(printf("scr: moveto %4g %4g\n", x, y));
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
					if (SHAPE_LOG) IF_DEBUG(printf("scr: fill0 = %d\n", current_path.m_fill0));
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
					if (SHAPE_LOG) IF_DEBUG(printf("scr: fill1 = %d\n", current_path.m_fill1));
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
					if (SHAPE_LOG) IF_DEBUG(printf("scr: line = %d\n", current_path.m_line));
				}
				if (flags & 0x10) {
					assert(tag_type >= 22);

					IF_DEBUG(printf("scr: more fill styles\n"));

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

					if (SHAPE_LOG) IF_DEBUG(printf("scr: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", x, y, cx, cy, ax, ay));

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
						dx = in->read_sint(num_bits);
						dy = in->read_sint(num_bits);
					}
					else
					{
						int	vert_flag = in->read_uint(1);
						if (vert_flag == 0) {
							// Horizontal line.
							dx = in->read_sint(num_bits);
						} else {
							// Vertical line.
							dy = in->read_sint(num_bits);
						}
					}

					if (SHAPE_LOG) IF_DEBUG(printf("scr: straight edge = %4g %4g - %4g %4g\n", x, y, x + dx, y + dy));

					current_path.m_edges.push_back(edge(x + dx/2, y + dy/2, x + dx, y + dy));

					x += dx;
					y += dy;
				}
			}
		}
	}


	void	shape_character::display(const display_info& di)
	// Draw the shape using the given environment.
	{
		display(di, m_fill_styles);
	}

		
	void	shape_character::display(const display_info& di, const array<fill_style>& fill_styles) const
	// Display our shape.  Use the fill_styles arg to
	// override our default set of fill styles (e.g. when
	// rendering text).
	{
		gameswf::render::push_apply_matrix(di.m_matrix);
		gameswf::render::push_apply_cxform(di.m_color_transform);

		gameswf::render::begin_shape();
		for (int i = 0; i < m_paths.size(); i++)
		{
			if (m_paths[i].m_new_shape == true)
			{
				// @@ this is awful -- need a better shape loader!!!
				gameswf::render::end_shape();
				gameswf::render::begin_shape();
			}
			else
			{
				m_paths[i].display(di, fill_styles, m_line_styles);
			}
		}
#if 0
		// Show bounding box.
		gameswf::render::line_style_color(rgba(0, 0, 0, 255));
		gameswf::render::fill_style_disable(0);
		gameswf::render::fill_style_disable(1);
		rect	bounds;
		get_bounds(&bounds);
		gameswf::render::begin_path(bounds.m_x_min, bounds.m_y_min);
		gameswf::render::add_line_segment(bounds.m_x_max, bounds.m_y_min);
		gameswf::render::add_line_segment(bounds.m_x_max, bounds.m_y_max);
		gameswf::render::add_line_segment(bounds.m_x_min, bounds.m_y_max);
		gameswf::render::add_line_segment(bounds.m_x_min, bounds.m_y_min);
		gameswf::render::end_path();
#endif // 0
		gameswf::render::end_shape();

		gameswf::render::pop_cxform();
		gameswf::render::pop_matrix();
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


	void	shape_character::get_bounds(rect* r) const
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

	
}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
