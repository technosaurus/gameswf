// gameswf_styles.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Fill and line style types.


#ifndef GAMESWF_STYLES_H
#define GAMESWF_STYLES_H


#include "gameswf_impl.h"


namespace gameswf
{
	struct stream;


	struct gradient_record
	{
		gradient_record();
		void	read(stream* in, int tag_type);

	//data:
		Uint8	m_ratio;
		rgba	m_color;
	};


	struct fill_style
	// For the interior of outline shapes.
	{
		fill_style();

		void	read(stream* in, int tag_type, movie* m);
		rgba	sample_gradient(int ratio) const;
		gameswf::render::bitmap_info*	create_gradient_bitmap() const;
		void	apply(int fill_side) const;

		rgba	get_color() const { return m_color; }
		void	set_color(rgba new_color) { m_color = new_color; }

	private:
		int	m_type;
		rgba	m_color;
		matrix	m_gradient_matrix;
		array<gradient_record>	m_gradients;
		gameswf::render::bitmap_info*	m_gradient_bitmap_info;
		bitmap_character*	m_bitmap_character;
		matrix	m_bitmap_matrix;
	};


	struct line_style
	// For the outside of outline shapes, or just bare lines.
	{
		line_style();
		void	read(stream* in, int tag_type);
		void	apply() const;

	private:
		Uint16	m_width;	// in TWIPS
		rgba	m_color;
	};

}


#endif // GAMESWF_STYLES_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
