// gameswf_font.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A font type for gameswf.


#include "gameswf_font.h"
#include "gameswf_stream.h"
#include "gameswf_impl.h"


namespace gameswf
{
	font::font()
		:
		m_name(NULL)
	{
	}

	font::~font()
	{
		// Iterate over m_glyphs and free the shapes.
		for (int i = 0; i < m_glyphs.size(); i++)
		{
			delete_shape_character(m_glyphs[i]);
		}
		m_glyphs.resize(0);

		// Delete the name string.
		if (m_name)
		{
			delete [] m_name;
			m_name = NULL;
		}
	}

	shape_character*	font::get_glyph(int index) const
	{
		if (index >= 0 && index < m_glyphs.size())
		{
			return m_glyphs[index];
		}
		else
		{
			return NULL;
		}
	}

	void	font::read(stream* in, int tag_type, movie* m)
	{
		if (tag_type == 10)
		{
			IF_DEBUG(printf("reading DefineFont\n"));

			int	table_base = in->get_position();

			// Read the glyph offsets.  Offsets
			// are measured from the start of the
			// offset table.
			array<int>	offsets;
			offsets.push_back(in->read_u16());
			IF_DEBUG(printf("offset[0] = %d\n", offsets[0]));
			int	count = offsets[0] >> 1;
			for (int i = 1; i < count; i++)
			{
				offsets.push_back(in->read_u16());
				IF_DEBUG(printf("offset[%d] = %d\n", i, offsets[i]));
			}

			m_glyphs.reserve(count);

			// Read the glyph shapes.
			{for (int i = 0; i < count; i++)
			{
				// Seek to the start of the shape data.
				int	new_pos = table_base + offsets[i];
				in->set_position(new_pos);

				// Create & read the shape.
				shape_character*	s = create_shape_character(in, 2, false, m);
				m_glyphs.push_back(s);
			}}
		}
		else if (tag_type == 48)
		{
			IF_DEBUG(printf("reading DefineFont2\n"));

			int	has_layout = in->read_uint(1);
			int	shift_jis = in->read_uint(1);
			int	unicode = in->read_uint(1);
			int	ansi = in->read_uint(1);
			int	wide_offsets = in->read_uint(1);
			int	wide_codes = in->read_uint(1);
			int	italic = in->read_uint(1);
			int	bold = in->read_uint(1);
			Uint8	reserved = in->read_u8();

			m_name = in->read_string_with_length();

			// Avoid warnings.
			reserved = reserved;
			unicode = unicode;
			bold = bold;
			italic = italic;
			wide_codes = wide_codes;
			ansi = ansi;
			shift_jis = shift_jis;
			has_layout = has_layout;

			int	glyph_count = in->read_u16();
			
			int	table_base = in->get_position();

			// Read the glyph offsets.  Offsets
			// are measured from the start of the
			// offset table.
			array<int>	offsets;
			if (wide_offsets)
			{
				// 32-bit offsets.
				for (int i = 0; i < glyph_count; i++)
				{
					offsets.push_back(in->read_u32());
				}
			}
			else
			{
				// 16-bit offsets.
				for (int i = 0; i < glyph_count; i++)
				{
					offsets.push_back(in->read_u16());
				}
			}

			int	font_code_offset;
			if (wide_offsets)
			{
				font_code_offset = in->read_u32();
			}
			else
			{
				font_code_offset = in->read_u16();
			}

			m_glyphs.reserve(glyph_count);

			// Read the glyph shapes.
			{for (int i = 0; i < glyph_count; i++)
			{
				// Seek to the start of the shape data.
				int	new_pos = table_base + offsets[i];
				assert(new_pos >= in->get_position());	// if we're seeking backwards, then that looks like a bug.
				in->set_position(new_pos);

				// Create & read the shape.
				shape_character*	s = create_shape_character(in, 22, false, m);
				m_glyphs.push_back(s);
			}}

			// Read code table...
			// in->set_position(table_base + font_code_offset);
			// if (wide_codes) { read glyph_count * u16(); }
			// else { read glyph_count * u8(); }
			// put codes in a hash table

			// if (has_layout)
			// {
			//    ascender height = s16();
			//    descender height = s16();
			//    leading height = s16();
			//    advance table = glyph_count * s16();
			//    bounds table = glyph_count * rect();
			//    font kerning count = u16();
			//    kerning info = font kerning count * kerning_record;
			// }

			// kerning record:
			// if (wide_codes) { code1 = u16(); } else { code1 = u8(); }
			// if (wide_codes) { code2 = u16(); } else { code2 = u8(); }
			// adjustment = s16(); // relative to advance values
		}
		else
		{
			IF_DEBUG(printf("*** define font tag type %d not implemented ***", tag_type));
			assert(0);
		}
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
