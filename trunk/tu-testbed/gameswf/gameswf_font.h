// gameswf_font.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A font type for gameswf.


#ifndef GAMESWF_FONT_H


#include "engine/container.h"
#include "gameswf_types.h"


namespace gameswf
{
	struct movie;
	struct shape_character;
	struct stream;

	struct font
	{
		font();
		~font();
		shape_character*	get_glyph(int index) const;
		void	read(stream* in, int tag_type, movie* m);
		void	read_font_info(stream* in);

	private:
		void	read_code_table(stream* in);

		array<shape_character*>	m_glyphs;
		char*	m_name;
		bool	m_has_layout;
		bool	m_unicode_chars;
		bool	m_shift_jis_chars;
		bool	m_ansi_chars;
		bool	m_is_italic;
		bool	m_is_bold;
		bool	m_wide_codes;
		array<Uint16>	m_code_table;	// m_code_table[glyph_index] = character code

		// Layout stuff.
		float	m_ascent;
		float	m_descent;
		float	m_leading;
		array<float>	m_advance_table;
		array<rect>	m_bounds_table;

		struct kerning_pair
		{
			Uint16	m_char0, m_char1;

			bool	operator==(const kerning_pair& k) const
			{
				return m_char0 == k.m_char0 && m_char1 == k.m_char1;
			}
		};
		hash<kerning_pair, float>	m_kerning_pairs;
	};

};	// end namespace gameswf



#endif // GAMESWF_FONT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
