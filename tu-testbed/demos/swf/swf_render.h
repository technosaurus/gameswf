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
//		void	fill_style0(....);
//		void	fill_style1(....);
//		void	line_style(...);

		void	begin_shape(float ax, float ay);
		void	add_line_segment(float ax, float ay);
		void	add_curve_segment(float cx, float cy, float ax, float ay);
		void	end_shape();
	};
};


#endif // SWF_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
