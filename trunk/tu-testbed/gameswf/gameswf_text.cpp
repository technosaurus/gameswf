// gameswf_text.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code for the text tags.


#include "gameswf_impl.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_log.h"
#include "gameswf_font.h"
#include "gameswf_render.h"


namespace gameswf
{
	//
	// text_character
	//


	// Helper struct.
	struct text_style
	{
		font*	m_font;
		rgba	m_color;
		float	m_x_offset;
		float	m_y_offset;
		float	m_text_height;
		bool	m_has_x_offset;
		bool	m_has_y_offset;

		text_style()
			:
			m_font(0),
			m_x_offset(0),
			m_y_offset(0),
			m_text_height(1.0f),
			m_has_x_offset(false),
			m_has_y_offset(false)
		{
		}
	};


	// Helper struct.
	struct text_glyph_record
	{
		struct glyph_entry
		{
			int	m_glyph_index;
			float	m_glyph_advance;
		};
		text_style	m_style;
		array<glyph_entry>	m_glyphs;

		void	read(stream* in, int glyph_count, int glyph_bits, int advance_bits)
		{
			m_glyphs.resize(glyph_count);
			for (int i = 0; i < glyph_count; i++)
			{
				m_glyphs[i].m_glyph_index = in->read_uint(glyph_bits);
				m_glyphs[i].m_glyph_advance = (float) in->read_sint(advance_bits);
			}
		}
	};


	// Render the given glyph records.
	static void	display_glyph_records(
		const display_info& di,
		const matrix& mat,
		const array<text_glyph_record>& records)
	{
		assert(di.m_movie);

		static array<fill_style>	s_dummy_style;	// used to pass a color on to shape_character::display()
		static array<line_style>	s_dummy_line_style;
		s_dummy_style.resize(1);

		display_info	sub_di = di;
		sub_di.m_matrix.concatenate(mat);

		matrix	base_matrix = sub_di.m_matrix;
		float	base_matrix_max_scale = base_matrix.get_max_scale();

		float	scale = 1.0f;
		float	x = 0.0f;
		float	y = 0.0f;

		for (int i = 0; i < records.size(); i++)
		{
			// Draw the characters within the current record; i.e. consecutive
			// chars that share a particular style.
			const text_glyph_record&	rec = records[i];

			if (rec.m_style.m_font == NULL) continue;

			scale = rec.m_style.m_text_height / 1024.0f;	// the EM square is 1024 x 1024
			float	text_screen_height = base_matrix_max_scale
				* scale
				* 1024.0f
				/ 20.0f
				* di.m_movie->get_pixel_scale();
			bool	use_shape_glyphs =
				text_screen_height > fontlib::get_nominal_texture_glyph_height() * 1.0f;

			if (rec.m_style.m_has_x_offset)
			{
				x = rec.m_style.m_x_offset;
			}
			if (rec.m_style.m_has_y_offset)
			{
				y = rec.m_style.m_y_offset;
			}

			s_dummy_style[0].set_color(rec.m_style.m_color);

			rgba	transformed_color = sub_di.m_color_transform.transform(rec.m_style.m_color);

			for (int j = 0; j < rec.m_glyphs.size(); j++)
			{
				int	index = rec.m_glyphs[j].m_glyph_index;
				const texture_glyph*	tg = rec.m_style.m_font->get_texture_glyph(index);
					
				sub_di.m_matrix = base_matrix;
				sub_di.m_matrix.concatenate_translation(x, y);
				sub_di.m_matrix.concatenate_scale(scale);

				if (tg && ! use_shape_glyphs)
				{
					fontlib::draw_glyph(sub_di.m_matrix, tg, transformed_color);
				}
				else
				{
					shape_character*	glyph = rec.m_style.m_font->get_glyph(index);

					// Draw the character using the filled outline.
					if (glyph) glyph->display(sub_di, s_dummy_style, s_dummy_line_style);
				}

				x += rec.m_glyphs[j].m_glyph_advance;
			}
		}
	}


	struct text_character : public character
	{
		rect	m_rect;
		matrix	m_matrix;
		array<text_glyph_record>	m_text_glyph_records;

		text_character()
		{
		}

