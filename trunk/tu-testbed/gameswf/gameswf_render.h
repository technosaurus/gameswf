// gameswf_render.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// 2D shape renderer interface.  Takes line/curve segments with style
// info, tesselates them, and draws them using a hardware 3D API.

#ifndef GAMESWF_RENDER_H
#define GAMESWF_RENDER_H


#include "gameswf_types.h"
#include "engine/image.h"


namespace gameswf
{
	namespace render
	{
		struct bitmap_info;

		bitmap_info*	create_bitmap_info(image::rgb* im);
		bitmap_info*	create_bitmap_info(image::rgba* im);
		bitmap_info*	create_bitmap_info_blank();
		void	set_alpha_image(bitmap_info* bi, int w, int h, Uint8* data);	// @@ munges *data!!!
		void	delete_bitmap_info(bitmap_info* bi);

		// Bracket the displaying of a frame from a movie.
		// Fill the background color, and set up default
		// transforms, etc.
		void	begin_display(
			rgba background_color,
			float x0, float x1, float y0, float y1);
		void	end_display();

		// Geometric and color transforms for mesh and line_strip rendering.
		void	set_matrix(const matrix& m);
		void	set_cxform(const cxform& cx);

		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-list order.  The type of the array should
		// be float[vertex_count*2]
		void	draw_mesh(const float coords[], int vertex_count);

		// Draw a line-strip using the current line style.
		// Clear the style list after rendering.
		//
		// Coords is a list of (x,y) coordinate pairs, in
		// sequence.
		void	draw_line_strip(const float coords[], int vertex_count);

		// Set line and fill styles for mesh & line_strip
		// rendering.
		enum bitmap_wrap_mode
		{
			WRAP_REPEAT,
			WRAP_CLAMP
		};
		void	fill_style_disable(int fill_side);
		void	fill_style_color(int fill_side, rgba color);
		void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm);

		void	line_style_disable();
		void	line_style_color(rgba color);
		void	line_style_width(float width);

		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Ignores
		// current transforms.
		void	draw_bitmap(const matrix& m, const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color);

	};	// end namespace render
};	// end namespace gameswf


#endif // GAMESWF_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
