// gameswf_types.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some basic types for gameswf.


#ifndef GAMESWF_TYPES_H
#define GAMESWF_TYPES_H


#include "base/utility.h"


#ifndef NDEBUG
#define IF_DEBUG(exp) exp
#else	// not NDEBUG
#define IF_DEBUG(exp)
#endif // not NDEBUG


namespace gameswf
{
	extern bool	s_verbose_action;
	extern bool	s_verbose_parse;
};

#define IF_VERBOSE_ACTION(exp) if (gameswf::s_verbose_action) { exp; }
#define IF_VERBOSE_PARSE(exp) if (gameswf::s_verbose_parse) { exp; }


#define TWIPS_TO_PIXELS(x)	((x) / 20.f)
#define PIXELS_TO_TWIPS(x)	((x) * 20.f)


namespace gameswf
{
	struct stream;	// forward declaration

	struct point
	{
		float	m_x, m_y;

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

		static matrix	identity;

		matrix();
		void	set_identity();
		void	concatenate(const matrix& m);
		void	concatenate_translation(float tx, float ty);
		void	concatenate_scale(float s);
		void	read(stream* in);
		void	print() const;
		void	apply() const;
		void	transform(point* result, const point& p) const;
		void	transform_vector(point* result, const point& p) const;
		void	transform_by_inverse(point* result, const point& p) const;
		void	set_inverse(const matrix& m);
		bool	does_flip() const;	// return true if we flip handedness
		float	get_max_scale() const;	// return the maximum scale factor that this transform applies
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

		void	apply() const;
		void	read(stream* in, int tag_type);
		void	read_rgba(stream* in);
		void	read_rgb(stream* in);

		void	set(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
		{
			m_r = r;
			m_g = g;
			m_b = b;
			m_a = a;
		}

		void	set_lerp(const rgba& a, const rgba& b, float f)
		{
			m_r = (Uint8) frnd(flerp(a.m_r, b.m_r, f));
			m_g = (Uint8) frnd(flerp(a.m_g, b.m_g, f));
			m_b = (Uint8) frnd(flerp(a.m_b, b.m_b, f));
			m_a = (Uint8) frnd(flerp(a.m_a, b.m_a, f));
		}

		void	print();
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


	struct rect
	{
		float	m_x_min, m_x_max, m_y_min, m_y_max;

		void	read(stream* in);
		void	print() const;
		bool	point_test(float x, float y) const;
		void	expand_to_point(float x, float y);
		float width() const { return m_x_max-m_x_min; }
		float height() const { return m_y_max-m_y_min; }

		point	get_corner(int i)
		// Get one of the rect verts.
		{
			assert(i >= 0 && i < 4);
			return point(
				(i == 0 || i == 3) ? m_x_min : m_x_max,
				(i < 2) ? m_y_min : m_y_max);
		}
	};


	struct font;
	
	// A polymorphic class for handling different types of
	// resources.
	struct resource
	{
		virtual ~resource() {}

		// Override in derived classes that implement font interface.
		virtual font*	cast_to_font() { return 0; }
	};

};	// end namespace gameswf


#endif // GAMESWF_TYPES_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
