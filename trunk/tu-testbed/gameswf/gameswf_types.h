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

	struct rgba
	{
		Uint8	m_r, m_g, m_b, m_a;

		rgba() : m_r(255), m_g(255), m_b(255), m_a(255) {}

		rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
			:
			m_r(r), m_g(g), m_b(b), m_a(a)
		{
		}

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