		void	read(stream* in, int tag_type, movie_definition* m)
		{
			assert(m != NULL);
			assert(tag_type == 11 || tag_type == 33);

			m_rect.read(in);
			m_matrix.read(in);

			int	glyph_bits = in->read_u8();
			int	advance_bits = in->read_u8();

			IF_VERBOSE_PARSE(log_msg("begin text records\n"));

			text_style	style;
			for (;;)
			{
				int	first_byte = in->read_u8();
				
				if (first_byte == 0)
				{
					// This is the end of the text records.
					IF_VERBOSE_PARSE(log_msg("end text records\n"));
					break;
				}

				int	type = (first_byte >> 7) & 1;
				if (type == 1)
				{
					// This is a style change.

					bool	has_font = (first_byte >> 3) & 1;
					bool	has_color = (first_byte >> 2) & 1;
					bool	has_y_offset = (first_byte >> 1) & 1;
					bool	has_x_offset = (first_byte >> 0) & 1;

					IF_VERBOSE_PARSE(log_msg("text style change\n"));

					if (has_font)
					{
						Uint16	font_id = in->read_u16();
						style.m_font = m->get_font(font_id);
						if (style.m_font == NULL)
						{
							log_error("error: text style with undefined font; font_id = %d\n", font_id);
						}

						IF_VERBOSE_PARSE(log_msg("has_font: font id = %d\n", font_id));
					}
					if (has_color)
					{
						if (tag_type == 11)
						{
							style.m_color.read_rgb(in);
						}
						else
						{
							assert(tag_type == 33);
							style.m_color.read_rgba(in);
						}
						IF_VERBOSE_PARSE(log_msg("has_color\n"));
					}
					if (has_x_offset)
					{
						style.m_has_x_offset = true;
						style.m_x_offset = in->read_s16();
						IF_VERBOSE_PARSE(log_msg("has_x_offset = %g\n", style.m_x_offset));
					}
					else
					{
						style.m_has_x_offset = false;
						style.m_x_offset = 0.0f;
					}
					if (has_y_offset)
					{
						style.m_has_y_offset = true;
						style.m_y_offset = in->read_s16();
						IF_VERBOSE_PARSE(log_msg("has_y_offset = %g\n", style.m_y_offset));
					}
					else
					{
						style.m_has_y_offset = false;
						style.m_y_offset = 0.0f;
					}
					if (has_font)
					{
						style.m_text_height = in->read_u16();
						IF_VERBOSE_PARSE(log_msg("text_height = %g\n", style.m_text_height));
					}
				}
				else
				{
					// Read the glyph record.
					int	glyph_count = first_byte & 0x7F;
					m_text_glyph_records.resize(m_text_glyph_records.size() + 1);
					m_text_glyph_records.back().m_style = style;
					m_text_glyph_records.back().read(in, glyph_count, glyph_bits, advance_bits);

					IF_VERBOSE_PARSE(log_msg("glyph_records: count = %d\n", glyph_count));
				}
			}
		}


		void	display(const display_info& di)
		// Draw the string.
		{
			display_glyph_records(di, m_matrix, m_text_glyph_records);
		}
	};


