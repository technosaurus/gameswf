// swf_render.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier shape renderer.


#include "swf_render.h"
#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"


namespace swf {
namespace render
{
	// Curve subdivision error tolerance (in TWIPs)
	static float	s_tolerance = 20.0f;


	// current line style
	bool	s_shape_is_line = true;

	// current fill styles (texture_id, color, gradient)


	static bool	is_shape_convex();


	struct point
	{
		float	m_x, m_y;	// should probably use integer TWIPs...

		point() : m_x(0), m_y(0) {}
		point(float x, float y) : m_x(x), m_y(y) {}
	};


	struct fill_segment
	{
		// m_fill_style_left, m_fill_style_right
		point	m_begin;
		point	m_end;

		fill_segment() {}

		fill_segment(const point& a, const point& b)
			:
			m_begin(a),
			m_end(b)
		{
			// For rasterization, we want to ensure that
			// the segment always points towards positive
			// y...
			if (m_begin.m_y > m_end.m_y)
			{
				flip();
			}
		}

		void	flip()
		// Exchange end points, and reverse fill sides.
		{
			swap(m_begin, m_end);

			// swap fill_styles...
		}
	};

	static array<fill_segment>	s_current_segments;
	static point	s_last_point;

//	static array<point>	s_current_path;


	void	fill_style0()
	// Set fill style for the left interior of the shape.
	{
		s_shape_is_line = false;
	}


	void	fill_style1()
	// Set fill style for the right interior of the shape.
	{
		s_shape_is_line = false;
	}


	void	line_style()
	// Set the line style of the shape.
	{
		s_shape_is_line = true;
	}


	void	begin_shape()
	{
		// ensure we're not already in a shape or path.
		// make sure our path state is cleared out.
		assert(s_current_segments.size() == 0);
		s_current_segments.resize(0);
	}


	void	end_shape()
	{
		// rasterize the path segments

		// sort by begining y

		// chop segments on y boundaries -- scan downwards,
		// chopping as we go?  Should we copy into an
		// array-of-arrays?  Or use a linked list?

		// for each horizontal band:
		//   for segments in band:
		//     set fill style of seg[i].left
		//     draw a quad with seg[i] and seg[i+1]

		// render lines (on top of existing filled shapes)

		// @@ stopgap: wireframe.
		glBegin(GL_LINES);
		{for (int i = 0; i < s_current_segments.size(); i++)
		{
			glVertex2f(s_current_segments[i].m_begin.m_x, s_current_segments[i].m_begin.m_y);
			glVertex2f(s_current_segments[i].m_end.m_x, s_current_segments[i].m_end.m_y);
		}}
		glEnd();
		
		s_current_segments.resize(0);
	}


	void	begin_path(float ax, float ay)
	// This call begins drawing a closed shape.  Add segments to
	// the shape using add_curve_segment(), and call end_shape()
	// when you want to close the shape and emit it.
	{
//		assert(s_current_path.size() == 0);
//		s_current_path.resize(0);

//		s_current_path.push_back(point(ax, ay));
		s_last_point.m_x = ax;
		s_last_point.m_y = ay;
	}


	void	add_line_segment(float ax, float ay)
	// Add a line segment running from the previous anchor point
	// to the given new anchor point.
	{
//		s_current_path.push_back(point(ax, ay));
		point	p(ax, ay);
		s_current_segments.push_back(fill_segment(s_last_point, p));
		s_last_point = p;
	}


	static void	curve(float p0x, float p0y, float p1x, float p1y, float p2x, float p2y)
	// Recursive routine to generate bezier curve within tolerance.
	{
		// Midpoint on line between two endpoints.
		float	midx = (p0x + p2x) * 0.5f;
		float	midy = (p0y + p2y) * 0.5f;

		// Midpoint on the curve.
		float	qx = (midx + p1x) * 0.5f;
		float	qy = (midy + p1y) * 0.5f;

		float	dist = fabsf(midx - qx) + fabsf(midy - qy);

		if (dist < s_tolerance)
		{
			// Emit edges.
			add_line_segment(qx, qy);
			add_line_segment(p2x, p2y);
		}
		else
		{
			// Subdivide.
			curve(p0x, p0y, (p0x + p1x) * 0.5f, (p0y + p1y) * 0.5f, qx, qy);
			curve(qx, qy, (p1x + p2x) * 0.5f, (p1y + p2y) * 0.5f, p2x, p2y);
		}
	}

	
	void	add_curve_segment(float cx, float cy, float ax, float ay)
	// Add a curve segment to the shape.  The curve segment is a
	// quadratic bezier, running from the previous anchor point to
	// the given new anchor point (ax, ay), with (cx, cy) acting
	// as the control point in between.
	{
//		if (s_current_path.size() > 0)
		if (1)
		{
//			const point&	a = s_current_path.back();
			// Subdivide, and add line segments...
			curve(s_last_point.m_x, s_last_point.m_y, cx, cy, ax, ay);
		}
		else
		{
			assert(0);
		}
	}


	void	end_path()
	// Mark the end of a closed shape.
	{
#if 0
		if (s_current_path.size() > 0)
		{
			if (s_shape_is_line == false
			    && is_shape_convex() == true)
			{
				// Bogus fill.
				glBegin(GL_TRIANGLE_FAN);

				for (int i = 0; i < s_current_path.size(); i++)
				{
					glVertex2f(s_current_path[i].m_x, s_current_path[i].m_y);
				}

				glEnd();
			}
			else
			{
				// Wireframe.
				glBegin(GL_LINE_STRIP);
				{for (int i = 0; i < s_current_path.size(); i++)
				{
					glVertex2f(s_current_path[i].m_x, s_current_path[i].m_y);
				}}

				glEnd();
			}

			s_current_path.resize(0);
		}
#endif // 0
	}


#if 0	
	static bool	is_shape_convex()
	// Examine s_current_path and return true if it's convex.
	// I.e., if all the angles turn the same direction.
	{
		// @@ this is a slightly dumb way to check convexity
		// (using acos), but it does have the advantage that
		// we can check for self intersection by looking at
		// the total angle.

		if (s_current_path.size() <= 0)
		{
			return true;
		}

		float	total_angle = 0;

		for (int i = 0; i < s_current_path.size() - 2; i++)
		{
			vec3	a(s_current_path[i].m_x, s_current_path[i].m_y, 0.0f);
			vec3	b(s_current_path[i + 1].m_x, s_current_path[i + 1].m_y, 0.0f);
			vec3	c(s_current_path[i + 2].m_x, s_current_path[i + 2].m_y, 0.0f);

			vec3	e0 = b - a;
			vec3	e1 = c - b;
			e0.normalize();	// augh, need to handle zero-length segments...
			e1.normalize();

			float	angle = asinf((e0.cross(e1)).z);

			if ((total_angle < 0.0f && angle > 0.0001f)
			    || (total_angle > 0.0f && angle < -0.0001f))
			{
				// Non-convex turn.
				return false;
			}
			total_angle += angle;
		}

		// Check for self-intersection.  Not foolproof, since
		// we're not checking the last segment.
		if (total_angle > 2.0f * M_PI + 0.01f)
		{
			return false;
		}

		return true;
	}
#endif // 0
}};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
