// gameswf_render_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier shape renderer for gameswf, with color & bitmap
// support.

// OpenGL version.


#include "gameswf_render.h"

#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"
#include "gameswf_types.h"
#include "gameswf_tesselate.h"
#include <stdlib.h>


namespace gameswf
{
namespace render
{
	// Some renderer state.

	// Enable/disable antialiasing.
	static bool	s_enable_antialias = true;

	// Output size.
	static float	s_display_width;
	static float	s_display_height;

	static matrix	s_current_matrix;
	static cxform	s_current_cxform;


	void	make_next_miplevel(int* width, int* height, Uint8* data)
	// Utility.  Mutates *width, *height and *data to create the
	// next mip level.
	{
		assert(width);
		assert(height);
		assert(data);

		int	new_w = *width >> 1;
		int	new_h = *height >> 1;
		if (new_w < 1) new_w = 1;
		if (new_h < 1) new_h = 1;
		
		if (new_w * 2 != *width  || new_h * 2 != *height)
		{
			// Image can't be shrunk along (at least) one
			// of its dimensions, so don't bother
			// resampling.  Technically we should, but
			// it's pretty useless at this point.  Just
			// change the image dimensions and leave the
			// existing pixels.
		}
		else
		{
			// Resample.  Simple average 2x2 --> 1, in-place.
			for (int j = 0; j < new_h; j++) {
				Uint8*	out = ((Uint8*) data) + j * new_w;
				Uint8*	in = ((Uint8*) data) + (j << 1) * *width;
				for (int i = 0; i < new_w; i++) {
					int	a;
					a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
					*(out) = a >> 2;
					out++;
					in += 2;
				}
			}
		}

		// Munge parameters to reflect the shrunken image.
		*width = new_w;
		*height = new_h;
	}


	// A struct used to hold info about an OpenGL texture.
	struct bitmap_info
	{
		unsigned int	m_texture_id;	// OpenGL texture id
		int	m_original_width;
		int	m_original_height;

		enum create_empty
		{
			empty
		};

		bitmap_info(create_empty e)
			:
			m_texture_id(0),
			m_original_width(0),
			m_original_height(0)
		{
			// A null texture.  Needs to be initialized later.
		}

		bitmap_info(image::rgb* im)
			:
			m_texture_id(0),
			m_original_width(0),
			m_original_height(0)
		{
			assert(im);

			// Create the texture.

			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &m_texture_id);
			glBindTexture(GL_TEXTURE_2D, m_texture_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);
		
			m_original_width = im->m_width;
			m_original_height = im->m_height;

			int	w = 1; while (w < im->m_width) { w <<= 1; }
			int	h = 1; while (h < im->m_height) { h <<= 1; }

			image::rgb*	rescaled = image::create_rgb(w, h);
			image::resample(rescaled, 0, 0, w - 1, h - 1,
					im, 0, 0, im->m_width, im->m_height);
		
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rescaled->m_data);
//			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_image->m_width, m_image->m_height, GL_RGB, GL_UNSIGNED_BYTE, m_image->m_data);

			delete rescaled;
		}


		bitmap_info(image::rgba* im)
			:
			m_texture_id(0),
			m_original_width(0),
			m_original_height(0)
		// Version of the constructor that takes an image with alpha.
		{
			assert(im);

			// Create the texture.

			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &m_texture_id);
			glBindTexture(GL_TEXTURE_2D, m_texture_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);
		
			m_original_width = im->m_width;
			m_original_height = im->m_height;

			int	w = 1; while (w < im->m_width) { w <<= 1; }
			int	h = 1; while (h < im->m_height) { h <<= 1; }

			if (w != im->m_width
			    || h != im->m_height)
			{
				image::rgba*	rescaled = image::create_rgba(w, h);
				image::resample(rescaled, 0, 0, w - 1, h - 1,
						im, 0, 0, im->m_width, im->m_height);
		
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rescaled->m_data);

