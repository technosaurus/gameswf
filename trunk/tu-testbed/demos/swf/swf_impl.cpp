// swf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF parser.


// Useful links:
//
// http://www.openswf.org
//
// http://www-lehre.informatik.uni-osnabrueck.de/~fbstark/diplom/docs/swf/Shapes.htm


#include "swf_impl.h"
#include "engine/image.h"
#include <string.h>	// for memset
#include "zlib.h"


namespace swf
{
	struct movie : public character
	{
		virtual void	add_character(int id, character* ch) {}
		virtual void	add_execute_tag(execute_tag* c) {}
	};


	// RGB --> 24 bit color

	// RGBA
	struct rgba {
		Uint8	m_r, m_g, m_b, m_a;

		rgba() : m_r(255), m_g(255), m_b(255), m_a(255) {}

		void	read(stream* in, int tag_type)
		{
			if (tag_type <= 22)
			{
				read_rgb(in);
			}
			else
			{
				read_rgba(in);
			}
		}

		void	read_rgba(stream* in)
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

		void	print(FILE* out)
		{
			fprintf(out, "rgba: %d %d %d %d\n", m_r, m_g, m_b, m_a);
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
			fprintf(out, "xmin = %g, ymin = %g, xmax = %g, ymax = %g\n",
				TWIPS_TO_PIXELS(m_x_min),
				TWIPS_TO_PIXELS(m_y_min),
				TWIPS_TO_PIXELS(m_x_max),
				TWIPS_TO_PIXELS(m_y_max));
		}
	};


	struct matrix {
		float	m_[2][3];

		matrix() { memset(&m_[0], 0, sizeof(m_)); }

		void	read(stream* in)
		{
			in->align();

			memset(&m_[0], 0, sizeof(m_));

			int	has_scale = in->read_uint(1);
			if (has_scale)
			{
				int	scale_nbits = in->read_uint(5);
				m_[0][0] = in->read_sint(scale_nbits) / 65536.0f;
				m_[1][1] = in->read_sint(scale_nbits) / 65536.0f;
			}
			int	has_rotate = in->read_uint(1);
			if (has_rotate)
			{
				int	rotate_nbits = in->read_uint(5);
				m_[1][0] = in->read_sint(rotate_nbits) / 65536.0f;
				m_[0][1] = in->read_sint(rotate_nbits) / 65536.0f;
			}

			int	translate_nbits = in->read_uint(5);
			if (translate_nbits > 0) {
				m_[0][2] = in->read_sint(translate_nbits) / 65536.0f;
				m_[1][2] = in->read_sint(translate_nbits) / 65536.0f;
			}
		}


