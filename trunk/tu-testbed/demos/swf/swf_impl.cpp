// swf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF parser.


#include "swf_impl.h"
#include "engine/image.h"


namespace swf
{
	struct control_tag;

	struct movie
	{
		virtual void	execute(float time) {}
		virtual void	add_character(int id, character* ch) {}
		virtual void	add_control_tag(control_tag* c) {}
	};


	// RGB --> 24 bit color

	// RGBA
	struct rgba {
		Uint8	m_r, m_g, m_b, m_a;

		void	read(stream* in, int tag_type)
		{
			if (tag_type <= 22)
			{
				read_rgb(in);
			}
			else
			{
				read(in);
			}
		}

		void	read(stream* in)
		{
			read_rgb(in);
			m_a = in->read_u8();
		}

		void	read_rgb(stream* in)
		{
			m_r = in->read_u8();
			m_g = in->read_u8();
			m_b = in->read_u8();
			m_a = 0x0FF;
		}
	};


	struct rect {
		float	m_x_min, m_x_max, m_y_min, m_y_max;

		void	read(stream* in)
		{
			int	nbits = in->read_uint(5);
			m_x_min = in->read_sint(nbits);
			m_x_max = in->read_sint(nbits);
			m_y_min = in->read_sint(nbits);
			m_y_max = in->read_sint(nbits);

//			IF_DEBUG(printf("rect::read() nbits = %d\n", nbits));
//			IF_DEBUG(print(stdout));
		}

		void	print(FILE* out)
		// Debug spew.
		{
			printf("xmin = %g, ymin = %g, xmax = %g, ymax = %g\n",
			       TWIPS_TO_PIXELS(m_x_min),
			       TWIPS_TO_PIXELS(m_y_min),
			       TWIPS_TO_PIXELS(m_x_max),
			       TWIPS_TO_PIXELS(m_y_max));
		}
	};


	struct matrix {
		float	m_[2][3];

		void	read(stream* in)
		{
			int	has_scale = in->read_uint(1);
			int	scale_nbits = in->read_uint(5);
//			int	scale_x = ;
//			int	scale_y = ;

			int	has_rotate = in->read_uint(1);
			int	rotate_nbits = in->read_uint(5);

			int	rs0;
			int	rs1;

			int	translate_nbits = in->read_uint(5);
			int	translate_x = in->read_sint(translate_nbits);
			int	translate_y = in->read_sint(translate_nbits);
		}
	};


	struct gradient_record
	{
		Uint8	m_ratio;
		rgba	m_color;

		gradient_record() : m_ratio(0) {}

		void	read(stream* in, int tag_type)
		{
			m_ratio = in->read_u8();
			m_color.read(in, tag_type);
		}
	};

	struct fill_style
	{
		int	m_type;
		union {
			rgba	m_color;
			struct {
				matrix	m_gradient_matrix;
				array<gradient_records>	m_gradients;
			};
			struct {
				Uint16	m_bitmap_id;
				matrix	m_bitmap_matrix;
			};
		};

		fill_style()
			:
			m_type(0),
			m_bitmap_id(0)
		{
		}

		void	read(stream* in, int tag_type)
		{
			m_type = in->read_u8();
			if (m_type == 0x00)
			{
				// 0x00: solid fill
				if (tag_type <= 22) {
					m_color.read_rgb(in);
				} else {
					m_color.read_rgba(in);
				}
			}
			else if (m_type == 0x10 || m_type == 0x12)
			{
				// 0x10: linear gradient fill
				// 0x12: radial gradient fill

				m_gradient_matrix.read(in);
				
				// GRADIENT
				int	num_gradients = in->read_u8();
				assert(num_gradients >= 1 && num_gradients <= 8);
				m_gradients->resize(num_gradients);
				for (int i = 0; i < num_gradients; i++) {
					m_gradients.read(in, tag_type);
				}
			}
			else if (m_type == 0x40 || m_type == 0x41)
			{
				// 0x40: tiled bitmap fill
				// 0x41: clipped bitmap fill

				m_bitmap_id = in->read_u16();
				m_bitmap_matrix.read(in);
			}
		}
	};


	struct line_style
	{
		Uint16	m_width;	// in TWIPS
		rgba	m_color;

		line_style() : m_width(0) {}

		void	read(stream* in, int tag_type)
		{
			m_width = in->read_u16();
			m_color.read(in, tag_type);
		}
	};


	struct cxform {
		float	m_[4][2];	// [RGBA][mult, add]

		void	read_rgb(stream* in)
		{
			int	has_add = in->read_uint(1);
			int	has_mult = in->read_uint(1);
			int	nbits = in->read_uint(4);

			if (has_mult) {
				m_[0][0] = in->read_sint(nbits);
				m_[1][0] = in->read_sint(nbits);
				m_[2][0] = in->read_sint(nbits);
				m_[3][0] = 1;
			}
			if (has_add) {
				m_[0][1] = in->read_sint(nbits);
				m_[1][1] = in->read_sint(nbits);
				m_[2][1] = in->read_sint(nbits);
				m_[3][1] = 1;
			}
		}