				delete rescaled;
			}
			else
			{
				// Use original image directly.
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im->m_data);
			}
		}


		void	set_alpha_image(int width, int height, Uint8* data)
		// Initialize this bitmap_info to an alpha image
		// containing the specified data (1 byte per texel).
		//
		// !! Munges *data in order to create mipmaps !!
		{
			assert(m_texture_id == 0);	// only call this on an empty bitmap_info
			assert(data);
			
			// Create the texture.

			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &m_texture_id);
			glBindTexture(GL_TEXTURE_2D, m_texture_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
			m_original_width = width;
			m_original_height = height;

#ifndef NDEBUG
			// You must use power-of-two dimensions!!
			int	w = 1; while (w < width) { w <<= 1; }
			int	h = 1; while (h < height) { h <<= 1; }
			assert(w == width);
			assert(h == height);
#endif // not NDEBUG

			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);

			// Build mips.
			int	level = 1;
			while (width > 1 || height > 1)
			{
				make_next_miplevel(&width, &height, data);
				glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
				level++;
			}
		}

	};


	struct fill_style
	{
		enum mode
		{
			INVALID,
			COLOR,
			BITMAP_WRAP,
			BITMAP_CLAMP,
			LINEAR_GRADIENT,
			RADIAL_GRADIENT,
		};
		mode	m_mode;
		rgba	m_color;
		const bitmap_info*	m_bitmap_info;
		matrix	m_bitmap_matrix;
		cxform	m_bitmap_color_transform;

		fill_style()
			:
			m_mode(INVALID)
		{
		}

		void	apply(/*const matrix& current_matrix*/) const
		// Push our style into OpenGL.
		{
			assert(m_mode != INVALID);

			if (m_mode == COLOR)
			{
				m_color.ogl_color();
				glDisable(GL_TEXTURE_2D);
			}
			else if (m_mode == BITMAP_WRAP
				 || m_mode == BITMAP_CLAMP)
			{
				assert(m_bitmap_info != NULL);

				m_color.ogl_color();

				if (m_bitmap_info == NULL)
				{
					glDisable(GL_TEXTURE_2D);
				}
				else
				{
					// Set up the texture for rendering.

					{
						// For the moment we can only handle the modulate part of the
						// color transform...
						// How would we handle any additive part?  Realistically we
						// need to use a pixel shader.  Although there is a GL_COLOR_SUM
						// extension that can set an offset for R,G,B (but apparently not A).
						glColor4f(m_bitmap_color_transform.m_[0][0],
							  m_bitmap_color_transform.m_[1][0],
							  m_bitmap_color_transform.m_[2][0],
							  m_bitmap_color_transform.m_[3][0]
							  );
					}

					glBindTexture(GL_TEXTURE_2D, m_bitmap_info->m_texture_id);
					glEnable(GL_TEXTURE_2D);
					glEnable(GL_TEXTURE_GEN_S);
					glEnable(GL_TEXTURE_GEN_T);
				
					if (m_mode == BITMAP_CLAMP)
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					}
					else
					{
						assert(m_mode == BITMAP_WRAP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}

					// Set up the bitmap matrix for texgen.

					float	inv_width = 1.0f / m_bitmap_info->m_original_width;
					float	inv_height = 1.0f / m_bitmap_info->m_original_height;

					const matrix&	m = m_bitmap_matrix;
					glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					float	p[4] = { 0, 0, 0, 0 };
					p[0] = m.m_[0][0] * inv_width;
					p[1] = m.m_[0][1] * inv_width;
					p[3] = m.m_[0][2] * inv_width;
					glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

					glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					p[0] = m.m_[1][0] * inv_height;
					p[1] = m.m_[1][1] * inv_height;
					p[3] = m.m_[1][2] * inv_height;
					glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
				}
			}
		}

		void	disable() { m_mode = INVALID; }
		void	set_color(rgba color) { m_mode = COLOR; m_color = color; }
		void	set_bitmap(const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm, const cxform& color_transform)
		{
			m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
			m_color = rgba();
			m_bitmap_info = bi;
			m_bitmap_matrix = m;
			m_bitmap_color_transform = color_transform;
		}
		bool	is_valid() const { return m_mode != INVALID; }
	};


	// Style state.
	enum style_index
	{
		LEFT_STYLE = 0,
		RIGHT_STYLE,
		LINE_STYLE,

		STYLE_COUNT
	};
	static fill_style	s_current_styles[STYLE_COUNT];


	bitmap_info*	create_bitmap_info(image::rgb* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		return new bitmap_info(im);
	}


	bitmap_info*	create_bitmap_info(image::rgba* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_style_bitmap(), to set a
	// bitmap fill style.
	//
	// This version takes an image with an alpha channel.
	{
		return new bitmap_info(im);
	}


	bitmap_info*	create_bitmap_info_blank()
	// Creates and returns an empty bitmap_info structure.  Image data
	// can be bound to this info later, via set_alpha_image().
	{
		return new bitmap_info(bitmap_info::empty);
	}


	void	set_alpha_image(bitmap_info* bi, int w, int h, Uint8* data)
	// Set the specified bitmap_info so that it contains an alpha
	// texture with the given data (1 byte per texel).
	//
	// Munges *data (in order to make mipmaps)!!
	{
		assert(bi);

		bi->set_alpha_image(w, h, data);
	}


	void	delete_bitmap_info(bitmap_info* bi)
	// Delete the given bitmap info struct.
	{
		delete bi;
	}


	void	begin_display(
		rgba background_color,
		float x0, float x1, float y0, float y1)
	// Set up to render a full frame from a movie.  Sets up
	// necessary transforms, to scale the movie to fit within the
	// given dimensions (and fills the background...).  Call
	// end_display() when you're done.
	//
	// The rectangel (x0, y0, x1, y1) is in pixel coordinates.
	{
		s_display_width = fabsf(x1 - x0);
		s_display_height = fabsf(y1 - y0);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glOrtho(x0, x1, y0, y1, -1, 1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// GL_MODULATE

		glDisable(GL_TEXTURE_2D);

		// Clear the background, if background color has alpha > 0.
		if (background_color.m_a > 0.0f)
		{
			// Draw a big quad.
			background_color.ogl_color();
			glBegin(GL_QUADS);
			glVertex2f(x0, y0);
			glVertex2f(x1, y0);
			glVertex2f(x1, y1);
			glVertex2f(x0, y1);
			glEnd();
		}

// Old unused code.  Might get revived someday.
#if 0
		// See if we want to, and can, use multitexture
		// antialiasing.
		s_multitexture_antialias = false;
		if (s_enable_antialias)
		{
			int	tex_units = 0;
			glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &tex_units);
			if (tex_units >= 2)
			{
				s_multitexture_antialias = true;
			}

			// Make sure we have an edge texture available.
			if (s_multitexture_antialias == true
			    && s_edge_texture_id == 0)
			{
				// Very simple texture: 2 texels wide, 1 texel high.
				// Both texels are white; left texel is all clear, right texel is all opaque.
				unsigned char	edge_data[8] = { 255, 255, 255, 0, 255, 255, 255, 255 };

				ogl::active_texture(GL_TEXTURE1_ARB);
				glEnable(GL_TEXTURE_2D);
				glGenTextures(1, &s_edge_texture_id);
				glBindTexture(GL_TEXTURE_2D, s_edge_texture_id);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, edge_data);

				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// @@ should we use a 1D texture???

				glDisable(GL_TEXTURE_2D);
				ogl::active_texture(GL_TEXTURE0_ARB);
				glDisable(GL_TEXTURE_2D);
			}
		}
#endif // 0
	}


	void	end_display()
	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	{
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}


	void	set_matrix(const matrix& m)
	// Set the current transform for mesh & line-strip rendering.
	{
		s_current_matrix = m;
	}


	void	set_cxform(const cxform& cx)
	// Set the current color transform for mesh & line-strip rendering.
	{
		s_current_cxform = cx;
	}


	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		assert(fill_side >= 0 && fill_side < 2);

		s_current_styles[fill_side].disable();
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		s_current_styles[LINE_STYLE].disable();
	}


	void	fill_style_color(int fill_side, rgba color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		assert(fill_side >= 0 && fill_side < 2);

		s_current_styles[fill_side].set_color(s_current_cxform.transform(color));
	}


	void	line_style_color(rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		s_current_styles[LINE_STYLE].set_color(s_current_cxform.transform(color));
	}


	void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm)
	{
		assert(fill_side >= 0 && fill_side < 2);
		s_current_styles[fill_side].set_bitmap(bi, m, wm, s_current_cxform);
	}


	void	draw_mesh(const float coords[], int vertex_count)
	{
		// Set up current style.
		matrix	ident;
		ident.set_identity();
		s_current_styles[LEFT_STYLE].apply();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		s_current_matrix.ogl_multiply();

		// Send the tris to OpenGL
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
		glDrawArrays(GL_TRIANGLES, 0, vertex_count);
		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
	}


	void	draw_line_strip(const float coords[], int vertex_count)
	// Draw the line strip formed by the sequence of points.
	{
		// Set up current style.
		matrix	ident;
		ident.set_identity();
		s_current_styles[LINE_STYLE].apply();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		s_current_matrix.ogl_multiply();

		// Send the line-strip to OpenGL
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
		glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
	}


	void	draw_bitmap(const matrix& m, const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color)
	// Draw a rectangle textured with the given bitmap, with the
	// given color.  Apply given transform; ignore any currently
	// set transforms.
	//
	// Intended for textured glyph rendering.
	{
		assert(bi);

//		color = s_cxform_stack.back().transform(color);
		color.ogl_color();

		point a, b, c, d;
		m.transform(&a, point(coords.m_x_min, coords.m_y_min));
		m.transform(&b, point(coords.m_x_max, coords.m_y_min));
		m.transform(&c, point(coords.m_x_min, coords.m_y_max));
		d.m_x = b.m_x + c.m_x - a.m_x;
		d.m_y = b.m_y + c.m_y - a.m_y;

		glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glBegin(GL_TRIANGLE_STRIP);

		glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_min);
		glVertex2f(a.m_x, a.m_y);

		glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_min);
		glVertex2f(b.m_x, b.m_y);

		glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_max);
		glVertex2f(c.m_x, c.m_y);

		glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_max);
		glVertex2f(d.m_x, d.m_y);

		glEnd();
	}

}	// end namespace render
};	// end namespace gameswf


// Some external interfaces.
namespace gameswf
{
	void	set_antialiased(bool enable)
	// Call with 'true' to enable antialiasing; with 'false' to
	// turn it off.
	{
		render::s_enable_antialias = enable;
	}


	// Hint that tells us how the physical output is zoomed.  This
	// is important for getting the antialiasing and curve
	// subdivision right.
	static float	s_pixel_scale = 1.0f;


	void	set_pixel_scale(float scale)
	// Call with the zoom factor the movie is being displayed at.
	// I.e. if you're showing the movie 2x normal size, then pass
	// 2.0f.  This is important for getting the antialiasing and
	// curve subdivision right.
	{
		if (scale > 0)
		{
			s_pixel_scale = scale;
		}
	}

	float	get_pixel_scale()
	{
		return s_pixel_scale;
	}

}	// end namespace gameswf



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
