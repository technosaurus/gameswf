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

		// Handle geometric and color transforms.  The
		// renderer has a matrix stack; to set a transform you
		// push_apply it, and then pop it when you're done.
		// The cumulative stack of transforms applies to all
		// colors and geometry rendered while the transforms
		// are in effect.
		void	push_apply_matrix(const matrix& m);
		void	pop_matrix();
		void	push_apply_cxform(const cxform& cx);
		void	pop_cxform();

		// A shape has one or more paths.  The paths in a
		// shape are rasterized together using a typical
		// polygon odd-even rule.
		void	begin_shape();
		void	end_shape();

		// Set line and fill styles.  You may change line and
		// fill styles outside a begin_path()/end_path() pair.
		void	fill_style_disable(int fill_side);
		void	fill_style_color(int fill_side, rgba color);

		enum bitmap_wrap_mode
		{
			WRAP_REPEAT,
			WRAP_CLAMP
		};
		void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm);

		void	line_style_disable();
		void	line_style_color(rgba color);
		void	line_style_width(float width);

		// A path is enclosed within a shape.  If fill styles
		// are active, a path should be a closed shape
		// (i.e. the last point should match the first point).
		// Set your styles before rendering the path; all
		// segments in a path must have the same styles.
		void	begin_path(float ax, float ay);
		void	add_line_segment(float ax, float ay);
		void	add_curve_segment(float cx, float cy, float ax, float ay);
		void	end_path();

		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Applies
		// current transforms to points and color.
		void	draw_bitmap(const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color);

		// Some hacky stuff for use by the fontlib texture-cacher.
		// Basically redirects rendering output to a RAM byte array.
		void	software_mode_enable(int width, int height);
		void	software_mode_disable();
		Uint8*	get_software_mode_buffer();

	};	// end namespace render
};	// end namespace gameswf


#endif // GAMESWF_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