	void	define_text_loader(stream* in, int tag_type, movie_definition* m)
	// Read a DefineText tag.
	{
		assert(tag_type == 11 || tag_type == 33);

		Uint16	character_id = in->read_u16();
		
		text_character*	ch = new text_character;
		ch->set_id(character_id);
		IF_VERBOSE_PARSE(log_msg("text_character, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		// IF_VERBOSE_PARSE(print some stuff);

		m->add_character(character_id, ch);
	}


	//
	// edit_text_character
	//


	struct edit_text_character : public character
	// A text display character, whose text can be changed at
	// runtime (by script or host).
	{
		rect	m_rect;
		array<text_glyph_record>	m_text_glyph_records;
		array<fill_style>	m_dummy_style;	// used to pass a color on to shape_character::display()
		array<line_style>	m_dummy_line_style;

		bool	m_word_wrap;
		bool	m_multiline;
		bool	m_password;	// show asterisks instead of actual characters
		bool	m_readonly;
		bool	m_auto_size;	// resize our bound to fit the text
		bool	m_no_select;
		bool	m_border;	// forces white background and black border -- perhaps useless?
		bool	m_html;

		// Allowed HTML (from Alexi's SWF Reference):
		//
		// <a href=url target=targ>...</a> -- hyperlink
		// <b>...</b> -- bold
		// <br> -- line break
		// <font face=name size=[+|-][0-9]+ color=#RRGGBB>...</font>  -- font change; size in TWIPS
		// <i>...</i> -- italic
		// <li>...</li> -- list item
		// <p>...</p> -- paragraph
		// <tab> -- insert tab
		// <TEXTFORMAT>  </TEXTFORMAT>
		//   [ BLOCKINDENT=[0-9]+ ]
		//   [ INDENT=[0-9]+ ]
		//   [ LEADING=[0-9]+ ]
		//   [ LEFTMARGIN=[0-9]+ ]
		//   [ RIGHTMARGIN=[0-9]+ ]
		//   [ TABSTOPS=[0-9]+{,[0-9]+} ]
		//
		// Change the different parameters as indicated. The
		// sizes are all in TWIPs. There can be multiple
		// positions for the tab stops. These are seperated by
		// commas.
		// <U>...</U> -- underline


		bool	m_use_outlines;	// when true, use specified SWF internal font.  Otherwise, renderer picks a default font

		font*	m_font;
		float	m_text_height;

		rgba	m_color;
		int	m_max_length;

		enum alignment
		{
			ALIGN_LEFT = 0,
			ALIGN_RIGHT,
			ALIGN_CENTER,
			ALIGN_JUSTIFY	// probably don't need to implement...
		};
		alignment	m_alignment;
		
		float	m_left_margin;	// extra space between box border and text
		float	m_right_margin;
		float	m_indent;	// how much to indent the first line of multiline text
		float	m_leading;	// extra space between lines (in addition to default font line spacing)
		tu_string	m_text;


		edit_text_character()
			:
			m_word_wrap(false),
			m_multiline(false),
			m_password(false),
			m_readonly(false),
			m_auto_size(false),
			m_no_select(false),
			m_border(false),
			m_html(false),
			m_use_outlines(false),
			m_font(NULL),
			m_text_height(1.0f),
			m_max_length(0),
			m_alignment(ALIGN_LEFT),
			m_left_margin(0.0f),
			m_right_margin(0.0f),
			m_indent(0.0f),
			m_leading(0.0f)
		{
			m_color.set(0, 0, 0, 255);
			m_dummy_style.push_back(fill_style());
		}

		
		~edit_text_character()
		{
		}


		virtual bool	set_value(const as_value& new_val)
		// Overload from class character.
		// Set our text to the given string.
		{
			set_text(new_val.to_string());
			return true;
		}

		virtual bool	get_value(as_value* val)
		// Return our contents.
		{
			val->set(m_text);
			return true;
		}

		void	set_text(const char* new_text)
		{
			if (m_text == new_text)
			{
				return;
			}

			m_text = new_text;
			if (m_max_length > 0
			    && m_text.length() > m_max_length)
			{
				m_text.resize(m_max_length);
			}

			format_text();
		}

		
		// @@ WIDTH_FUDGE is a total fudge to make it match the Flash player!  Maybe
		// we have a bug?
		#define WIDTH_FUDGE 80.0f


		void	align_line(alignment align, int last_line_start_record, float x)
		// Does LEFT/CENTER/RIGHT alignment on the records in
		// m_text_glyph_records[], starting with
		// last_line_start_record and going through the end of
		// m_text_glyph_records.
		{
			float	extra_space = (m_rect.width() - m_right_margin) - x - WIDTH_FUDGE;
			assert(extra_space >= 0.0f);

			float	shift_right = 0.0f;

			if (align == ALIGN_LEFT)
			{
				// Nothing to do; already aligned left.
				return;
			}
			else if (align == ALIGN_CENTER)
			{
				// Distribute the space evenly on both sides.
				shift_right = extra_space / 2;
			}
			else if (align == ALIGN_RIGHT)
			{
				// Shift all the way to the right.
				shift_right = extra_space;
			}

			// Shift the beginnings of the records on this line.
			for (int i = last_line_start_record; i < m_text_glyph_records.size(); i++)
			{
				text_glyph_record&	rec = m_text_glyph_records[i];

				if (rec.m_style.m_has_x_offset)
				{
					rec.m_style.m_x_offset += shift_right;
				}
			}
		}
		

		// Convert the characters in m_text into a series of
		// text_glyph_records to be rendered.
		void	format_text()
		{
			m_text_glyph_records.resize(0);

			if (m_font == NULL) return;

			float	scale = m_text_height / 1024.0f;	// the EM square is 1024 x 1024

			text_glyph_record	rec;	// one to work on
			rec.m_style.m_font = m_font;
			rec.m_style.m_color = m_color;
			rec.m_style.m_x_offset = fmax(0, m_left_margin + m_indent);
			rec.m_style.m_y_offset = m_text_height + (m_font->get_leading() - m_font->get_descent()) * scale;
			rec.m_style.m_text_height = m_text_height;
			rec.m_style.m_has_x_offset = true;
			rec.m_style.m_has_y_offset = true;

			float	x = rec.m_style.m_x_offset;
			float	y = rec.m_style.m_y_offset;
			UNUSED(y);

			float	leading = m_leading;
			leading += m_font->get_leading() * scale;

			int	last_code = -1;
			int	last_space_glyph = -1;
			int	last_line_start_record = 0;

			for (int j = 0; j < m_text.length(); j++)
			{
				if (y + m_font->get_descent() * scale > m_rect.height())
				{
					// Text goes below the bottom of our bounding box.
					rec.m_glyphs.resize(0);
					break;
				}

				Uint16	code = m_text[j];

				x += m_font->get_kerning_adjustment(last_code, code) * scale;
				last_code = code;

				if (code == 13 || code == 10)
				{
					// newline.

					// Frigging Flash seems to use '\r' (13) as its
					// default newline character.  If we get DOS-style \r\n
					// sequences, it'll show up as double newlines, so maybe we
					// need to detect \r\n and treat it as one newline.

					// Close out this stretch of glyphs.
					m_text_glyph_records.push_back(rec);
					align_line(m_alignment, last_line_start_record, x);

					x = fmax(0, m_left_margin + m_indent);	// new paragraphs get the indent.
					y += m_text_height + leading;

					// Start a new record on the next line.
					rec.m_glyphs.resize(0);
					rec.m_style.m_font = m_font;
					rec.m_style.m_color = m_color;
					rec.m_style.m_x_offset = x;
					rec.m_style.m_y_offset = y;
					rec.m_style.m_text_height = m_text_height;
					rec.m_style.m_has_x_offset = true;
					rec.m_style.m_has_y_offset = true;

					last_space_glyph = -1;
					last_line_start_record = m_text_glyph_records.size();

					continue;
				}

				// Remember where word breaks occur.
				if (code == 32)
				{
					last_space_glyph = rec.m_glyphs.size();
				}

				int	index = m_font->get_glyph_index(code);
				if (index == -1)
				{
					// error -- missing glyph!
					log_error("edit_text_character::display() -- missing glyph for char %d\n", code);
					continue;
				}
				text_glyph_record::glyph_entry	ge;
				ge.m_glyph_index = index;
				ge.m_glyph_advance = scale * m_font->get_advance(index);

				rec.m_glyphs.push_back(ge);

				x += ge.m_glyph_advance;

				if (x >= m_rect.width() - m_right_margin - WIDTH_FUDGE)
				{
					// Whoops, we just exceeded the box width.  Do word-wrap.

					// Insert newline.

					// Close out this stretch of glyphs.
					m_text_glyph_records.push_back(rec);
					float	previous_x = x;

					x = m_left_margin;
					y += m_text_height + leading;

					// Start a new record on the next line.
					rec.m_glyphs.resize(0);
					rec.m_style.m_font = m_font;
					rec.m_style.m_color = m_color;
					rec.m_style.m_x_offset = x;
					rec.m_style.m_y_offset = y;
					rec.m_style.m_text_height = m_text_height;
					rec.m_style.m_has_x_offset = true;
					rec.m_style.m_has_y_offset = true;
					
					text_glyph_record&	last_line = m_text_glyph_records.back();
					if (last_space_glyph == -1)
					{
						// Pull the previous glyph down onto the
						// new line.
						if (last_line.m_glyphs.size() > 0)
						{
							rec.m_glyphs.push_back(last_line.m_glyphs.back());
							x += last_line.m_glyphs.back().m_glyph_advance;
							previous_x -= last_line.m_glyphs.back().m_glyph_advance;
							last_line.m_glyphs.resize(last_line.m_glyphs.size() - 1);
						}
					}
					else
					{
						// Move the previous word down onto the next line.

						previous_x -= last_line.m_glyphs[last_space_glyph].m_glyph_advance;

						for (int i = last_space_glyph + 1; i < last_line.m_glyphs.size(); i++)
						{
							rec.m_glyphs.push_back(last_line.m_glyphs[i]);
							x += last_line.m_glyphs[i].m_glyph_advance;
							previous_x -= last_line.m_glyphs[i].m_glyph_advance;
						}
						last_line.m_glyphs.resize(last_space_glyph);
					}

					align_line(m_alignment, last_line_start_record, previous_x);

					last_space_glyph = -1;
					last_line_start_record = m_text_glyph_records.size();
				}


				// TODO: alignment
				// TODO: HTML markup
			}

			// Add this line to our output.
			m_text_glyph_records.push_back(rec);
			align_line(m_alignment, last_line_start_record, x);
		}


		void	read(stream* in, int tag_type, movie_definition* m)
		{
			assert(m != NULL);
			assert(tag_type == 37);

			m_rect.read(in);

			in->align();
			bool	has_text = in->read_uint(1) ? true : false;
			m_word_wrap = in->read_uint(1) ? true : false;
			m_multiline = in->read_uint(1) ? true : false;
			m_password = in->read_uint(1) ? true : false;
			m_readonly = in->read_uint(1) ? true : false;
			bool	has_color = in->read_uint(1) ? true : false;
			bool	has_max_length = in->read_uint(1) ? true : false;
			bool	has_font = in->read_uint(1) ? true : false;

			in->read_uint(1);	// reserved
			m_auto_size = in->read_uint(1) ? true : false;
			bool	has_layout = in->read_uint(1) ? true : false;
			m_no_select = in->read_uint(1) ? true : false;
			m_border = in->read_uint(1) ? true : false;
			in->read_uint(1);	// reserved
			m_html = in->read_uint(1) ? true : false;
			m_use_outlines = in->read_uint(1) ? true : false;

			if (has_font)
			{
				Uint16	font_id = in->read_u16();
				m_font = m->get_font(font_id);
				if (m_font == NULL)
				{
					log_error("error: edit_text with undefined font; font_id = %d\n", font_id);
				}

				m_text_height = (float) in->read_u16();
			}

			if (has_color)
			{
				m_color.read_rgba(in);
			}

			if (has_max_length)
			{
				m_max_length = in->read_u16();
			}

			if (has_layout)
			{
				m_alignment = (alignment) in->read_u8();
				m_left_margin = (float) in->read_u16();
				m_right_margin = (float) in->read_u16();
				m_indent = (float) in->read_s16();
				m_leading = (float) in->read_s16();
			}

			char*	name = in->read_string();
			set_name(name);
			delete [] name;

			if (has_text)
			{
				char*	str = in->read_string();
				set_text(str);
				delete [] str;
			}
		}


		void	display(const display_info& di)
		// Draw the dynamic string.
		{
			if (m_border)
			{
				// Show white background + black bounding box.
				render::set_matrix(di.m_matrix);

				point	coords[4];
				coords[0] = m_rect.get_corner(0);
				coords[1] = m_rect.get_corner(1);
				coords[2] = m_rect.get_corner(3);
				coords[3] = m_rect.get_corner(2);

				Sint16	icoords[18] = 
				{
					// strip (fill in)
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
					(Sint16) coords[1].m_x, (Sint16) coords[1].m_y,
					(Sint16) coords[2].m_x, (Sint16) coords[2].m_y,
					(Sint16) coords[3].m_x, (Sint16) coords[3].m_y,

					// outline
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
					(Sint16) coords[1].m_x, (Sint16) coords[1].m_y,
					(Sint16) coords[3].m_x, (Sint16) coords[3].m_y,
					(Sint16) coords[2].m_x, (Sint16) coords[2].m_y,
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
				};
				
				render::fill_style_color(0, rgba(255, 255, 255, 255));
				render::draw_mesh_strip(&icoords[0], 4);

				render::line_style_color(rgba(0,0,0,255));
				render::draw_line_strip(&icoords[8], 5);
			}

			// Draw our actual text.
			display_glyph_records(di, matrix::identity, m_text_glyph_records);
		}
	};


	void	define_edit_text_loader(stream* in, int tag_type, movie_definition* m)
	// Read a DefineText tag.
	{
		assert(tag_type == 37);

		Uint16	character_id = in->read_u16();

		edit_text_character*	ch = new edit_text_character;
		ch->set_id(character_id);
		IF_VERBOSE_PARSE(log_msg("edit_text_char, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		m->add_character(character_id, ch);
	}
}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

