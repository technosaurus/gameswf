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
	struct shape_character;
	namespace tesselate { struct trapezoid_accepter; }


	struct edge
	// Together with the previous anchor, defines a quadratic
	// curve segment.
	{
		edge();
		edge(float cx, float cy, float ax, float ay);
		void	tesselate_curve() const;

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

		bool	point_test(float x, float y);

		// Push the path into the tesselator.
		void	tesselate() const;

	//private:
		int	m_fill0, m_fill1, m_line;
		float	m_ax, m_ay;	// starting point
		array<edge>	m_edges;
		bool	m_new_shape;
	};


	struct mesh
	// For holding a pre-tesselated shape.
	{
		mesh();

		void	add_triangle(const point& a, const point& b, const point& c);

		void	display(const fill_style& style) const;

		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);
	private:
		array<point>	m_triangle_list;	// 3 points per tri
	};


	struct line_strip
	// For holding a line-strip (i.e. polyline).
	{
		line_strip();
		line_strip(int style, const point coords[], int coord_count);

		void	display(const line_style& style) const;

		int	get_style() const { return m_style; }
		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);
	private:
		int	m_style;
		array<point>	m_coords;
	};


	struct mesh_set
	// A whole shape, tesselated to a certain error tolerance.
	{
		mesh_set();
		mesh_set(const shape_character* sh, float error_tolerance);

//		int	get_last_frame_rendered() const;
//		void	set_last_frame_rendered(int frame_counter);
		float	get_error_tolerance() const { return m_error_tolerance; }

		void display(
			const display_info& di,
			const array<fill_style>& fills,
			const array<line_style>& line_styles) const;

		void	add_triangle(int style, const point& a, const point& b, const point& c);
		void	add_line_strip(int style, const point coords[], int coord_count);

		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);

	private:
//		int	m_last_frame_rendered;	// @@ Hm, we shouldn't spontaneously drop cached data I don't think...
		float	m_error_tolerance;
		array<mesh>	m_meshes;	// One mesh per style.
		array<line_strip>	m_line_strips;
	};


	struct shape_character : public character
	// Represents the outline of one or more shapes, along with
	// information on fill and line styles.
	{
		shape_character();
		virtual ~shape_character();

		void	read(stream* in, int tag_type, bool with_style, movie* m);
		void	display(const display_info& di);
		void	display(
			const display_info& di,
			const array<fill_style>& fill_styles,
			const array<line_style>& line_styles) const;
		void	tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const;
		bool	point_test(float x, float y);
		const rect&	get_bound() const { return m_bound; }
		void	compute_bound(rect* r) const;

		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);

	private:
		void	sort_and_clean_meshes(int display_number) const;
		
		rect	m_bound;
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<path>	m_paths;

		// Cached pre-tesselated meshes.
		mutable array<mesh_set*>	m_cached_meshes;
	};

}	// end namespace gameswf


#endif // GAMESWF_SHAPE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
