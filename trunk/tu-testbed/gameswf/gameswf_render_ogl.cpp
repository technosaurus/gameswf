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


// On any multitexture card, we should be able to modulate in an edge
// texture for nice edge antialiasing.
#define USE_MULTITEXTURE_ANTIALIASING	1


namespace gameswf
{
namespace render
{
	// Some renderer state.

	// Hint that tells us how the physical output is zoomed.  This
	// is important for getting the antialiasing and curve
	// subdivision right.
	static float	s_pixel_scale = 1.0f;

	// Enable/disable antialiasing.
	static bool	s_enable_antialias = true;

	// Curve subdivision error tolerance (in TWIPs)
	static float	s_tolerance = 20.0f / s_pixel_scale;

	// Controls whether we do antialiasing by modulating in a special edge texture.
	static bool	s_multitexture_antialias = false;
	static unsigned int	s_edge_texture_id = 0;

	static bool	s_wireframe = false;

	// Output size.
	static float	s_display_width;
	static float	s_display_height;

	// Transform stacks.
	static array<matrix>	s_matrix_stack;
	static array<cxform>	s_cxform_stack;


	// Specialized hacky software mode -- for rendering font glyph
	// shapes into an 8-bit alpha texture, in order to have faster
	// & better looking small text rendering.
	static bool	s_software_mode_active = false;
	static Uint8*	s_software_mode_buffer = NULL;
	static int	s_software_mode_width = 0;
	static int	s_software_mode_height = 0;


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

		void	apply(const matrix& current_matrix) const
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
						// need to use a pixel shader.
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

					// It appears as though the bitmap matrix is the
					// inverse of the matrix I want to apply to the TWIPs
					// coords to get the texture pixel coords (I want to
					// scale again by 1/w and 1/h, on top of that).

					float	inv_width = 1.0f / m_bitmap_info->m_original_width;
					float	inv_height = 1.0f / m_bitmap_info->m_original_height;

					matrix	screen_to_obj;
					screen_to_obj.set_inverse(current_matrix);

					matrix	m = m_bitmap_matrix;
					m.concatenate(screen_to_obj);


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


	// More Renderer state.
	static array<fill_style>	s_current_styles;
	static int	s_left_style = -1;
	static int	s_right_style = -1;
	static int	s_line_style = -1;


	static void	peel_off_and_render(int i0, int i1, float y0, float y1);


	bitmap_info*	create_bitmap_info(image::rgb* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		return new bitmap_info(im);
	}


	bitmap_info*	create_bitmap_info(image::rgba* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
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

		// Prime the matrix stack with an identity transform.
		assert(s_matrix_stack.size() == 0);
		matrix	identity;
		identity.set_identity();
		s_matrix_stack.push_back(identity);

		// Prime the cxform stack an identity cxform.
		assert(s_cxform_stack.size() == 0);
		cxform	cx_identity;
		s_cxform_stack.push_back(cx_identity);

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


	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		if (fill_side == 0)
		{
			s_left_style = -1;
		}
		else
		{
			assert(fill_side == 1);
			s_right_style = -1;
		}
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		s_line_style = -1;
	}


	void	fill_style_color(int fill_side, rgba color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		s_current_styles.push_back(fill_style());
		s_current_styles.back().set_color(s_cxform_stack.back().transform(color));

		if (fill_side == 0)
		{
			s_left_style = s_current_styles.size() - 1;
		}
		else
		{
			assert(fill_side == 1);
			s_right_style = s_current_styles.size() - 1;
		}
	}


	void	line_style_color(rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		s_current_styles.push_back(fill_style());
		s_current_styles.back().set_color(s_cxform_stack.back().transform(color));
		s_line_style = s_current_styles.size() - 1;
	}


	void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm)
	{
		s_current_styles.push_back(fill_style());
		s_current_styles.back().set_bitmap(bi, m, wm, s_cxform_stack.back());

		if (fill_side == 0)
		{
			s_left_style = s_current_styles.size() - 1;
		}
		else
		{
			assert(fill_side == 1);
			s_right_style = s_current_styles.size() - 1;
		}
	}


	void	begin_shape()
	{
		// make sure our shape state is cleared out.
		s_current_styles.resize(0);
		s_left_style = -1;
		s_right_style = -1;
		s_line_style = -1;

		gameswf::tesselate::begin_shape(s_tolerance / 20.0f);
	}


	static void	software_trapezoid(
		float y0, float y1,
		float xl0, float xl1,
		float xr0, float xr1)
	// Fill the specified trapezoid in the software output buffer.
	{
		int	iy0 = (int) ceilf(y0);
		int	iy1 = (int) ceilf(y1);
		float	dy = y1 - y0;

		for (int y = iy0; y < iy1; y++)
		{
			if (y < 0) continue;
			if (y >= s_software_mode_height) return;

			float	f = (y - y0) / dy;
			int	xl = (int) ceilf(flerp(xl0, xl1, f));
			int	xr = (int) ceilf(flerp(xr0, xr1, f));
			
			xl = iclamp(xl, 0, s_software_mode_width - 1);
			xr = iclamp(xr, 0, s_software_mode_width - 1);

			if (xr > xl)
			{
				memset(s_software_mode_buffer + y * s_software_mode_width + xl,
				       255,
				       xr - xl);
			}
		}
	}


	struct software_accepter : public gameswf::tesselate::trapezoid_accepter
	{
		void	accept_trapezoid(int style, const gameswf::tesselate::trapezoid& tr)
		// Render trapezoids into the software buffer, as they're
		// emitted from the tesselator.
		{
			software_trapezoid(tr.m_y0, tr.m_y1, tr.m_lx0, tr.m_lx1, tr.m_rx0, tr.m_rx1);
		}

