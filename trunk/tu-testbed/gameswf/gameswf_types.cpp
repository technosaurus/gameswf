// gameswf_types.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some basic types for gameswf.


#include "engine/ogl.h"
#include "gameswf_types.h"

#include "gameswf_log.h"
#include "gameswf_stream.h"
#include <string.h>


namespace gameswf
{
	//
	// matrix
	//

	matrix	matrix::identity;

	matrix::matrix()
	{
		// Default to identity.
		set_identity();
	}


	void	matrix::set_identity()
	// Set the matrix to identity.
	{
		memset(&m_[0], 0, sizeof(m_));
		m_[0][0] = 1;
		m_[1][1] = 1;
	}


	void	matrix::concatenate(const matrix& m)
	// Concatenate m's transform onto ours.  When
	// transforming points, m happens first, then our
	// original xform.
	{
		matrix	t;
		t.m_[0][0] = m_[0][0] * m.m_[0][0] + m_[0][1] * m.m_[1][0];
		t.m_[1][0] = m_[1][0] * m.m_[0][0] + m_[1][1] * m.m_[1][0];
		t.m_[0][1] = m_[0][0] * m.m_[0][1] + m_[0][1] * m.m_[1][1];
		t.m_[1][1] = m_[1][0] * m.m_[0][1] + m_[1][1] * m.m_[1][1];
		t.m_[0][2] = m_[0][0] * m.m_[0][2] + m_[0][1] * m.m_[1][2] + m_[0][2];
		t.m_[1][2] = m_[1][0] * m.m_[0][2] + m_[1][1] * m.m_[1][2] + m_[1][2];

		*this = t;
	}


	void	matrix::concatenate_translation(float tx, float ty)
	// Concatenate a translation onto the front of our
	// matrix.  When transforming points, the translation
	// happens first, then our original xform.
	{
		m_[0][2] += m_[0][0] * tx + m_[0][1] * ty;
		m_[1][2] += m_[1][0] * tx + m_[1][1] * ty;
	}


	void	matrix::concatenate_scale(float scale)
	// Concatenate a uniform scale onto the front of our
	// matrix.  When transforming points, the scale
	// happens first, then our original xform.
	{
		m_[0][0] *= scale;
		m_[0][1] *= scale;
		m_[1][0] *= scale;
		m_[1][1] *= scale;
	}


	void	matrix::read(stream* in)
	// Initialize from the stream.
	{
		in->align();

		set_identity();

		int	has_scale = in->read_uint(1);
		if (has_scale)
		{
			int	scale_nbits = in->read_uint(5);
			m_[0][0] = in->read_sint(scale_nbits) / 65536.0f;
			m_[1][1] = in->read_sint(scale_nbits) / 65536.0f;
		}
		int	has_rotate = in->read_uint(1);
		if (has_rotate)
		{
			int	rotate_nbits = in->read_uint(5);
			m_[1][0] = in->read_sint(rotate_nbits) / 65536.0f;
			m_[0][1] = in->read_sint(rotate_nbits) / 65536.0f;
		}

		int	translate_nbits = in->read_uint(5);
		if (translate_nbits > 0) {
			m_[0][2] = in->read_sint(translate_nbits);
			m_[1][2] = in->read_sint(translate_nbits);
		}

		IF_DEBUG(log_msg("has_scale = %d, has_rotate = %d\n", has_scale, has_rotate));
	}


	void	matrix::print() const
	// Debug log.
	{
		log_msg("| %4.4f %4.4f %4.4f |\n", m_[0][0], m_[0][1], TWIPS_TO_PIXELS(m_[0][2]));
		log_msg("| %4.4f %4.4f %4.4f |\n", m_[1][0], m_[1][1], TWIPS_TO_PIXELS(m_[1][2]));
	}

	void	matrix::ogl_multiply() const
	// Apply this matrix onto the top of the currently
	// selected OpenGL matrix stack.
	{
		float	m[16];
		memset(&m[0], 0, sizeof(m));
		m[0] = m_[0][0];
		m[1] = m_[1][0];
		m[4] = m_[0][1];
		m[5] = m_[1][1];
		m[10] = 1;
		m[12] = m_[0][2];
		m[13] = m_[1][2];
		m[15] = 1;
		glMultMatrixf(m);
	}

	void	matrix::transform(point* result, const point& p) const
	// Transform point 'p' by our matrix.  Put the result in
	// *result.
	{
		assert(result);

		result->m_x = m_[0][0] * p.m_x + m_[0][1] * p.m_y + m_[0][2];
		result->m_y = m_[1][0] * p.m_x + m_[1][1] * p.m_y + m_[1][2];
	}
	
