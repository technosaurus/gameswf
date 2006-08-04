// gameswf_tesselate.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// 2D shape tesselator.  Actually a "trapezoidizer".  Takes line/curve
// segments with a style index, and outputs trapezoids.


#ifndef GAMESWF_TESSELATE_H
#define GAMESWF_TESSELATE_H


#include "gameswf.h"
#include "gameswf_types.h"


namespace gameswf
{
	namespace tesselate
	{
		struct trapezoid
		{
			float	m_y0, m_y1;
			float	m_lx0, m_lx1, m_rx0, m_rx1;
		};

		// Interface for receiving the results of
		// trapezoidation.
		struct trapezoid_accepter
		{
			virtual void	accept_trapezoid(int style, const trapezoid& tr) = 0;
			virtual void	accept_line_strip(int style, const point coords[], int coord_count) = 0;
		};

		// A shape has one or more paths.  The paths in a
		// shape are tesselated together using a typical
		// polygon odd-even rule.
		//
		// The error tolerance tells the tesselator how much
		// geometric error is allowed along curve edges.
		void	begin_shape(trapezoid_accepter* accepter, float curve_error_tolerance);
		void	end_shape();

		// A path is enclosed within a shape.  If fill styles
		// are active, a path should be a closed shape
		// (i.e. the last point should match the first point).
		// Set your styles before rendering the path; all
		// segments in a path must have the same styles.
		void	begin_path(int style_left, int style_right, int line_style, float ax, float ay);
		void	add_line_segment(float ax, float ay);
		void	add_curve_segment(float cx, float cy, float ax, float ay);
		void	end_path();

	};	// end namespace tesselate

	namespace tesselate_new {
		struct mesh_accepter {
			// TODO: too much array copying!  Fix it.
			// Accepter should be able to fill its buffer
			// triangle-by-triangle via a callback from
			// within the tesselator.  We should also add
			// a result size hint function so the accepter
			// can reserve the right amount of output
			// space.
			virtual void accept_trilist(int style, const point trilist[], int point_count) = 0;
			virtual void accept_line_strip(int style, const point coords[], int point_count) = 0;
		};

		void	begin_shape(mesh_accepter* accepter, float curve_error_tolerance);
		void	end_shape();

		// A path is a subpart of a shape, having a consistent style.
		void	begin_path(int style_left, int style_right, int line_style, float ax, float ay);
		void	add_line_segment(float ax, float ay);
		void	add_curve_segment(float cx, float cy, float ax, float ay);
		void	end_path();
	} // end namespace tesselate_new

};	// end namespace gameswf


#endif // GAMESWF_TESSELATE_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
