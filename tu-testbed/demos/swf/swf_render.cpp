// swf_render.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier shape renderer.


#include "swf_render.h"
#include "engine/ogl.h"
#include "engine/utility.h"


namespace swf {
namespace render
{
	// Position of previous anchor point.
	static float	s_x = 0.f;
	static float	s_y = 0.f;

	// Curve subdivision error tolerance (in TWIPs)
	static float	s_tolerance = 20.0f;


	void	begin_shape(float ax, float ay)
	// This call begins drawing a closed shape.  Add segments to
	// the shape using add_curve_segment(), and call end_shape()
	// when you want to close the shape and emit it.
	{
		s_x = ax;
		s_y = ay;

		// xxxx
		glBegin(GL_LINE_STRIP);
		glVertex2f(s_x, s_y);
	}


	void	add_line_segment(float ax, float ay)
	// Add a line segment running from the previous anchor point
	// to the given new anchor point.
	{
		// xxxx
		glVertex2f(ax, ay);
		s_x = ax;
		s_y = ay;
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
		// Subdivide, and add line segments...
		curve(s_x, s_y, cx, cy, ax, ay);
	}


	void	end_shape()
	// Mark the end of a closed shape.
	{
		//xxx
		glEnd();
	}
}};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