	void	matrix::transform_vector(point* result, const point& v) const
	// Transform vector 'v' by our matrix. Doesn't apply translation.
	// Put the result in *result.
	{
		assert(result);

		result->m_x = m_[0][0] * v.m_x + m_[0][1] * v.m_y;
		result->m_y = m_[1][0] * v.m_x + m_[1][1] * v.m_y;
	}

	void	matrix::transform_by_inverse(point* result, const point& p) const
	// Transform point 'p' by the inverse of our matrix.  Put result in *result.
	{
		// @@ TODO optimize this!
		matrix	m;
		m.set_inverse(*this);
		m.transform(result, p);
	}


	void	matrix::set_inverse(const matrix& m)
	// Set this matrix to the inverse of the given matrix.
	{
		assert(this != &m);

		// Invert the rotation part.
		float	det = m.m_[0][0] * m.m_[1][1] - m.m_[0][1] * m.m_[1][0];
		if (det == 0.0f)
		{
			// Not invertible.
			//assert(0);	// castano: this happens sometimes! (ie. sample6.swf)

			// Arbitrary fallback.
			set_identity();
			m_[0][2] = -m.m_[0][2];
			m_[1][2] = -m.m_[1][2];
		}
		else
		{
			float	inv_det = 1.0f / det;
			m_[0][0] = m.m_[1][1] * inv_det;
			m_[1][1] = m.m_[0][0] * inv_det;
			m_[0][1] = -m.m_[0][1] * inv_det;
			m_[1][0] = -m.m_[1][0] * inv_det;

			m_[0][2] = -(m_[0][0] * m.m_[0][2] + m_[0][1] * m.m_[1][2]);
			m_[1][2] = -(m_[1][0] * m.m_[0][2] + m_[1][1] * m.m_[1][2]);
		}
	}


	bool	matrix::does_flip() const
	// Return true if this matrix reverses handedness.
	{
		float	det = m_[0][0] * m_[1][1] - m_[0][1] * m_[1][0];

		return det < 0.f;
	}


	float	matrix::get_max_scale() const
	// Return the maximum scale factor that this transform
	// applies.  For assessing scale, when determining acceptable
	// errors in tesselation.
	{
		// @@ not 100% sure what the heck I'm doing here.  I
		// think this is roughly what I want; take the max
		// length of the two basis vectors.
		float	basis0_length = sqrtf(m_[0][0] * m_[0][0] + m_[0][1] * m_[0][1]);
		float	basis1_length = sqrtf(m_[1][0] * m_[1][0] + m_[1][1] * m_[1][1]);
		return fmax(basis0_length, basis1_length);
	}


	//
	// cxform
	//


	cxform::cxform()
	// Initialize to identity transform.
	{
		m_[0][0] = 1;
		m_[1][0] = 1;
		m_[2][0] = 1;
		m_[3][0] = 1;
		m_[0][1] = 0;
		m_[1][1] = 0;
		m_[2][1] = 0;
		m_[3][1] = 0;
	}

	void	cxform::concatenate(const cxform& c)
	// Concatenate c's transform onto ours.  When
	// transforming colors, c's transform is applied
	// first, then ours.
	{
		m_[0][1] += m_[0][0] * c.m_[0][1];
		m_[1][1] += m_[1][0] * c.m_[1][1];
		m_[2][1] += m_[2][0] * c.m_[2][1];
		m_[3][1] += m_[3][0] * c.m_[3][1];

		m_[0][0] *= c.m_[0][0];
		m_[1][0] *= c.m_[1][0];
		m_[2][0] *= c.m_[2][0];
		m_[3][0] *= c.m_[3][0];
	}

	
	rgba	cxform::transform(const rgba in) const
	// Apply our transform to the given color; return the result.
	{
		rgba	result;

		result.m_r = (Uint8) fclamp(in.m_r * m_[0][0] + m_[0][1], 0, 255);
		result.m_g = (Uint8) fclamp(in.m_g * m_[1][0] + m_[1][1], 0, 255);
		result.m_b = (Uint8) fclamp(in.m_b * m_[2][0] + m_[2][1], 0, 255);
		result.m_a = (Uint8) fclamp(in.m_a * m_[3][0] + m_[3][1], 0, 255);

		return result;
	}