		void	print(FILE* out)
		{
			fprintf(out, "| %4.4f %4.4f %4.4f |\n", m_[0][0], m_[0][1], m_[0][2]);
			fprintf(out, "| %4.4f %4.4f %4.4f |\n", m_[1][0], m_[1][1], m_[1][2]);
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
		rgba	m_color;
		matrix	m_gradient_matrix;
		array<gradient_record>	m_gradients;
		Uint16	m_bitmap_id;
		matrix	m_bitmap_matrix;

		fill_style()
			:
			m_type(0)
		{
		}

		void	read(stream* in, int tag_type)
		{
			m_type = in->read_u8();

			IF_DEBUG(printf("fsr type = 0x%X\n", m_type));

			if (m_type == 0x00)
			{
				// 0x00: solid fill
				if (tag_type <= 22) {
					m_color.read_rgb(in);
				} else {
					m_color.read_rgba(in);
				}

				IF_DEBUG(printf("fsr color: ");
					 m_color.print(stdout));
			}
			else if (m_type == 0x10 || m_type == 0x12)
			{
				// 0x10: linear gradient fill
				// 0x12: radial gradient fill

				m_gradient_matrix.read(in);
				
				// GRADIENT
				int	num_gradients = in->read_u8();
				assert(num_gradients >= 1 && num_gradients <= 8);
				m_gradients.resize(num_gradients);
				for (int i = 0; i < num_gradients; i++) {
					m_gradients[i].read(in, tag_type);
				}

				IF_DEBUG(printf("fsr: num_gradients = %d\n", num_gradients));
			}
			else if (m_type == 0x40 || m_type == 0x41)
			{
				// 0x40: tiled bitmap fill
				// 0x41: clipped bitmap fill

				m_bitmap_id = in->read_u16();
				IF_DEBUG(printf("fsr: bitmap_id = %d\n", m_bitmap_id));

				m_bitmap_matrix.read(in);
				IF_DEBUG(m_bitmap_matrix.print(stdout));
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
			in->align();

			int	has_add = in->read_uint(1);
			int	has_mult = in->read_uint(1);
			int	nbits = in->read_uint(4);

			if (has_mult) {
				m_[0][0] = in->read_sint(nbits);
				m_[1][0] = in->read_sint(nbits);
				m_[2][0] = in->read_sint(nbits);
				m_[3][0] = 1;
			}
			else {
				for (int i = 0; i < 4; i++) { m_[i][0] = 1; }
			}
			if (has_add) {
				m_[0][1] = in->read_sint(nbits);
				m_[1][1] = in->read_sint(nbits);
				m_[2][1] = in->read_sint(nbits);
				m_[3][1] = 1;
			}
			else {
				for (int i = 0; i < 4; i++) { m_[i][1] = 0; }
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
			else {
				for (int i = 0; i < 4; i++) { m_[i][0] = 1; }
			}
			if (has_add) {
				m_[0][1] = in->read_sint(nbits);
				m_[1][1] = in->read_sint(nbits);
				m_[2][1] = in->read_sint(nbits);
				m_[3][1] = in->read_sint(nbits);
			}
			else {
				for (int i = 0; i < 4; i++) { m_[i][1] = 0; }
			}
		}
	};


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
		array<execute_tag*>	m_playlist;	// movie control events.
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
					IF_DEBUG(printf("*** no tag loader for type %d\n", tag_type));
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
			register_tag_loader(0, end_loader);
			register_tag_loader(1, show_frame_loader);
			register_tag_loader(2, define_shape_loader);
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
			register_tag_loader(22, define_shape_loader);
			register_tag_loader(26, place_object_2_loader);
			register_tag_loader(28, remove_object_2_loader);
			register_tag_loader(32, define_shape_loader);
			register_tag_loader(36, define_bits_lossless_2_loader);
			register_tag_loader(39, sprite_loader);
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


	struct set_background_color : public execute_tag
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

		m->add_execute_tag(t);
	}


	// Bitmap character
	struct bitmap_character_rgb : public character
	{
		image::rgb*	m_image;

		bitmap_character_rgb() : m_image(0) {}

		void	execute() { /* do we display here? */ }
	};

	struct bitmap_character_rgba : public character
	{
		image::rgba*	m_image;

		bitmap_character_rgba() : m_image(0) {}

		void	execute() { /* do some display... */ }
	};


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 21);
		
		Uint16	character_id = in->read_u16();

		in->align();
		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->m_image = image::read_swf_jpeg2(in->m_input);

		IF_DEBUG(printf("id = %d, bm = 0x%X, width = %d, height = %d, pitch = %d\n",
				character_id,
				(unsigned) ch->m_image,
				ch->m_image->m_width,
				ch->m_image->m_height,
				ch->m_image->m_pitch));

		// add image to movie, under character id.
		m->add_character(character_id, ch);
	}


