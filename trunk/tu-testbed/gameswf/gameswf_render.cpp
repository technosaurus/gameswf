// gameswf_render.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Interface to back-end renderer.


#include "gameswf_render.h"
#include "gameswf_log.h"


namespace gameswf 
{
	static render_handler* s_render_handler;

	void set_render_handler(render_handler* r)
	{
		s_render_handler = r;
	}

	render_handler* get_render_handler()
	{
		return s_render_handler;
	}


	namespace render
	{
		bitmap_info*	create_bitmap_info(image::rgb* im)
		{
			if (s_render_handler) return s_render_handler->create_bitmap_info(im);
			else return NULL;
		}

		bitmap_info*	create_bitmap_info(image::rgba* im)
		{
			if (s_render_handler) return s_render_handler->create_bitmap_info(im);
			else return NULL;
		}

		bitmap_info*	create_bitmap_info_blank()
		{
			if (s_render_handler) return s_render_handler->create_bitmap_info_blank();
			else return NULL;
		}

		void	set_alpha_image(bitmap_info* bi, int w, int h, Uint8* data)
		// @@ munges *data!!!
		{
			if (s_render_handler) s_render_handler->set_alpha_image(bi, w, h, data);
		}

		void	delete_bitmap_info(bitmap_info* bi)
		{
			if (s_render_handler) s_render_handler->delete_bitmap_info(bi);
		}

		// Bracket the displaying of a frame from a movie.
		// Fill the background color, and set up default
		// transforms, etc.
		void	begin_display(
			rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1)
		{
			if (s_render_handler)
			{
				s_render_handler->begin_display(
					background_color, viewport_x0, viewport_y0,
					viewport_width, viewport_height,
					x0, x1, y0, y1);
			}
			else
			{
				log_error("begin_display called, but no render_handler was registered by the app!\n");
			}
		}


		void	end_display()
		{
			if (s_render_handler) s_render_handler->end_display();
		}


		// Geometric and color transforms for mesh and line_strip rendering.
		void	set_matrix(const matrix& m)
		{
			if (s_render_handler) s_render_handler->set_matrix(m);
		}
		void	set_cxform(const cxform& cx)
		{
			if (s_render_handler) s_render_handler->set_cxform(cx);
		}

		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-list order.  The type of the array should
		// be float[vertex_count*2]
		void	draw_mesh(const float coords[], int vertex_count)
		{
			if (s_render_handler) s_render_handler->draw_mesh(coords, vertex_count);
		}

		// Draw a line-strip using the current line style.
		// Clear the style list after rendering.
		//
		// Coords is a list of (x,y) coordinate pairs, in
		// sequence.
		void	draw_line_strip(const float coords[], int vertex_count)
		{
			if (s_render_handler) s_render_handler->draw_line_strip(coords, vertex_count);
		}

// 		// Set line and fill styles for mesh & line_strip
// 		// rendering.
// 		enum bitmap_wrap_mode
// 		{
// 			WRAP_REPEAT,
// 			WRAP_CLAMP
// 		};

		void	fill_style_disable(int fill_side)
		{
			if (s_render_handler) s_render_handler->fill_style_disable(fill_side);
		}

		void	fill_style_color(int fill_side, rgba color)
		{
			if (s_render_handler) s_render_handler->fill_style_color(fill_side, color);
		}

		void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, render_handler::bitmap_wrap_mode wm)
		{
			if (s_render_handler) s_render_handler->fill_style_bitmap(fill_side, bi, m, wm);
		}

		void	line_style_disable()
		{
			if (s_render_handler) s_render_handler->line_style_disable();
		}

		void	line_style_color(rgba color)
		{
			if (s_render_handler) s_render_handler->line_style_color(color);
		}

		void	line_style_width(float width)
		{
			if (s_render_handler) s_render_handler->line_style_width(width);
		}

		void	begin_submit_mask()
		{
			if (s_render_handler) s_render_handler->begin_submit_mask();
		}

		void	end_submit_mask()
		{
			if (s_render_handler) s_render_handler->end_submit_mask();
		}

		void	disable_mask()
		{
			if (s_render_handler) s_render_handler->disable_mask();
		}

		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Ignores
		// current transforms.
		void	draw_bitmap(const matrix& m, const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color)
		{
			if (s_render_handler)
			{
				s_render_handler->draw_bitmap(
					m, bi, coords, uv_coords, color);
			}
		}
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