	void	cxform::read_rgb(stream* in)
	{
		in->align();

		int	has_add = in->read_uint(1);
		int	has_mult = in->read_uint(1);
		int	nbits = in->read_uint(4);

		if (has_mult) {
			m_[0][0] = in->read_sint(nbits) / 255.0f;
			m_[1][0] = in->read_sint(nbits) / 255.0f;
			m_[2][0] = in->read_sint(nbits) / 255.0f;
			m_[3][0] = 1;
		}
		else {
			for (int i = 0; i < 4; i++) { m_[i][0] = 1; }
		}
		if (has_add) {
			m_[0][1] = in->read_sint(nbits);
			m_[1][1] = in->read_sint(nbits);
			m_[2][1] = in->read_sint(nbits);
			m_[3][1] = 1;
		}
		else {
			for (int i = 0; i < 4; i++) { m_[i][1] = 0; }
		}
	}

	void	cxform::read_rgba(stream* in)
	{
		in->align();

		int	has_add = in->read_uint(1);
		int	has_mult = in->read_uint(1);
		int	nbits = in->read_uint(4);

		if (has_mult) {
			m_[0][0] = in->read_sint(nbits) / 255.0f;
			m_[1][0] = in->read_sint(nbits) / 255.0f;
			m_[2][0] = in->read_sint(nbits) / 255.0f;
			m_[3][0] = in->read_sint(nbits) / 255.0f;
		}
		else {
			for (int i = 0; i < 4; i++) { m_[i][0] = 1; }
		}
		if (has_add) {
			m_[0][1] = in->read_sint(nbits);
			m_[1][1] = in->read_sint(nbits);
			m_[2][1] = in->read_sint(nbits);
			m_[3][1] = in->read_sint(nbits);
		}
		else {
			for (int i = 0; i < 4; i++) { m_[i][1] = 0; }
		}
	}


	//
	// rgba
	//

	void	rgba::ogl_color() const
	// Set the glColor to our state.
	{
		glColor4ub(m_r, m_g, m_b, m_a);
	}

	void	rgba::read(stream* in, int tag_type)
	{
		if (tag_type <= 22)
		{
			read_rgb(in);
		}
		else
		{
			read_rgba(in);
		}
	}

	void	rgba::read_rgba(stream* in)
	{
		read_rgb(in);
		m_a = in->read_u8();
	}

	void	rgba::read_rgb(stream* in)
	{
		m_r = in->read_u8();
		m_g = in->read_u8();
		m_b = in->read_u8();
		m_a = 0x0FF;
	}

	void	rgba::print()
	// For debugging.
	{
		log_msg("rgba: %d %d %d %d\n", m_r, m_g, m_b, m_a);
	}


	//
	// rect
	//


	void	rect::read(stream* in)
	{
		in->align();
		int	nbits = in->read_uint(5);
		m_x_min = in->read_sint(nbits);
		m_x_max = in->read_sint(nbits);
		m_y_min = in->read_sint(nbits);
		m_y_max = in->read_sint(nbits);

//		IF_DEBUG(log_msg("rect::read() nbits = %d\n", nbits));
	}

	void	rect::print() const
	// Debug spew.
	{
		log_msg("xmin = %g, ymin = %g, xmax = %g, ymax = %g\n",
			TWIPS_TO_PIXELS(m_x_min),
			TWIPS_TO_PIXELS(m_y_min),
			TWIPS_TO_PIXELS(m_x_max),
			TWIPS_TO_PIXELS(m_y_max));
	}

#if 0
	void	rect::debug_display(const display_info& di)
	// Show the rectangle.
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		di.m_matrix.ogl_multiply();

		glColor3f(1, 1, 0);
		glBegin(GL_LINE_STRIP);
		{
			glVertex2f(m_x_min, m_y_min);
			glVertex2f(m_x_min, m_y_max);
			glVertex2f(m_x_max, m_y_max);
			glVertex2f(m_x_max, m_y_min);
			glVertex2f(m_x_min, m_y_min);
		}
		glEnd();

		glPopMatrix();
	}
#endif // 0

	
	bool	rect::point_test(float x, float y) const
	// Return true if the specified point is inside this rect.
	{
		if (x < m_x_min
		    || x > m_x_max
		    || y < m_y_min
		    || y > m_y_max)
		{
			return false;
		}
		else
		{
			return true;
		}
	}


	void	rect::expand_to_point(float x, float y)
	// Expand this rectangle to enclose the given point.
	{
		m_x_min = fmin(m_x_min, x);
		m_y_min = fmin(m_y_min, y);
		m_x_max = fmax(m_x_max, x);
		m_y_max = fmax(m_y_max, y);
	}

};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
