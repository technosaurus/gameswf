// swf_render.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier shape renderer.


#include "swf_render.h"
#include "swf_types.h"
#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"
#include <stdlib.h>


namespace swf
{
namespace render
{
	// Curve subdivision error tolerance (in TWIPs)
	static float	s_tolerance = 20.0f;


	struct fill_style
	{
		rgba	m_color;
		bool	m_valid;

		fill_style()
			:
			m_valid(false)
		{
		}

		void	apply() const
		// Push our style into OpenGL.
		{
			assert(m_valid == true);
			m_color.ogl_color();
		}

		void	set_valid(bool valid) { m_valid = valid; }
		bool	is_valid() const { return m_valid; }
	};


	struct fill_segment
	{
		point	m_begin;
		point	m_end;
		fill_style	m_left_style, m_right_style, m_line_style;
//		bool	m_down;	// true if the segment was initially pointing down (rather than up)

		fill_segment() {}

		fill_segment(
			const point& a,
			const point& b,
			const fill_style& left_style,
			const fill_style& right_style,
			const fill_style& line_style)
			:
			m_begin(a),
			m_end(b),
			m_left_style(left_style),
			m_right_style(right_style),
			m_line_style(line_style)
//			m_down(false),
			
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

			// swap fill styles...
			swap(m_left_style, m_right_style);

//			m_down = ! m_down;
		}

		float	get_height() const
		// Return segment height.
		{
			assert(m_end.m_y >= m_begin.m_y);

			return m_end.m_y - m_begin.m_y;
		}
	};


	// Renderer state.
	static array<fill_segment>	s_current_segments;
	static point	s_last_point;
	static fill_style	s_current_left_style;
	static fill_style	s_current_right_style;
	static fill_style	s_current_line_style;
	static bool	s_shape_has_line;	// flag to let us skip the line rendering if no line styles were set when defining the shape.
	static bool	s_shape_has_fill;	// flag to let us skip the fill rendering if no fill styles were set when defining the shape.

	static array<matrix>	s_matrix_stack;
	static array<cxform>	s_cxform_stack;


	static void	peel_off_and_render(int i0, int i1, float y0, float y1);


	void	begin_display(/*background color???*/
		float x0, float x1, float y0, float y1)
	// Set up to render a full frame from a movie.  Sets up
	// necessary transforms, to scale the movie to fit within the
	// given dimensions (and fills the background...).  Call
	// end_display() when you're done.
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glOrtho(x0, x1, y0, y1, -1, 1);

		// Prime the matrix stack with an identity transform.
		assert(s_matrix_stack.size() == 0);
		matrix	identity;
		identity.set_identity();
		s_matrix_stack.push_back(identity);

