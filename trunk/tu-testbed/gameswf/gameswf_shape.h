// gameswf_shape.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier outline shapes, the basis for most SWF rendering.


#ifndef GAMESWF_SHAPE_H
#define GAMESWF_SHAPE_H


#include "gameswf_styles.h"


namespace gameswf
{
	struct stream;


	struct edge
	// Together with the previous anchor, defines a quadratic
	// curve segment.
	{
		edge();
		edge(float cx, float cy, float ax, float ay);
		void	emit_curve() const;

	//private:
		// *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
		float	m_cx, m_cy;		// "control" point
		float	m_ax, m_ay;		// "anchor" point
	};


	struct path
	// A subset of a shape -- a series of edges sharing a single set
	// of styles.
	{
		path();
		path(float ax, float ay, int fill0, int fill1, int line);

		void	reset(float ax, float ay, int fill0, int fill1, int line);
		bool	is_empty() const;

		void	display(
			const display_info& di,
			const array<fill_style>& fill_styles,
			const array<line_style>& line_styles) const;
		bool	point_test(float x, float y);

	//private:
		int	m_fill0, m_fill1, m_line;
		float	m_ax, m_ay;	// starting point
		array<edge>	m_edges;
		bool	m_new_shape;
	};


	struct shape_character : public character
	// Represents the outline of one or more shapes, along with
	// information on fill and line styles.
	{
		shape_character();
		virtual ~shape_character();

		void	read(stream* in, int tag_type, bool with_style, movie* m);
		void	display(const display_info& di);
		void	display(const display_info& di, const array<fill_style>& fill_styles) const;
		bool	point_test(float x, float y);
		void	get_bounds(rect* r) const;

	//private:
		rect	m_bound;
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<path>	m_paths;
	};

}	// end namespace gameswf


#endif // GAMESWF_SHAPE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
