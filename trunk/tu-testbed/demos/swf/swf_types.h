// swf_types.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some basic types for gameswf.


#ifndef SWF_TYPES_H
#define SWF_TYPES_H


#include "SDL.h"


// ifdef this out to get rid of debug spew.
#define IF_DEBUG(exp) exp


#define TWIPS_TO_PIXELS(x)	((x) / 20.f)


namespace swf
{
	struct stream;	// forward declaration

	struct point
	{
		float	m_x, m_y;	// should probably use integer TWIPs...

		point() : m_x(0), m_y(0) {}
		point(float x, float y) : m_x(x), m_y(y) {}

		void	set_lerp(const point& a, const point& b, float t)
		// Set to a + (b - a) * t
		{
			m_x = a.m_x + (b.m_x - a.m_x) * t;
			m_y = a.m_y + (b.m_y - a.m_y) * t;
		}
	};


	struct matrix
	{
		float	m_[2][3];

		matrix();
		void	set_identity();
		void	concatenate(const matrix& m);
		void	concatenate_translation(float tx, float ty);
		void	read(stream* in);
		void	print(FILE* out) const;
		void	ogl_multiply() const;
		void	transform(point* result, const point& p);
		void	set_inverse(const matrix& m);
	};


	struct rgba
	{
		Uint8	m_r, m_g, m_b, m_a;

		rgba() : m_r(255), m_g(255), m_b(255), m_a(255) {}

		rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
			:
			m_r(r), m_g(g), m_b(b), m_a(a)
		{
		}

		void	ogl_color() const;
		void	read(stream* in, int tag_type);
		void	read_rgba(stream* in);
		void	read_rgb(stream* in);

		void	print(FILE* out);
	};


	struct cxform
	{
		float	m_[4][2];	// [RGBA][mult, add]

		cxform();
		void	concatenate(const cxform& c);
		rgba	transform(const rgba in) const;
		void	read_rgb(stream* in);
		void	read_rgba(stream* in);
	};


};	// end namespace swf


#endif // SWF_TYPES_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