		// Prime the cxform stack an identity cxform.
		assert(s_cxform_stack.size() == 0);
		cxform	cx_identity;
		s_cxform_stack.push_back(cx_identity);
	}


	void	end_display()
	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	{
		assert(s_matrix_stack.size() == 1);
		s_matrix_stack.resize(0);

		assert(s_cxform_stack.size() == 1);
		s_cxform_stack.resize(0);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}


	void	push_apply_matrix(const matrix& m)
	// Compute the current matrix times the given matrix, and push
	// it onto the top of the matrix stack.  The composite matrix
	// applies to all geometry sent into the renderer, until the
	// next call to pop_matrix().
	{
		assert(s_matrix_stack.size() < 100);	// sanity check.

		matrix	composite = s_matrix_stack.back();
		composite.concatenate(m);
		s_matrix_stack.push_back(composite);
	}


	void	pop_matrix()
	// Pop the previously-pushed matrix.  Restore the transform
	// from before the last push_apply_matrix.
	{
		assert(s_matrix_stack.size() > 0);
		if (s_matrix_stack.size() > 0)
		{
			s_matrix_stack.resize(s_matrix_stack.size() - 1);
		}
	}


	void	push_apply_cxform(const cxform& cx)
	// Apply a color transform to be used in rendering the
	// following styles.  Concatenates the given transform onto
	// any existing transform; the combined cxform applies until
	// the next call to pop_cxform(), when the current cxform
	// reverts to the previous value.
	{
		assert(s_cxform_stack.size() < 100);	// sanity check.

		cxform	composite = s_cxform_stack.back();
		composite.concatenate(cx);
		s_cxform_stack.push_back(composite);
	}

	
	void	pop_cxform()
	// Restore the cxform that was in effect before the last call
	// to push_apply_cxform().
	{
		assert(s_cxform_stack.size() > 0);
		if (s_cxform_stack.size() > 0)
		{
			s_cxform_stack.resize(s_cxform_stack.size() - 1);
		}
	}


	void	fill_style0(bool enable, rgba color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		s_current_left_style.set_valid(enable);
		s_current_left_style.m_color = s_cxform_stack.back().transform(color);
		if (enable == true
		    && s_shape_has_fill == false)
		{
			s_shape_has_fill = true;
		}
	}


	void	fill_style1(bool enable, rgba color)
	// Set fill style for the right interior of the shape.  If
	// enable is false, turn off fill for the right interior.
	{
		s_current_right_style.set_valid(enable);
		s_current_right_style.m_color = s_cxform_stack.back().transform(color);
		if (enable == true
		    && s_shape_has_fill == false)
		{
			s_shape_has_fill = true;
		}
	}


	void	line_style(bool enable, rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		s_current_line_style.set_valid(enable);
		s_current_line_style.m_color = s_cxform_stack.back().transform(color);
		if (enable == true
		    && s_shape_has_line == false)
		{
			s_shape_has_line = true;
		}
	}


	void	begin_shape()
	{
		// ensure we're not already in a shape or path.
		// make sure our path state is cleared out.
		assert(s_current_segments.size() == 0);
		s_current_segments.resize(0);

		s_current_line_style.set_valid(false);
		s_current_left_style.set_valid(false);
		s_current_right_style.set_valid(false);
		s_shape_has_fill = false;
		s_shape_has_line = false;
	}


	static int	compare_segment_y(const void* a, const void* b)
	// For sorting segments by m_begin.m_y, and then by height.
	{
		const fill_segment*	A = (const fill_segment*) a;
		const fill_segment*	B = (const fill_segment*) b;

		const float	ay0 = A->m_begin.m_y;
		const float	by0 = B->m_begin.m_y;

		if (ay0 < by0)
		{
			return -1;
		}
		else if (ay0 == by0)
		{
			const float	ah = A->get_height();
			const float	bh = B->get_height();

			if (ah < bh)
			{
				return -1;
			}
			else if (ah == bh)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}


	static int	compare_segment_x(const void* a, const void* b)
	// For sorting segments by m_begin.m_x, and then by m_end.m_x.
	{
		const fill_segment*	A = (const fill_segment*) a;
		const fill_segment*	B = (const fill_segment*) b;

		const float	ax0 = A->m_begin.m_x;
		const float	bx0 = B->m_begin.m_x;

		if (ax0 < bx0)
		{
			return -1;
		}
		else if (ax0 == bx0)
		{
			const float	ax1 = A->m_end.m_x;
			const float	bx1 = B->m_end.m_x;

			if (ax1 < bx1)
			{
				return -1;
			}
			else if (ax1 == bx1)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}


	void	end_shape()
	{
		if (s_shape_has_line)
		{
			//
			// Draw outlines.
			//

			glBegin(GL_LINES);
			{for (int i = 0; i < s_current_segments.size(); i++)
			{
				const fill_segment&	seg = s_current_segments[i];
				if (seg.m_line_style.is_valid())
				{
					seg.m_line_style.apply();
					glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
					glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
				}
			}}
			glEnd();
		}

// wireframe, for debugging
#if 0
		{
			glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
			glBegin(GL_LINES);
			{for (int i = 0; i < s_current_segments.size(); i++)
			{
				const fill_segment&	seg = s_current_segments[i];
				glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
				glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
			}}
			glEnd();
		}
#endif 

		if (s_shape_has_fill == true)
		{
			//
			// Draw the filled shape.
			//

#if 1
			// Draw outlines as antialiased lines, for
			// antialiasing effect.  This actually works
			// half-decently!  Needs a few tweaks: when
			// the fill mode has fractional alpha, the
			// line needs to be drawn with severely
			// reduced alpha, to avoid a halo effect.
			// Doesn't seem to work on some shapes
			// (inside-out ones?)
			glBegin(GL_LINES);
			for (int i = 0; i < s_current_segments.size(); i++)
			{
				const fill_segment&	seg = s_current_segments[i];
				if (seg.m_left_style.is_valid())
				{
					seg.m_left_style.apply();
					glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
					glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
				}
				else if (seg.m_right_style.is_valid())
				{
					seg.m_right_style.apply();
					glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
					glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
				}
			}
			glEnd();
#endif // 1		


			// sort by begining y (smaller first), then by height (shorter first)
			qsort(&s_current_segments[0], s_current_segments.size(), sizeof(s_current_segments[0]), compare_segment_y);
		
			int	base = 0;
			while (base < s_current_segments.size())
			{
				float	ytop = s_current_segments[base].m_begin.m_y;
				int	next_base = base + 1;
				for (;;)
				{
					if (next_base == s_current_segments.size()
					    || s_current_segments[next_base].m_begin.m_y > ytop)
					{
						break;
					}
					next_base++;
				}

				// sort this first part again by y
				qsort(&s_current_segments[base], next_base - base, sizeof(s_current_segments[0]), compare_segment_y);

				// s_current_segments[base] through s_current_segments[next_base - 1] is all the segs that start at ytop
				if (next_base >= s_current_segments.size()
				    || s_current_segments[base].m_end.m_y <= s_current_segments[next_base].m_begin.m_y)
				{
					// No segments start between ytop and
					// [base].m_end.m_y, so we can peel
					// off that whole interval and render
					// it right away.
					float	ybottom = s_current_segments[base].m_end.m_y;
					peel_off_and_render(base, next_base, ytop, ybottom);

					while (base < s_current_segments.size()
					       && s_current_segments[base].m_end.m_y <= ybottom)
					{
						base++;
					}
				}
				else
				{
					float	ybottom = s_current_segments[next_base].m_begin.m_y;
					assert(ybottom > ytop);
					peel_off_and_render(base, next_base, ytop, ybottom);

					// don't update base; it's still active.
				}
			}
		}
		
		s_current_segments.resize(0);
	}


	void	peel_off_and_render(int i0, int i1, float y0, float y1)
	// Clip the interval [y0, y1] off of the segments from s_current_segments[i0 through (i1-1)]
	// and the clipped segments.  Modifies the values in s_current_segments.
	{
		if (y0 == y1)
		{
			// Don't bother doing any work...
			return;
		}

		// Peel off first.
		array<fill_segment>	slab;
		for (int i = i0; i < i1; i++)
		{
			fill_segment*	f = &s_current_segments[i];
			assert(f->m_begin.m_y == y0);
			assert(f->m_end.m_y >= y1);

			float	dy = f->m_end.m_y - f->m_begin.m_y;
			float	t = 1.0f;
			if (dy > 0)
			{
				t = (y1 - f->m_begin.m_y) / dy;
			}
			point	intersection;
			intersection.m_y = y1;
			intersection.m_x = f->m_begin.m_x + (f->m_end.m_x - f->m_begin.m_x) * t;

			// Peel off.
			slab.push_back(*f);
			slab.back().m_end = intersection;

			// Modify segment.
			s_current_segments[i].m_begin = intersection;
		}

		// Sort by x.
		qsort(&slab[0], slab.size(), sizeof(slab[0]), compare_segment_x);

		glBegin(GL_QUADS);
//		glBegin(GL_LINE_STRIP);	//xxxxxxxx
//		glBegin(GL_LINES);	//xxxxxxxx
		// Render pairs.
		// @@ need to deal with self-intersection?
		{for (int i = 0; i < slab.size() - 1; i++)
		{
			if (slab[i].m_left_style.is_valid())
			{
				// assert(slab[i + 1].m_right_style == slab[i].m_left_style);	//????

				slab[i].m_left_style.apply();

				glVertex2f(slab[i].m_begin.m_x, slab[i].m_begin.m_y);
				glVertex2f(slab[i].m_end.m_x, slab[i].m_end.m_y);
				glVertex2f(slab[i + 1].m_end.m_x, slab[i + 1].m_end.m_y);
				glVertex2f(slab[i + 1].m_begin.m_x, slab[i + 1].m_begin.m_y);
			}
		}}
		glEnd();
	}


	void	begin_path(float ax, float ay)
	// This call begins drawing a closed shape.  Add segments to
	// the shape using add_curve_segment(), and call end_shape()
	// when you want to close the shape and emit it.
	{
		s_matrix_stack.back().transform(&s_last_point, point(ax, ay));
	}


	static void	add_line_segment_raw(const point& p)
	// INTERNAL: Add a line segment running from the previous
	// anchor point to the given new anchor point -- (ax, ay) is
	// already transformed.
	{
		s_current_segments.push_back(
			fill_segment(
				s_last_point,
				p,
				s_current_left_style,
				s_current_right_style,
				s_current_line_style));
		s_last_point = p;
	}


	void	add_line_segment(float ax, float ay)
	// Add a line running from the previous anchor point to the
	// given new anchor point.
	{
		point	p;
		s_matrix_stack.back().transform(&p, point(ax, ay));
		add_line_segment_raw(p);
	}


	static void	curve(float p0x, float p0y, float p1x, float p1y, float p2x, float p2y)
	// Recursive routine to generate bezier curve within tolerance.
	{
#ifndef NDEBUG
		static int	recursion_count = 0;
		recursion_count++;
		if (recursion_count > 500)
		{
			assert(0);	// probably a bug!
		}
#endif // not NDEBUG

		// @@ use struct point in here?

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
			add_line_segment_raw(point(qx, qy));
			add_line_segment_raw(point(p2x, p2y));
		}
		else
		{
			// Subdivide.
			curve(p0x, p0y, (p0x + p1x) * 0.5f, (p0y + p1y) * 0.5f, qx, qy);
			curve(qx, qy, (p1x + p2x) * 0.5f, (p1y + p2y) * 0.5f, p2x, p2y);
		}

#ifndef NDEBUG
		recursion_count--;
#endif // not NDEBUG
	}

	
	void	add_curve_segment(float cx, float cy, float ax, float ay)
	// Add a curve segment to the shape.  The curve segment is a
	// quadratic bezier, running from the previous anchor point to
	// the given new anchor point (ax, ay), with (cx, cy) acting
	// as the control point in between.
	{
		// Subdivide, and add line segments...
		point	c, a;
		s_matrix_stack.back().transform(&c, point(cx, cy));
		s_matrix_stack.back().transform(&a, point(ax, ay));
		curve(s_last_point.m_x, s_last_point.m_y, c.m_x, c.m_y, a.m_x, a.m_y);
	}


	void	end_path()
	// Mark the end of a closed shape.
	{
	}


}};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