		void	read_rgba(stream* in)
		{
			int	has_add = in->read_uint(1);
			int	has_mult = in->read_uint(1);
			int	nbits = in->read_uint(4);

			if (has_mult) {
				m_[0][0] = in->read_sint(nbits);
				m_[1][0] = in->read_sint(nbits);
				m_[2][0] = in->read_sint(nbits);
				m_[3][0] = in->read_sint(nbits);
			}
			if (has_add) {
				m_[0][1] = in->read_sint(nbits);
				m_[1][1] = in->read_sint(nbits);
				m_[2][1] = in->read_sint(nbits);
				m_[3][1] = in->read_sint(nbits);
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

	struct gradient {};

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


	struct line_style {};
	struct edge {};

	struct shape : public character
	{
		virtual ~shape() {}

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
		int	shape_id = in->read_u16();
		assert(shape_id != 0);

		shape*	s = new shape;
		s->read(in);

		assert(m);
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
	static hash<int, loader_function>	s_tag_loaders;

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
		int	m_version;

		movie_impl()
			:
			m_frame_rate(30),
			m_frame_count(0),
			m_version(0)
		{
		}

		virtual ~movie_impl() {}

		void	read(SDL_RWops* in)
		// Read a .SWF movie.
		{
			Uint32	header = SDL_ReadLE32(in);
			Uint32	file_length = SDL_ReadLE32(in);

			int	m_version = (header >> 24) & 255;
			if ((header & 0x0FFFFFF) != 0x00535746)
			{
				// ERROR
				IF_DEBUG(printf("swf::movie_impl::read() -- file does not start with a SWF header!\n"));
				return;
			}

			IF_DEBUG(printf("version = %d, file_length = %d\n", m_version, file_length));

			stream	str(in);

			m_frame_size.read(&str);
			m_frame_rate = str.read_u16() / 256.0f;
			m_frame_count = str.read_u16();

			IF_DEBUG(m_frame_size.print(stdout));
			IF_DEBUG(printf("frame rate = %f, frames = %d\n", m_frame_rate, m_frame_count));

			while ((Uint32) str.get_position() < file_length) {
				int	tag_type = str.open_tag();
				loader_function	lf = NULL;
				if (s_tag_loaders.get(tag_type, &lf)) {
					// call the tag loader.  The tag loader should add
					// characters or tags to the movie data structure.
					(*lf)(&str, tag_type, this);

				} else {
					// no tag loader for this tag type.
					IF_DEBUG(printf("no tag loader for type %d\n", tag_type));
				}

				str.close_tag();
			}
		}
	};

	
	static void	ensure_loaders_registered()
	{
		static bool	s_registered = false;
		
		if (s_registered == false)
		{
			// Register the standard loaders.
			s_registered = true;
			register_tag_loader(2, define_shape_loader);
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
		}
	}


	movie*	create_movie(SDL_RWops* in)
	// External API.  Create a movie from the given stream, and
	// return it.
	{
		ensure_loaders_registered();

		movie_impl*	m = new movie_impl;
		m->read(in);

		return m;
	}



	//
	// Some tag implementations
	//

	struct set_background_color : public control_tag
	{
		rgba	m_color;

		void	execute() {}

		void	read(stream* in)
		{
			m_color.read_rgb(in);
		}
	};


	void	set_background_color_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 9);
		assert(m);

		set_background_color*	t = new set_background_color;
		t->read(in);

		m->add_control_tag(t);
	}


	// Bitmap character
	struct bitmap_character : public character
	{
		image::rgb*	m_image;

		bitmap_character(image::rgb* im)
			:
			m_image(im)
		{
		}

		void	execute() { /* do we display here? */ }
	};


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 21);
		
		Uint16	character_id = in->read_u16();

		in->align();
		bitmap_character*	ch = new bitmap_character(image::read_swf_jpeg2(in->m_input));

		IF_DEBUG(printf("id = %d, bm = 0x%X, width = %d, height = %d, pitch = %d\n",
				character_id,
				(unsigned) ch->m_image,
				ch->m_image->m_width,
				ch->m_image->m_height,
				ch->m_image->m_pitch));

		// add image to movie, under character id.
		m->add_character(character_id, ch);
	}


	struct shape_character : public character
	{
		rect	m_bound;
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;

		// more shape info...
	};

	
	void	define_shape_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 2);

		Uint16	character_id = in->read_u16();

		shape_character*	ch = new shape_character;
		ch->m_bound.read(in);

		// @@ read shape with style

		//
		// FILLSTYLEARRAY
		//
		{
			// Get the count.
			int	fill_style_count = in->read_u8();
			if (tag_type > 2)
			{
				if (fill_style_count == 0xFF)
				{
					fill_style_count = in->read_u16();
				}
			}
			m_fill_styles.resize(fill_style_count);

			// Read the styles.
			for (int i = 0; i < fill_style_count; i++)
			{
				m_fill_styles[i].read(in, tag_type);
			}
		}

		//
		// LINESTYLEARRAY
		//
		{
			// Get the count.
			int	line_style_count = in->read_u8();

			// @@ does the 0xFF flag apply to all tag types?
			// if (tag_type > 2)
			// {

				if (fill_style_count == 0xFF)
				{
					fill_style_count = in->read_u16();
				}

			// }

			m_line_styles.resize(line_style_count);
			// Read the styles.
			for (int i = 0; i < line_style_count; i++)
			{
				m_line_styles[i].read(in, tag_type);
			}
		}

		int	num_fill_bits = in->read_uint(4);
		int	num_line_bits = in->read_uint(4);

		// SHAPERECORDS


		IF_DEBUG(printf("shape_loader: id = %d, rect ", character_id);
			 ch->m_bound.print(stdout));

		m->add_character(character_id, ch);
	}
};