	void	inflate_wrapper(SDL_RWops* in, void* buffer, int buffer_bytes)
	// Wrapper function -- uses Zlib to uncompress in_bytes worth
	// of data from the input file into buffer_bytes worth of data
	// into *buffer.
	{
		assert(in);
		assert(buffer);
		assert(buffer_bytes > 0);

		int err;
		z_stream d_stream; /* decompression stream */

		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;

		d_stream.next_in  = 0;
		d_stream.avail_in = 0;

		d_stream.next_out = (Byte*) buffer;
		d_stream.avail_out = (uInt) buffer_bytes;

		err = inflateInit(&d_stream);
		if (err != Z_OK) {
			fprintf(stderr, "error: inflate_wrapper() inflateInit() returned %d\n", err);
			return;
		}

		Uint8	buf[1];

		for (;;) {
			// Fill a one-byte (!) buffer.
			buf[0] = ReadByte(in);
			d_stream.next_in = &buf[0];
			d_stream.avail_in = 1;

			err = inflate(&d_stream, Z_NO_FLUSH);
			if (err == Z_STREAM_END) break;
			if (err != Z_OK)
			{
				fprintf(stderr, "error: inflate_wrapper() inflate() returned %d\n", err);
			}
		}

		err = inflateEnd(&d_stream);
		if (err != Z_OK)
		{
			fprintf(stderr, "error: inflate_wrapper() inflateEnd() return %d\n", err);
		}
	}


	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 36);

		Uint16	character_id = in->read_u16();
		Uint8	bitmap_format = in->read_u8();	// 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
		Uint16	width = in->read_u16();
		Uint16	height = in->read_u16();

		IF_DEBUG(printf("dbl2l: id = %d, fmt = %d, w = %d, h = %d\n", character_id, bitmap_format, width, height));

		bitmap_character_rgba*	ch = new bitmap_character_rgba();
		ch->m_image = image::create_rgba(width, height);

		if (bitmap_format == 3)
		{
			// 8-bit data, preceded by a palette.

			const int	bytes_per_pixel = 1;
			int	color_table_size = in->read_u8();
			color_table_size += 1;	// !! SWF stores one less than the actual size

			int	pitch = (width * bytes_per_pixel + 3) & ~3;

			int	buffer_bytes = color_table_size * 4 + pitch * height;
			Uint8*	buffer = new Uint8[buffer_bytes];

			inflate_wrapper(in->m_input, buffer, buffer_bytes);
			assert(in->get_tag_end_position() == in->get_position());

			Uint8*	color_table = buffer;

			for (int j = 0; j < height; j++)
			{
				Uint8*	image_in_row = buffer + color_table_size * 4 + j * pitch;
				Uint8*	image_out_row = image::scanline(ch->m_image, j);
				for (int i = 0; i < width; i++)
				{
					Uint8	pixel = image_in_row[i * bytes_per_pixel];
					image_out_row[i * 4 + 0] = color_table[pixel * 4 + 0];
					image_out_row[i * 4 + 1] = color_table[pixel * 4 + 1];
					image_out_row[i * 4 + 2] = color_table[pixel * 4 + 2];
					image_out_row[i * 4 + 3] = color_table[pixel * 4 + 3];
				}
			}

			delete [] buffer;
		}
		else if (bitmap_format == 4)
		{
			// 16 bits / pixel
			const int	bytes_per_pixel = 2;
			int	pitch = (width * bytes_per_pixel + 3) & ~3;

			int	buffer_bytes = pitch * height;
			Uint8*	buffer = new Uint8[buffer_bytes];

			inflate_wrapper(in->m_input, buffer, buffer_bytes);
			assert(in->get_tag_end_position() == in->get_position());
			
			for (int j = 0; j < height; j++)
			{
				Uint8*	image_in_row = buffer + j * pitch;
				Uint8*	image_out_row = image::scanline(ch->m_image, j);
				for (int i = 0; i < width; i++)
				{
					Uint16	pixel = image_in_row[i * 2] | (image_in_row[i * 2 + 1] << 8);
					
					// @@ How is the data packed???  I'm just guessing here that it's 565!
					image_out_row[i * 4 + 0] = 255;			// alpha
					image_out_row[i * 4 + 1] = (pixel >> 8) & 0xF8;	// red
					image_out_row[i * 4 + 2] = (pixel >> 3) & 0xFC;	// green
					image_out_row[i * 4 + 3] = (pixel << 3) & 0xF8;	// blue
				}
			}
			
			delete [] buffer;
		}
		else if (bitmap_format == 5)
		{
			// 32 bits / pixel, input is ARGB format

			inflate_wrapper(in->m_input, ch->m_image->m_data, width * height * 4);
			assert(in->get_tag_end_position() == in->get_position());
			
			// Need to re-arrange ARGB into RGBA.
			for (int j = 0; j < height; j++)
			{
				Uint8*	image_row = image::scanline(ch->m_image, j);
				for (int i = 0; i < width; i++)
				{
					Uint8	a = image_row[i * 4 + 0];
					Uint8	r = image_row[i * 4 + 1];
					Uint8	g = image_row[i * 4 + 2];
					Uint8	b = image_row[i * 4 + 3];
					image_row[i * 4 + 0] = r;
					image_row[i * 4 + 1] = g;
					image_row[i * 4 + 2] = b;
					image_row[i * 4 + 3] = a;
				}
			}
		}

		// add image to movie, under character id.
		m->add_character(character_id, ch);
	}


	// Together with the previous anchor, defines a quadratic
	// curve segment.
	struct edge
	{
		float	m_cx, m_cy;	// *quadratic* bezier: point = ... * previous_a + ... * c + ... * a
		float	m_ax, m_ay;
		int	m_fill0, m_fill1, m_line;

		edge()
			:
			m_cx(0), m_cy(0),
			m_ax(0), m_ay(0),
			m_fill0(-1), m_fill1(-1), m_line(-1)
		{}

		edge(float cx, float cy, float ax, float ay,
		     int fill0, int fill1, int line)
			:
			m_cx(cx), m_cy(cy),
			m_ax(ax), m_ay(ay),
			m_fill0(fill0),
			m_fill1(fill1),
			m_line(line)
		{
		}
	};


	static void	read_fill_styles(array<fill_style>* styles, stream* in, int tag_type)
	// Read fill styles, and push them onto the given style array.
	{
		assert(styles);

		// Get the count.
		int	fill_style_count = in->read_u8();
		if (tag_type > 2)
		{
			if (fill_style_count == 0xFF)
			{
				fill_style_count = in->read_u16();
			}
		}

		IF_DEBUG(printf("rfs: fsc = %d\n", fill_style_count));

		// Read the styles.
		for (int i = 0; i < fill_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type);
		}
	}


	void	read_line_styles(array<line_style>* styles, stream* in, int tag_type)
	// Read line styles and push them onto the back of the given array.
	{
		// Get the count.
		int	line_style_count = in->read_u8();

		IF_DEBUG(printf("rls: lsc = %d\n", line_style_count));

		// @@ does the 0xFF flag apply to all tag types?
		// if (tag_type > 2)
		// {
			if (line_style_count == 0xFF)
			{
				line_style_count = in->read_u16();
			}
		// }

		IF_DEBUG(printf("rls: lsc2 = %d\n", line_style_count));

		// Read the styles.
		for (int i = 0; i < line_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type);
		}
	}


	struct shape_character : public character
	{
		rect	m_bound;
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<edge>	m_edges;

		virtual ~shape_character() {}

		void	read(stream* in, int tag_type)
		{
			m_bound.read(in);
			
			read_fill_styles(&m_fill_styles, in, tag_type);
			read_line_styles(&m_line_styles, in, tag_type);

			//
			// SHAPE
			//
			int	num_fill_bits = in->read_uint(4);
			int	num_line_bits = in->read_uint(4);

			IF_DEBUG(printf("scr: nfb = %d, nlb = %d\n", num_fill_bits, num_line_bits));

			int	fill_base = 0;
			int	line_base = 0;
			float	x = 0, y = 0;
			int	fill_0 = 0, fill_1 = 0, line = 0;

			// SHAPERECORDS
			for (;;) {
				int	type_flag = in->read_uint(1);
				if (type_flag == 0)
				{
					int	flags = in->read_uint(5);
					if (flags == 0) {
						// End of shape records.
						break;
					}
					if (flags & 0x01)
					{
						// move_to = 1;
						int	num_move_bits = in->read_uint(5);
						int	move_x = in->read_sint(num_move_bits);
						int	move_y = in->read_sint(num_move_bits);

						x += move_x;
						y += move_y;

						// add *blank* edge(x, y, -1, -1, -1, -1);
					}
					if (flags & 0x02) {
						// fill_style_0_change = 1;
						fill_0 = in->read_uint(num_fill_bits);
					}
					if (flags & 0x04) {
						// fill_style_1_change = 1;
						fill_1 = in->read_uint(num_fill_bits);
					}
					if (flags & 0x08) {
						// line_style_change = 1;
						line = in->read_uint(num_line_bits);
					}
					if (flags & 0x10) {
						// assert(tag_type >= 22); new_styles = 1;
						fill_base = m_fill_styles.size();
						line_base = m_line_styles.size();
						read_fill_styles(&m_fill_styles, in, tag_type);
						read_line_styles(&m_line_styles, in, tag_type);
						num_fill_bits = in->read_uint(4);
						num_line_bits = in->read_uint(4);
					}
				}
				else
				{
					// EDGERECORD
					int	edge_flag = in->read_uint(1);
					if (edge_flag == 0)
					{
						// curved edge
						int num_bits = 2 + in->read_uint(4);
						float	cx = x + in->read_sint(num_bits);
						float	cy = y + in->read_sint(num_bits);
						float	ax = cx + in->read_sint(num_bits);
						float	ay = cy + in->read_sint(num_bits);

						x = ax;
						y = ay;

						m_edges.push_back(edge(cx, cy, ax, ay,
								       fill_base + fill_0 - 1, fill_base + fill_1,
								       line_base + line));
					}
					else
					{
						// straight edge
						int	num_bits = 2 + in->read_uint(4);
						int	line_flag = in->read_uint(1);
						float	dx = 0, dy = 0;
						if (line_flag)
						{
							// general line
							dx = in->read_sint(num_bits);
							dy = in->read_sint(num_bits);
						}
						else
						{
							int	vert_flag = in->read_uint(1);
							if (vert_flag == 0) {
								dx = in->read_sint(num_bits);
							} else {
								dy = in->read_sint(num_bits);
							}
						}

						m_edges.push_back(edge(x + dx/2, y + dy/2, x + dx, y + dy,
								       fill_base + fill_0 - 1, fill_base + fill_1 - 1,
								       line_base + line - 1));
					}
				}
			}
		}
	};

	
	void	define_shape_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 2
		       || tag_type == 22
		       || tag_type == 32);

		Uint16	character_id = in->read_u16();

		shape_character*	ch = new shape_character;
		ch->read(in, tag_type);

		IF_DEBUG(printf("shape_loader: id = %d, rect ", character_id);
			 ch->m_bound.print(stdout));
		IF_DEBUG(printf("shape_loader: fill style ct = %d, line style ct = %d, edge ct = %d\n",
				ch->m_fill_styles.size(), ch->m_line_styles.size(), ch->m_edges.size()));

		m->add_character(character_id, ch);
	}

	
	struct place_object_2 : public execute_tag
	{
		char*	m_name;
		float	m_ratio;
		cxform	m_color_transform;
		matrix	m_matrix;
		Uint16	m_depth;
		Uint16	m_character_id;
		enum place_type {
			PLACE,
			MOVE,
			REPLACE,
		} m_place_type;


		place_object_2()
			:
			m_name(NULL),
			m_ratio(0),
			m_depth(0),
			m_character_id(0),
			m_place_type(PLACE)
		{
		}

		void	read(stream* in)
		{
			in->align();

			in->read_uint(1);	// reserved flag
			bool	has_clip_bracket = in->read_uint(1);
			bool	has_name = in->read_uint(1);
			bool	has_ratio = in->read_uint(1);
			bool	has_cxform = in->read_uint(1);
			bool	has_matrix = in->read_uint(1);
			bool	has_char = in->read_uint(1);
			bool	flag_move = in->read_uint(1);

			m_depth = in->read_u16();

			if (has_char) {
				m_character_id = in->read_u16();
			}
			if (has_matrix) {
				m_matrix.read(in);
			}
			if (has_cxform) {
				m_color_transform.read_rgba(in);
			}
			if (has_ratio) {
				m_ratio = in->read_u16();
			}
			if (has_clip_bracket) {
				int	clip_depth = in->read_u16();
				UNUSED(clip_depth);
				IF_DEBUG(printf("HAS CLIP BRACKET!\n"));
			}
			if (has_name) {
				m_name = in->read_string();
			}

			if (has_char == true && flag_move == true)
			{
				// Remove whatever's at m_depth, and put m_character there.
				m_place_type = REPLACE;
			}
			else if (has_char == false && flag_move == true)
			{
				// Moves the object at m_depth to the new location.
				m_place_type = MOVE;
			}
			else if (has_char == true && flag_move == false)
			{
				// Put m_character at m_depth.
				m_place_type = PLACE;
			}

			IF_DEBUG(printf("po2r: name = %s\n", m_name ? m_name : "<null>");
				 printf("po2r: char id = %d, mat:\n", m_character_id);
				 m_matrix.print(stdout);
				);
		}
	};

	
	void	place_object_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 26);

		place_object_2*	ch = new place_object_2;
		ch->read(in);

		m->add_execute_tag(ch);
	}


	//
	// sprite
	//


	// A sprite is a mini movie-within-a-movie.  It doesn't define
	// its own characters; it uses the characters from the parent
	// movie, but it has its own frame counter, display list, etc.
	struct sprite : public movie
	{
		movie_impl*	m_movie;		// parent movie.
		array<execute_tag*>	m_playlist;	// movie control events.
		array<character*>	m_display_list;	// active characters, ordered by depth.

		// array<int>	m_frame_start;	// tag indices of frame starts. ?

		int	m_frame_count;

		sprite();
		~sprite();

		sprite(movie_impl* movie)
			:
			m_movie(movie),
			m_frame_count(0)
		{
			assert(m_movie);
		}

		void	execute(float time)
		{
			/* do stuff here! */
		}

		void	add_character(int id, character* ch)
		{
			fprintf(stderr, "error: sprite::add_character() is illegal\n");
		}

		void	add_execute_tag(execute_tag* c)
		{
			m_playlist.push_back(c);
		}


		void	read(stream* in)
		// Read the sprite info.  Consists of a series of tags.
		{
			int	tag_end = in->get_tag_end_position();

			m_frame_count = in->read_u16();

			IF_DEBUG(printf("sprite: frames = %d\n", m_frame_count));

			while ((Uint32) in->get_position() < (Uint32) tag_end)
			{
				int	tag_type = in->open_tag();
				loader_function	lf = NULL;
				if (s_tag_loaders.get(tag_type, &lf))
				{
//  					if (tag_type != show frame
//  					    && tag_type != place object
//  					    && tag_type != place object 2
//  					    && tag_type != remove object
//  					    && tag_type != remove object 2
//  					    && tag_type != do action
//  					    && tag_type != start sound
//  					    && tag_type != frame label
//  					    && tag_type != sound stream head
//  					    && tag_type != sound stream block
//  					    && tag_type != end)
//  					{
//  						// error, invalid tag for a sprite.
//  					}

					// call the tag loader.  The tag loader should add
					// characters or tags to the movie data structure.
					(*lf)(in, tag_type, this);
				}
				else
				{
					// no tag loader for this tag type.
					IF_DEBUG(printf("*** no tag loader for type %d\n", tag_type));
				}

				in->close_tag();
			}
		}
	};

	sprite::sprite() : m_movie(0), m_frame_count(0) {}
	sprite::~sprite() {}

	
	void	sprite_loader(stream* in, int tag_type, movie* m)
	// Create and initialize a sprite, and add it to the movie.
	{
		assert(tag_type == 39);

		int	character_id = in->read_u16();

		movie_impl*	mi = dynamic_cast<movie_impl*>(m);
		assert(mi);

//		sprite*	ch = new sprite(mi);
		sprite*	ch = new sprite;
		ch->m_movie = mi;
		ch->read(in);

		m->add_character(character_id, ch);
	}


	//
	// show_frame
	//

	
	struct show_frame : public execute_tag
	{
	};

	void	show_frame_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 1);
		assert(in->get_position() == in->get_tag_end_position());

		m->add_execute_tag(new show_frame);
	}


	//
	// end_tag
	//

	// end_tag doesn't actually need to exist.

	void	end_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 0);
		assert(in->get_position() == in->get_tag_end_position());
	}


	//
	// remove_object_2
	//

	
	struct remove_object_2 : public execute_tag
	{
		int	m_depth;

		remove_object_2() : m_depth(-1) {}

		void	read(stream* in)
		{
			m_depth = in->read_u16();
		}

		void	execute(float time) { /* do something here! */ }
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 28);

		remove_object_2*	t = new remove_object_2;
		t->read(in);

		m->add_execute_tag(t);
	}
};

