// swf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation details for SWF parser.


#ifndef SWF_IMPL_H
#define SWF_IMPL_H


#include "SDL.h"

#include "swf.h"


namespace swf {

	// stream is used to encapsulate bit-packed file reads.
	struct stream {
		SDL_RWops*	m_input;
		Uint8	m_current_byte;
		Uint8	m_unused_bits;

		array<int>	m_tag_stack;	// position of end of tag


		stream(SDL_RWops* input)
			: m_input(input),
			m_current_byte(0),
			m_unused_bits(0)
		{
		}


		int	read_uint(int bitcount)
		// Reads a bit-packed unsigned integer from the
		// stream.  The given bitcount determines the number
		// of bits to read.
		{
			assert(bitcount <= 32 && bitcount > 0);
			
			Uint32	value = 0;

			while (bitcount)
			{
				if (unused_bits) {
					if (bitcount >= unused_bits) {
						// Consume all the unused bits.
						value <<= unused_bits;
						value |= m_current_byte;

						bitcount -= unused_bits;

						m_current_byte = 0;
						m_unused_bits = 0;

					} else {
						// Consume some of the unused bits.
						value <<= bitcount;
						value |= m_current_byte & ((1 << bitcount) - 1);
						m_current_byte >>= bitcount;
						m_unused_bits -= bitcount;

						bitcount = 0;
					}
				} else {
					m_current_byte = SDL_ReadByte(m_input);
					m_unused_bits = 8;
				}
			}
		}


		int	read_sint(int bitcount)
		// Reads a bit-packed little-endian signed integer
		// from the stream.  The given bitcount determines the
		// number of bits to read.
		{
			assert(bitcount <= 32 && bitcount > 0);

			Sint32	value = (Sint32) read_uint(bitcount);

			// Sign extend...
			if (value & (1 << (bitcount - 1))) {
				value |= -1 << bitcount;
			}
		}


		float	read_fixed()
		{
			m_unused_bits = 0;
			Sint32	val = SDL_ReadLE32(m_input);
			return (float) val / 65536.0f;
		}


		Uint8	read_u8() { m_unused_bits = 0; return SDL_ReadByte(m_input); }
		Sint8	read_s8() { m_unused_bits = 0; return SDL_ReadByte(m_input); }
		Uint16	read_u16() { m_unused_bits = 0; return SDL_ReadLE16(m_input); }
		Sint16	read_s16() { m_unused_bits = 0; return SDL_ReadLE16(m_input); }


		int	get_position()
		// Return our current (byte) position in the input stream.
		{
			return SDL_RWtell(m_input);
		}

		int	open_tag()
		{
			int	tag_header = SDL_ReadBE16(m_input);
			int	tag_type = tag_header >> 6;
			int	tag_length = tag_header & 0x3f;
			if (tag_length == 0x3f) {
				tag_length = SDL_ReadBE32(m_input);
			}

			// Remember where the end of the tag is, so we can
			// fast-forward past it when we're done reading it.
			m_tag_stack.push_back(get_position() + tag_length);
		}
		


		void	close_tag()
		// Seek to the end of the most-recently-opened tag.
		{
			assert(m_tag_stack.size() > 0);
			int	end_pos = m_tag_stack.pop_back();
			SDL_RWseek(m_input, end_pos);

			m_unused_bits = 0;
		}
	};


	// RGB --> 24 bit color

	// RGBA
	struct rgba {
		Uint8	m_r, m_g, m_b, m_a;

		void	read(stream* in)
		{
			read_rgb(in);
			m_a = in.read_uint(8);
		}

		void	read_rgb(stream* in)
		{
			m_r = in.read_uint(8);
			m_g = in.read_uint(8);
			m_b = in.read_uint(8);
			m_a = 0x0FF;
		}
	};


	struct rect {
		float	m_x_min, m_x_max, m_y_min, m_y_max;

		void	read(stream* in)
		{
			int	nbits = in.read_uint(5);
			m_x_min = in.read_sint(nbits);
			m_x_max = in.read_sint(nbits);
			m_y_min = in.read_sint(nbits);
			m_y_max = in.read_sint(nbits);
		}
	};


	struct matrix {
		float	m_[2][3];

