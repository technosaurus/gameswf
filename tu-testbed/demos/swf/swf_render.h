// swf_render.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// 2D shape renderer.  Takes line/curve segments with style info,
// tesselates them, and draws them using OpenGL.

#ifndef SWF_RENDER_H
#define SWF_RENDER_H


namespace swf
{
	namespace render
	{
		// Set line and fill styles.  You may not change line
		// and fill styles within a begin_path()/end_path()
		// pair.
		void	fill_style0(/*...*/);
		void	fill_style1(/*...*/);
		void	line_style(/*...*/);

		// A shape has one or more paths.  The paths in a
		// shape are rasterized together using a typical
		// polygon odd-even rule.
		void	begin_shape();
		void	end_shape();

		// A path is enclosed within a shape.  If fill styles
		// are active, a path should be a closed shape
		// (i.e. the last point should match the first point).
		void	begin_path(float ax, float ay);
		void	add_line_segment(float ax, float ay);
		void	add_curve_segment(float cx, float cy, float ax, float ay);
		void	end_path();
	};
};


#endif // SWF_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
