// gameswf_render_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier shape renderer for gameswf, with color & bitmap
// support.

// OpenGL version.


#include "gameswf_render.h"
#include "gameswf_types.h"
#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"
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


	// A struct used to hold info about an OpenGL texture.
	struct bitmap_info
	{
		unsigned int	m_texture_id;	// OpenGL texture id
		int	m_original_width;
		int	m_original_height;

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

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);
		
			m_original_width = im->m_width;
			m_original_height = im->m_height;

			int	w = 1; while (w < im->m_width) { w <<= 1; }
			int	h = 1; while (h < im->m_height) { h <<= 1; }

			image::rgb*	rescaled = image::create_rgb(w, h);
			image::resample(rescaled, 0, 0, w - 1, h - 1,
					im, 0, 0, im->m_width, im->m_height);
		
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rescaled->m_data);
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

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
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
		
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rescaled->m_data);

				delete rescaled;
			}
			else
			{
				// Use original image directly.
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im->m_data);
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
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
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
					screen_to_obj.set_inverse(s_matrix_stack.back());

					matrix	m = m_bitmap_matrix;
					m.concatenate(screen_to_obj);


					glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					float	p[4] = { 0, 0, 0, 0 };
					p[0] = m.m_[0][0] * inv_width;
					p[1] = m.m_[0][1] * inv_width;
					p[3] = m.m_[0][2] * inv_width;
					glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
					p[0] = 0.0f;

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


	// More Renderer state.
	static array<fill_segment>	s_current_segments;
	static array<point>	s_current_path;
	static point	s_last_point;
	static fill_style	s_current_left_style;
	static fill_style	s_current_right_style;
	static fill_style	s_current_line_style;
	static bool	s_shape_has_line;	// flag to let us skip the line rendering if no line styles were set when defining the shape.
	static bool	s_shape_has_fill;	// flag to let us skip the fill rendering if no fill styles were set when defining the shape.



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

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, edge_data);

				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// @@ should we use a 1D texture???
				ogl::active_texture(GL_TEXTURE0_ARB);
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
			s_current_left_style.disable();
		}
		else
		{
			assert(fill_side == 1);
			s_current_right_style.disable();
		}
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		s_current_line_style.disable();
	}


	void	fill_style_color(int fill_side, rgba color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		assert(s_current_path.size() == 0);	// you can't change styles within a begin_path()/end_path() pair.

		if (fill_side == 0)
		{
			s_current_left_style.set_color(s_cxform_stack.back().transform(color));
			s_shape_has_fill = true;
		}
		else
		{
			assert(fill_side == 1);

			s_current_right_style.set_color(s_cxform_stack.back().transform(color));
			s_shape_has_fill = true;
		}
	}


	void	line_style_color(rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		assert(s_current_path.size() == 0);	// you can't change styles within a begin_path()/end_path() pair.

		s_current_line_style.set_color(s_cxform_stack.back().transform(color));
		s_shape_has_line = true;
	}


	void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm)
	{
		if (fill_side == 0)
		{
			s_current_left_style.set_bitmap(bi, m, wm, s_cxform_stack.back());
			s_shape_has_fill = true;
		}
		else
		{
			assert(fill_side == 1);

			s_current_right_style.set_bitmap(bi, m, wm, s_cxform_stack.back());
			s_shape_has_fill = true;
		}
	}


	void	begin_shape()
	{
		// ensure we're not already in a shape or path.
		// make sure our shape state is cleared out.
		assert(s_current_segments.size() == 0);
		s_current_segments.resize(0);

		assert(s_current_path.size() == 0);
		s_current_path.resize(0);

		s_current_line_style.disable();
		s_current_left_style.disable();
		s_current_right_style.disable();
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


	void	output_current_segments()
	// Draw our shapes and lines, then clear the segment list.
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
					seg.m_line_style.apply(s_matrix_stack.back());
					glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
					glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
				}
			}}
			glEnd();
		}

		// wireframe, for debugging
		if (s_wireframe)
		{
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			glBegin(GL_LINES);
			{for (int i = 0; i < s_current_segments.size(); i++)
			{
				const fill_segment&	seg = s_current_segments[i];
				glVertex2f(seg.m_begin.m_x, seg.m_begin.m_y);
				glVertex2f(seg.m_end.m_x, seg.m_end.m_y);
			}}
			glEnd();
		}

		if (s_wireframe == false
		    && s_shape_has_fill == true)
		{
			//
			// Draw the filled shape.
			//

// 			// xxxxx
// 			if (s_current_left_style.is_valid())
// 			{
// 				s_current_left_style.apply(s_matrix_stack.back());
// 			}
// 			if (s_current_right_style.is_valid())
// 			{
// 				s_current_right_style.apply(s_matrix_stack.back());
// 			}
// 			// xxxxx

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


	void	compute_antialias_s_coords(float* s0, float* s1, const point& a, const point& b, const point& c, const point& d)
	// Given the quad {a,b,c,d}, compute texture coords s0,s1
	// such that setting texture coords {0,0,s0,s1} for the quad
	// will make a 1-pixel wide feathered antialised edge along
	// {a,b}.
	{
#if 0
		// Basically, find the normal to the edge {a,b} and then compute normal * {a,c} and normal*{a,d}

		float	nx = (b.m_y - a.m_y);
		float	ny = - (b.m_x - a.m_x);
		float	scale = (1.0f / sqrtf(nx * nx + ny * ny)) * (0.5f * s_pixel_scale / 20.f);
		nx *= scale;
		ny *= scale;

		*s0 = nx * (c.m_x - a.m_x) + ny * (c.m_y - a.m_y);
		*s1 = nx * (d.m_x - a.m_x) + ny * (d.m_y - a.m_y);
#else
		*s0 = (c.m_x - a.m_x) * 0.5f * s_pixel_scale / 20.f;
		*s1 = (d.m_x - a.m_x) * 0.5f * s_pixel_scale / 20.f;

		if (a.m_y > b.m_y)
		{
			*s0 = -*s0;
			*s1 = -*s1;
		}
#endif // 0
	}


	void	peel_off_and_render(int i0, int i1, float y0, float y1)
	// Clip the interval [y0, y1] off of the segments from s_current_segments[i0 through (i1-1)]
	// and render the clipped segments.  Modifies the values in s_current_segments.
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

		// Render pairs.
		if (s_multitexture_antialias)
		{
			for (int i = 0; i < slab.size() - 1; i++)
			{
				if (slab[i].m_left_style.is_valid())
				{
					// assert(slab[i + 1].m_right_style == slab[i].m_left_style);	//????

					slab[i].m_left_style.apply(s_matrix_stack.back());

					ogl::active_texture(GL_TEXTURE1_ARB);
					glBindTexture(GL_TEXTURE_2D, s_edge_texture_id);
					glEnable(GL_TEXTURE_2D);	// @@ should we use a 1D texture???
					ogl::active_texture(GL_TEXTURE0_ARB);

					glBegin(GL_QUADS);

					// Here's what we have:
					//
					//    a   +--------------+ c
					//       /                \
					//      /                  \
					//   b +------------------- + d
					//
					// We'll turn it into:
					//
					//               e
					//    a   +------+-------+ c
					//       /       |        \
					//      /         |        \
					//   b +----------+-------- + d
					//                f
					//
					// Where edges {a,b} and {c,d} will be antialiased.

					point	a = slab[i].m_begin;
					point	b = slab[i].m_end;
					point	c = slab[i + 1].m_begin;
					point	d = slab[i + 1].m_end;

					// Expand the trapezoid by a pixel to make up for
					// the reduced coverage due to antialiasing.
					a.m_x -= 20.f / s_pixel_scale;
					b.m_x -= 20.f / s_pixel_scale;
					c.m_x += 20.f / s_pixel_scale;
					d.m_x += 20.f / s_pixel_scale;

					const point	e((a.m_x + c.m_x) * 0.5f, (a.m_y + c.m_y) * 0.5f);
					const point	f((b.m_x + d.m_x) * 0.5f, (b.m_y + d.m_y) * 0.5f);

					float	s0, s1;
					compute_antialias_s_coords(&s0, &s1, a, b, f, e);

					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, 0.0f, 0.0f);
					glVertex2f(a.m_x, a.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, 0.0f, 0.0f);
					glVertex2f(b.m_x, b.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, s0, 0.0f);
					glVertex2f(f.m_x, f.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, s1, 0.0f);
					glVertex2f(e.m_x, e.m_y);

					compute_antialias_s_coords(&s0, &s1, d, c, e, f);

					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, 0.0f, 0.0f);
					glVertex2f(d.m_x, d.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, 0.0f, 0.0f);
					glVertex2f(c.m_x, c.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, s0, 0.0f);
					glVertex2f(e.m_x, e.m_y);
					ogl::multi_tex_coord_2f(GL_TEXTURE1_ARB, s1, 0.0f);
					glVertex2f(f.m_x, f.m_y);

					glEnd();

					ogl::active_texture(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					ogl::active_texture(GL_TEXTURE0_ARB);
				}
			}
		}
		else
		{
			for (int i = 0; i < slab.size() - 1; i++)
			{
				if (slab[i].m_left_style.is_valid())
				{
					// assert(slab[i + 1].m_right_style == slab[i].m_left_style);	//????

					slab[i].m_left_style.apply(s_matrix_stack.back());

					glBegin(GL_QUADS);
					glVertex2f(slab[i].m_begin.m_x, slab[i].m_begin.m_y);
					glVertex2f(slab[i].m_end.m_x, slab[i].m_end.m_y);
					glVertex2f(slab[i + 1].m_end.m_x, slab[i + 1].m_end.m_y);
					glVertex2f(slab[i + 1].m_begin.m_x, slab[i + 1].m_begin.m_y);
					glEnd();
				}
			}
		}
	}


	void	end_shape()
	{
		output_current_segments();
	}


	void	begin_path(float ax, float ay)
	// This call begins drawing a sequence of segments, which all
	// share the same fill & line styles.  Add segments to the
	// shape using add_curve_segment() or add_line_segment(), and
	// call end_path() when you're done with this sequence.
	{
		point	p(ax, ay);
		s_matrix_stack.back().transform(&s_last_point, p);

		assert(s_current_path.size() == 0);
		s_current_path.resize(0);

		s_current_path.push_back(s_last_point);
	}


	static void	add_line_segment_raw(const point& p)
	// INTERNAL: Add a line segment running from the previous
	// anchor point to the given new anchor point -- (ax, ay) is
	// already transformed.
	{
		// s_current_segments is used for filling shapes.
		// if (either fill style is valid) { ...
		s_current_segments.push_back(
			fill_segment(
				s_last_point,
				p,
				s_current_left_style,
				s_current_right_style,
				s_current_line_style));

		s_last_point = p;

		s_current_path.push_back(p);
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
	// Mark the end of a set of edges that all use the same styles.
	{
		s_current_path.resize(0);
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

};	// end namespace gameswf



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