		void	read(stream* in)
		{
			int	has_scale = in.read_uint(1);
			int	scale_nbits = in.read_uint(5);
			int	scale_x = ;
			int	scale_y = ;

			int	has_rotate = in.read_uint(1);
			int	rotate_nbits = in.read_uint(5);

			int	rs0;
			int	rs1;

			int	translate_nbits = in.read_uint(5);
			int	translate_x = in.read_sint(translate_nbits);
			int	translate_y = in.read_sint(translate_nbits);
		}
	};


	struct cxform {
		float	m_[4][2];	// [RGBA][mult, add]

		void	read_rgb(stream* in)
		{
			int	has_add = in.read_uint(1);
			int	has_mult = in.read_uint(1);
			int	nbits = in.read_uint(4);

			if (has_mult) {
				m_[0][0] = in.read_sint(nbits);
				m_[1][0] = in.read_sint(nbits);
				m_[2][0] = in.read_sint(nbits);
				m_[3][0] = 1;
			}
			if (has_add) {
				m_[0][1] = in.read_sint(nbits);
				m_[1][1] = in.read_sint(nbits);
				m_[2][1] = in.read_sint(nbits);
				m_[3][1] = 1;
			}
		}

		void	read_rgba(stream* in)
		{
			int	has_add = in.read_uint(1);
			int	has_mult = in.read_uint(1);
			int	nbits = in.read_uint(4);

			if (has_mult) {
				m_[0][0] = in.read_sint(nbits);
				m_[1][0] = in.read_sint(nbits);
				m_[2][0] = in.read_sint(nbits);
				m_[3][0] = in.read_sint(nbits);
			}
			if (has_add) {
				m_[0][1] = in.read_sint(nbits);
				m_[1][1] = in.read_sint(nbits);
				m_[2][1] = in.read_sint(nbits);
				m_[3][1] = in.read_sint(nbits);
			}
		}
	};


	// struct string;


	enum fill_style_type {
		fill_solid,
		fill_linear_gradient,
		fill_radial_gradient,
		fill_tiled_bitmap,
		fill_clipped_bitmap
	};

	struct fill_style {
		fill_style_type	m_type;
		rgba	m_color;
		matrix	m_gradient_matrix;
		gradient	m_gradient;
		character*	m_bitmap_id;
		matrix	m_bitmap_matrix;

		void	read(stream* in)
		{
		}
	};


	struct shape : public character {
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<edge>	m_edges;
		rect	m_bounds;

		void	read(stream* in)
		{
		}
	};


	void	define_shape_tag_loader(stream* in, int tag_type, movie* m)
	// Register this to load tag type 2. (possibly also 22 and 32?)
	{
		int	shape_id = in.read_uint16(in);
		assert(shape_id != 0);

		shape*	s = new shape;
		s->read(in);

		m->add_character(shape_id, s);
	}


	struct bitmap : public character {
	};

	struct button : public character {
	};

	struct text : public character {
	};

	struct font : public character {
	};

	struct sound : public character {
	};

	struct sprite : public character {
	};


	// Keep a table of loader functions for the different tag types.
	hash<int, loader_function>	s_tag_loaders;

	void	register_tag_loader(int tag_type, loader_function lf)
	// Associate the specified tag type with the given tag loader
	// function.
	{
		assert(s_tag_loaders.get(tag_type, NULL) == false);
		assert(lf != NULL);

		s_tag_loaders.add(tag_type, lf);
	}


	struct movie_impl : public movie
	{
		hash<int, character*>	m_characters;
		array<tag*>	m_playlist;	// movie control events.
		array<character*>	m_display_list;	// active characters, ordered by depth.
		// array<int>	m_frame_start;	// tag indices of frame starts. ?

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_frame_count;

		void	read(SDL_RWops* in)
		// Read a .SWF movie.
		{
			Uint32	header = SDL_ReadLE32(in);
			Uint32	file_length = SDL_ReadLE32(in);

			stream	str(in);

			m_frame_size.read(in);
			m_frame_rate = str.read_uint(16);
			m_frame_count = str.read_uint(16);

			while (current position < file_length) {
				int	tag_type = str.open_tag();
				loader_function	lf = NULL;
				if (s_tag_loaders.get(tag_type, &lf)) {
					// call the tag loader.  The tag loader should add
					// characters or tags to the movie data structure.
					(*lf)(in, tag_type, this);

				} else {
					// no tag loader for this tag type.
				}

				str.close_tag();
			}
		}
	};
};



#endif // SWF_IMPL_H