		void	accept_line_segment(int style, float x0, float y0, float x1, float y1)
		// The software renderer doesn't care about lines;
		// it's only used for rendering font glyphs which
		// don't use lines.
		{
		}
	};


	// Render OpenGL trapezoids, as they're emitted from the
	// tesselator.
	struct opengl_accepter : public gameswf::tesselate::trapezoid_accepter
	{
		void	accept_trapezoid(int style, const gameswf::tesselate::trapezoid& tr)
		// The style arg tells us which style out of
		// s_current_styles to activate.
		{
			assert(style >= 0 && style < s_current_styles.size());
			assert(s_current_styles[style].is_valid());

			s_current_styles[style].apply(s_matrix_stack.back());

			// Draw the trapezoid.
			glBegin(GL_QUADS);
			glVertex2f(tr.m_lx0, tr.m_y0);
			glVertex2f(tr.m_lx1, tr.m_y1);
			glVertex2f(tr.m_rx1, tr.m_y1);
			glVertex2f(tr.m_rx0, tr.m_y0);
			glEnd();
		}

		void	accept_line_segment(int style, float x0, float y0, float x1, float y1)
		// Draw the specified line segment.
		{
			assert(style >= 0 && style < s_current_styles.size());
			assert(s_current_styles[style].is_valid());

			s_current_styles[style].apply(s_matrix_stack.back());

			// Draw the line segment.
			glBegin(GL_LINES);
			glVertex2f(x0, y0);
			glVertex2f(x1, y1);
			glEnd();
		}
	};


	void	end_shape()
	{
		// Render the shape stored in the tesselator.
		if (s_software_mode_active)
		{
			gameswf::tesselate::end_shape(&software_accepter());
		}
		else
		{
			gameswf::tesselate::end_shape(&opengl_accepter());
		}
	}


	void	begin_path(float ax, float ay)
	// This call begins drawing a sequence of segments, which all
	// share the same fill & line styles.  Add segments to the
	// shape using add_curve_segment() or add_line_segment(), and
	// call end_path() when you're done with this sequence.
	{
		point	p;
		s_matrix_stack.back().transform(&p, point(ax, ay));

		gameswf::tesselate::begin_path(s_left_style, s_right_style, s_line_style, p.m_x, p.m_y);
	}


	void	add_line_segment(float ax, float ay)
	// Add a line running from the previous anchor point to the
	// given new anchor point.
	{
		point	p;
		s_matrix_stack.back().transform(&p, point(ax, ay));

		gameswf::tesselate::add_line_segment(p.m_x, p.m_y);
	}


	void	add_curve_segment(float cx, float cy, float ax, float ay)
	// Add a curve segment to the shape.  The curve segment is a
	// quadratic bezier, running from the previous anchor point to
	// the given new anchor point (ax, ay), with (cx, cy) acting
	// as the control point in between.
	{
		point	c, a;
		s_matrix_stack.back().transform(&c, point(cx, cy));
		s_matrix_stack.back().transform(&a, point(ax, ay));

		gameswf::tesselate::add_curve_segment(c.m_x, c.m_y, a.m_x, a.m_y);
	}


	void	end_path()
	// Mark the end of a set of edges that all use the same styles.
	{
		gameswf::tesselate::end_path();
	}


	void	draw_bitmap(const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color)
	// Draw a rectangle textured with the given bitmap, with the
	// given color.  Apply current transforms.
	//
	// Intended for textured glyph rendering.
	{
		assert(bi);

		color = s_cxform_stack.back().transform(color);
		color.ogl_color();

		point a, b, c, d;
		s_matrix_stack.back().transform(&a, point(coords.m_x_min, coords.m_y_min));
		s_matrix_stack.back().transform(&b, point(coords.m_x_max, coords.m_y_min));
		s_matrix_stack.back().transform(&c, point(coords.m_x_min, coords.m_y_max));
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


	//
	// special hacky software-mode interface
	//


	void	software_mode_enable(int width, int height)
	// Enable special software-rendering mode.  8-bit RAM render
	// target of given dimensions.
	{
		assert(s_software_mode_active == false);
		assert(s_software_mode_buffer == NULL);

		s_software_mode_active = true;
		s_software_mode_buffer = new Uint8[width * height];
		s_software_mode_width = width;
		s_software_mode_height = height;

		// Set up transform stacks.

		// Prime the matrix stack with an identity transform.
		assert(s_matrix_stack.size() == 0);
		matrix	identity;
		identity.set_identity();
		s_matrix_stack.push_back(identity);

		// Prime the cxform stack with an identity cxform.
		assert(s_cxform_stack.size() == 0);
		cxform	cx_identity;
		s_cxform_stack.push_back(cx_identity);
	}


	void	software_mode_disable()
	// Turn off software-rendering mode.
	{
		assert(s_software_mode_active);
		assert(s_software_mode_buffer);

		s_software_mode_active = false;
		delete [] s_software_mode_buffer;
		s_software_mode_buffer = NULL;

		// Clean up stacks.

		assert(s_matrix_stack.size() == 1);
		s_matrix_stack.resize(0);

		assert(s_cxform_stack.size() == 1);
		s_cxform_stack.resize(0);
	}


	Uint8*	get_software_mode_buffer()
	// Return the buffer we've been using for software rendering.
	{
		return s_software_mode_buffer;
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


	void	set_pixel_scale(float scale)
	// Call with the zoom factor the movie is being displayed at.
	// I.e. if you're showing the movie 2x normal size, then pass
	// 2.0f.  This is important for getting the antialiasing and
	// curve subdivision right.
	{
		if (scale > 0)
		{
			render::s_pixel_scale = scale;
			render::s_tolerance = 20.0f / render::s_pixel_scale;
		}
	}

}	// end namespace gameswf



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
