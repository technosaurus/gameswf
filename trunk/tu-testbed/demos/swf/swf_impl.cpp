// swf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF parser.


// Useful links:
//
// http://www.openswf.org


#include "engine/ogl.h"
#include "swf_impl.h"
#include "swf_render.h"
#include "swf_stream.h"
#include "engine/image.h"
#include <string.h>	// for memset
#include <zlib.h>
#include <typeinfo>


namespace swf
{
	// Information about how to display an element.
	struct display_info
	{
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			m_depth = di.m_depth;
			m_color_transform.concatenate(di.m_color_transform);
			m_matrix.concatenate(di.m_matrix);
			m_ratio = di.m_ratio;
		}
	};


	struct rect
	{
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

		void	debug_display(const display_info& di)
		// Show the rectangle.
		{
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();

			di.m_matrix.ogl_multiply();

			glColor3f(1, 1, 0);
			glBegin(GL_LINE_STRIP);
			{
				glVertex2f(m_x_min, m_y_min);
				glVertex2f(m_x_min, m_y_max);
				glVertex2f(m_x_max, m_y_max);
				glVertex2f(m_x_max, m_y_min);
				glVertex2f(m_x_min, m_y_min);
			}
			glEnd();

			glPopMatrix();
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
		bitmap_character*	m_bitmap_character;
		matrix	m_bitmap_matrix;

		fill_style()
			:
			m_type(0),
			m_bitmap_character(0)
		{
			assert(m_gradients.size() == 0);
		}

		void	read(stream* in, int tag_type, movie* m)
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
				for (int i = 0; i < num_gradients; i++)
				{
					m_gradients[i].read(in, tag_type);
				}

				IF_DEBUG(printf("fsr: num_gradients = %d\n", num_gradients));

				// @@ hack.
				if (num_gradients > 0)
				{
					m_color = m_gradients[0].m_color;
				}
			}
			else if (m_type == 0x40 || m_type == 0x41)
			{
				// 0x40: tiled bitmap fill
				// 0x41: clipped bitmap fill

				int	bitmap_char_id = in->read_u16();
				IF_DEBUG(printf("fsr: bitmap_char = %d\n", bitmap_char_id));

				// Look up the bitmap character.
				m_bitmap_character = m->get_bitmap_character(bitmap_char_id);

				m_bitmap_matrix.read(in);
				IF_DEBUG(m_bitmap_matrix.print(stdout));
			}
		}

		void	apply(int fill_side) const
		// Push our style parameters into the renderer.
		{
			if (m_type == 0x00)
			{
				// 0x00: solid fill
				swf::render::fill_style_color(fill_side, m_color);
			}
			else if (m_type == 0x10 || m_type == 0x12)
			{
				// 0x10: linear gradient fill
				// 0x12: radial gradient fill

				// Hack.
				swf::render::fill_style_color(fill_side, m_color);
			}
			else if (m_type == 0x40
				 || m_type == 0x41)
			{
				// bitmap fill (either tiled or clipped)
				swf::render::bitmap_info*	bi = NULL;
				if (m_bitmap_character != NULL)
				{
					bi = m_bitmap_character->get_bitmap_info();
					if (bi != NULL)
					{
						swf::render::bitmap_wrap_mode	wmode = swf::render::WRAP_REPEAT;
						if (m_type == 0x41)
						{
							wmode = swf::render::WRAP_CLAMP;
						}
						swf::render::fill_style_bitmap(fill_side, bi, m_bitmap_matrix, wmode);
					}
				}
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

		void	apply() const
		{
			swf::render::line_style_color(m_color);
		}
	};


	struct bitmap : public character {
	};

	struct button : public character {
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


	// A struct to serve as an entry in the display list.
	struct display_object_info : display_info
	{
		character*	m_character;

		static int compare(const void* _a, const void* _b)
		// For qsort().
		{
			display_object_info*	a = (display_object_info*) _a;
			display_object_info*	b = (display_object_info*) _b;

			if (a->m_depth < b->m_depth)
			{
				return -1;
			}
			else if (a->m_depth == b->m_depth)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	};


	//
	// some display list management functions
	//


	static int	find_display_index(const array<display_object_info>& display_list, int depth)
	// Find the index in the display list matching the given
	// depth.  Failing that, return the index of the first object
	// with a larger depth.
	{
		int	size = display_list.size();
		if (size == 0)
		{
			return 0;
		}

		// Binary search.
		int	jump = size >> 1;
		int	index = jump;
		for (;;)
		{
			jump >>= 1;
			if (jump < 1) jump = 1;

			if (depth > display_list[index].m_depth) {
				if (index == size - 1)
				{
					index = size;
					break;
				}
				index += jump;
			}
			else if (depth < display_list[index].m_depth)
			{
				if (index == 0
				    || depth > display_list[index - 1].m_depth)
				{
					break;
				}
				index -= jump;
			}
			else
			{
				// match -- return this index.
				break;
			}
		}

		assert(index >= 0 && index <= size);

		return index;
	}



	static void	add_display_object(array<display_object_info>* display_list,
					   character* ch,
					   Uint16 depth,
					   const cxform& color_transform,
					   const matrix& matrix,
					   float ratio)
	{
		assert(ch);

		// If the character needs per-instance state, then
		// create the instance here, and substitute it for the
		// definition.
		if (ch->is_definition())
		{
			ch = ch->create_instance();
			ch->restart();
		}

		display_object_info	di;
		di.m_character = ch;
		di.m_depth = depth;
		di.m_color_transform = color_transform;
		di.m_matrix = matrix;
		di.m_ratio = ratio;

		// Insert into the display list...

		int	index = find_display_index(*display_list, di.m_depth);

		// Shift existing objects up.
		display_list->resize(display_list->size() + 1);
		if (display_list->size() - 1 > index)
		{
			memmove(&(*display_list)[index + 1],
				&(*display_list)[index],
				sizeof((*display_list)[0]) * (display_list->size() - 1 - index));
		}

		// Put new info in the opened slot.
		(*display_list)[index] = di;
	}


	static void	move_display_object(array<display_object_info>* display_list,
					    Uint16 depth,
					    bool use_cxform,
					    const cxform& color_xform,
					    bool use_matrix,
					    const matrix& mat,
					    float ratio)
	// Updates the transform properties of the object at
	// the specified depth.
	{
		int	size = display_list->size();
		if (size <= 0)
		{
			// error.
			assert(0);
			return;
		}

		int	index = find_display_index(*display_list, depth);
		if (index < 0 || index >= size)
		{
			// error.
			assert(0);
			return;
		}

		display_object_info&	di = (*display_list)[index];
		if (di.m_depth != depth)
		{
			// error
			IF_DEBUG(printf("error: move_display_object() -- no object at depth %d\n", depth));
//			assert(0);
			return;
		}

		if (use_cxform)
		{
			di.m_color_transform = color_xform;
		}
		if (use_matrix)
		{
			di.m_matrix = mat;
		}
		di.m_ratio = ratio;
	}
		

	static void	replace_display_object(array<display_object_info>* display_list,
					       character* ch,
					       Uint16 depth,
					       bool use_cxform,
					       const cxform& color_xform,
					       bool use_matrix,
					       const matrix& mat,
					       float ratio)
	// Puts a new character at the specified depth, replacing any
	// existing character.  If use_cxform or use_matrix are false,
	// then keep those respective properties from the existing
	// character.
	{
		int	size = display_list->size();
		if (size <= 0)
		{
			// error.
			assert(0);
			return;
		}

		int	index = find_display_index(*display_list, depth);
		if (index < 0 || index >= size)
		{
			// error.
			assert(0);
			return;
		}

		display_object_info&	di = (*display_list)[index];
		if (di.m_depth != depth)
		{
			// error
			IF_DEBUG(printf("error: replace_display_object() -- no object at depth %d\n", depth));
//			assert(0);
			return;
		}

		// If the old character is an instance, then delete it.
		if (di.m_character->is_instance())
		{
			delete di.m_character;
			di.m_character = 0;
		}

		// Put the new character in its place.
		assert(ch);

		// If the character needs per-instance state, then
		// create the instance here, and substitute it for the
		// definition.
		if (ch->is_definition())
		{
			ch = ch->create_instance();
			ch->restart();
		}

		// Set the display properties.
		di.m_character = ch;
		if (use_cxform)
		{
			di.m_color_transform = color_xform;
		}
		if (use_matrix)
		{
			di.m_matrix = mat;
		}
		di.m_ratio = ratio;
	}
		

	static void remove_display_object(array<display_object_info>* display_list, Uint16 depth)
	// Removes the object at the specified depth.
	{
		int	size = display_list->size();
		if (size <= 0)
		{
			// error.
			assert(0);
			return;
		}

		int	index = find_display_index(*display_list, depth);
		if (index < 0 || index >= size)
		{
			// error.
			assert(0);
			return;
		}

		// Removing the character at (*display_list)[index].
		display_object_info&	di = (*display_list)[index];

		// If the character is an instance, then delete it.
		if (di.m_character->is_instance())
		{
			delete di.m_character;
			di.m_character = 0;
		}

		// Remove the display list entry.
		if (index < size - 1)
		{
			memmove(&(*display_list)[index],
				&(*display_list)[index + 1],
				sizeof((*display_list)[0]) * (size - 1 - index));
		}
		display_list->resize(size - 1);
	}


	// Base class for actions.
	struct action_buffer
	{
		array<unsigned char>	m_buffer;

		void	read(stream* in);
		void	execute(movie* m);
	};



	static bool	execute_actions(movie* m, const array<action_buffer*>& action_list)
	// Execute the actions in the action list, on the given movie.
	// Return true if the actions did something to change the
	// current_frame of the movie.
	{
		{for (int i = 0; i < action_list.size(); i++)
		{
			int	local_current_frame = m->get_current_frame();

			action_list[i]->execute(m);

			// @@ the action could possibly have changed
			// the size of m_action_list... do we have to
			// do anything special to make sure
			// m_action_list.m_size is re-read from RAM,
			// and not cached in a register???  Declare it
			// volatile, or something like that?

			// Frame state could have changed!
			if (m->get_current_frame() != local_current_frame)
			{
				// @@ would this be more elegant if we passed back a "early-out" flag from execute?
				return true;
			}
		}}

		return false;
	}


	//
	// movie_impl
	//

	struct movie_impl : public movie
	{
		hash<int, character*>	m_characters;
		hash<int, font*>	m_fonts;
		hash<int, bitmap_character*>	m_bitmap_characters;
		array<array<execute_tag*> >	m_playlist;	// A list of movie control events for each frame.
		array<display_object_info>	m_display_list;	// active characters, ordered by depth.
		array<action_buffer*>	m_action_list;	// pending actions.

		// array<int>	m_frame_start;	// tag indices of frame starts. ?

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_frame_count;
		int	m_version;
		rgba	m_background_color;

		int	m_current_frame;
		float	m_time;

		movie_impl()
			:
			m_frame_rate(30.0f),
			m_frame_count(0),
			m_version(0),
			m_background_color(0.0f, 0.0f, 0.0f, 1.0f),
			m_current_frame(0),
			m_time(0.0f)
		{
		}

		virtual ~movie_impl() {}

		int	get_current_frame() const { return m_current_frame; }

		void	add_character(int character_id, character* c)
		{
			assert(c);
			assert(c->m_id == character_id);
			m_characters.add(character_id, c);
		}

		void	add_font(int font_id, font* f)
		{
			assert(f);
			m_fonts.add(font_id, f);
		}

		font*	get_font(int font_id)
		{
			font*	f = NULL;
			m_fonts.get(font_id, &f);
			return f;
		}

		bitmap_character*	get_bitmap_character(int character_id)
		{
			bitmap_character*	ch = 0;
			m_bitmap_characters.get(character_id, &ch);
			return ch;
		}

		void	add_bitmap_character(int character_id, bitmap_character* ch)
		{
			m_bitmap_characters.add(character_id, ch);
		}

		void	add_execute_tag(execute_tag* e)
		{
			assert(e);
			m_playlist[m_current_frame].push_back(e);
		}

		
		void	add_display_object(Uint16 character_id,
					   Uint16 depth,
					   const cxform& color_transform,
					   const matrix& matrix,
					   float ratio)
		{
			character*	ch = NULL;
			if (m_characters.get(character_id, &ch) == false)
			{
				fprintf(stderr, "movie_impl::add_display_object(): unknown cid = %d\n", character_id);
				return;
			}
			assert(ch);

			swf::add_display_object(&m_display_list, ch, depth, color_transform, matrix, ratio);
		}


		void	move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio)
		// Updates the transform properties of the object at
		// the specified depth.
		{
			swf::move_display_object(&m_display_list, depth, use_cxform, color_xform, use_matrix, mat, ratio);
		}


		void	replace_display_object(Uint16 character_id,
					       Uint16 depth,
					       bool use_cxform,
					       const cxform& color_transform,
					       bool use_matrix,
					       const matrix& mat,
					       float ratio)
		{
			character*	ch = NULL;
			if (m_characters.get(character_id, &ch) == false)
			{
				fprintf(stderr, "movie_impl::add_display_object(): unknown cid = %d\n", character_id);
				return;
			}
			assert(ch);

			swf::replace_display_object(&m_display_list, ch, depth, use_cxform, color_transform, use_matrix, mat, ratio);
		}


		void	remove_display_object(Uint16 depth)
		// Remove the object at the specified depth.
		{
			swf::remove_display_object(&m_display_list, depth);
		}

		void	add_action_buffer(action_buffer* a)
		// Add the given action buffer to the list of action
		// buffers to be processed at the end of the next
		// frame advance.
		{
			m_action_list.push_back(a);
		}

		int	get_width() { return (int) ceilf(TWIPS_TO_PIXELS(m_frame_size.m_x_max - m_frame_size.m_x_min)); }
		int	get_height() { return (int) ceilf(TWIPS_TO_PIXELS(m_frame_size.m_y_max - m_frame_size.m_y_min)); }

		void	set_background_color(const rgba& bg_color)
		{
			m_background_color = bg_color;
		}
		
		void	restart()
		{
			m_display_list.resize(0);
			m_action_list.resize(0);
			m_current_frame = -1;
			m_time = 0;
		}


		void	advance(float delta_time)
		{
			// Call advance() on each object in our
			// display list.  Takes care of sprites.
			for (int i = 0; i < m_display_list.size(); i++)
			{
				m_display_list[i].m_character->advance(delta_time);
			}

			m_time += delta_time;
			int	target_frame_number = (int) floorf(m_time * m_frame_rate);

			if (target_frame_number >= m_frame_count
			    || target_frame_number < 0)
			{
 				// Loop.  Correct?  Or should we hang?
  				target_frame_number = 0;
  				restart();
			}

			goto_frame(target_frame_number);
		}


		void	goto_frame(int target_frame_number)
		// Set the movie state at the specified frame number.
		// If the target frame number is in the future, we can
		// do this incrementally; otherwise we have to rewind
		// and advance from the beginning.
		{
			if (target_frame_number < m_current_frame)
			{
				// Can't incrementally update, so start from the beginning.
				restart();
			}

			assert(target_frame_number >= m_current_frame);
			assert(target_frame_number < m_frame_count);

			for (int frame = m_current_frame + 1; frame <= target_frame_number; frame++)
			{
				array<execute_tag*>&	playlist = m_playlist[frame];
				for (int i = 0; i < playlist.size(); i++)
				{
					execute_tag*	e = playlist[i];
					e->execute(this);
				}

				m_current_frame = frame;

				// Take care of this frame's actions.
				if (execute_actions(this, m_action_list))
				{
					// Some action called goto_frame() or restart() or something like that.
					// Exit this function immediately before undoing desired changes.
					return;
				}
				m_action_list.resize(0);
			}

//			IF_DEBUG(printf("m_time = %f, fr = %d\n", m_time, m_current_frame));
		}

		void	display()
		// Show our display list.
		{
			swf::render::begin_display(
				m_background_color,
				m_frame_size.m_x_min, m_frame_size.m_x_max,
				m_frame_size.m_y_min, m_frame_size.m_y_max);

			// Display all display objects.  Lower depths
			// are obscured by higher depths.
			for (int i = 0; i < m_display_list.size(); i++)
			{
				display_object_info&	di = m_display_list[i];
				di.m_character->display(di);

//				printf("display %s\n", typeid(*(di.m_character)).name());
			}

//			glPopMatrix();
			swf::render::end_display();
		}


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

			m_playlist.resize(m_frame_count);

			IF_DEBUG(m_frame_size.print(stdout));
			IF_DEBUG(printf("frame rate = %f, frames = %d\n", m_frame_rate, m_frame_count));

			while ((Uint32) str.get_position() < file_length)
			{
				int	tag_type = str.open_tag();
				loader_function	lf = NULL;
				if (tag_type == 1)
				{
					// show frame tag -- advance to the next frame.
					m_current_frame++;
				}
				else if (s_tag_loaders.get(tag_type, &lf))
				{
					// call the tag loader.  The tag loader should add
					// characters or tags to the movie data structure.
					(*lf)(&str, tag_type, this);

				} else {
					// no tag loader for this tag type.
					IF_DEBUG(printf("*** no tag loader for type %d\n", tag_type));
				}

				str.close_tag();
			}

			restart();
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
			register_tag_loader(2, define_shape_loader);
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(10, define_font_loader);
			register_tag_loader(11, define_text_loader);
			register_tag_loader(12, do_action_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
			register_tag_loader(22, define_shape_loader);
			register_tag_loader(26, place_object_2_loader);
			register_tag_loader(28, remove_object_2_loader);
			register_tag_loader(32, define_shape_loader);
			register_tag_loader(33, define_text_loader);
			register_tag_loader(36, define_bits_lossless_2_loader);
			register_tag_loader(39, sprite_loader);
			register_tag_loader(48, define_font_loader);
		}
	}


	movie_interface*	create_movie(SDL_RWops* in)
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

		void	execute(movie* m)
		{
			m->set_background_color(m_color);
		}

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
	struct bitmap_character_rgb : public bitmap_character
	{
		image::rgb*	m_image;
		swf::render::bitmap_info*	m_bitmap_info;

		bitmap_character_rgb()
			:
			m_image(0),
			m_bitmap_info(0)
		{
		}

		swf::render::bitmap_info*	get_bitmap_info()
		{
			if (m_image != 0)
			{
				// Create our bitmap info, from our image.
				m_bitmap_info = swf::render::create_bitmap_info(m_image);
				delete m_image;
				m_image = 0;
			}
			return m_bitmap_info;
		}
#if 0
		void	display(const display_info& di)
		{
			if (m_texture == 0)
			{
				// Create texture.
				glEnable(GL_TEXTURE_2D);
				glGenTextures(1, &m_texture);
				glBindTexture(GL_TEXTURE_2D, m_texture);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);

				int	w = 1; while (w < m_image->m_width) { w <<= 1; }
				int	h = 1; while (h < m_image->m_height) { h <<= 1; }

				char*	temp = new char[w * h * 3];
				memset(temp, 0, w * h * 3);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, temp);
				delete [] temp;

				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_image->m_width, m_image->m_height, GL_RGB, GL_UNSIGNED_BYTE, m_image->m_data);
			}

			printf("showing texture...\n");

			glBegin(GL_QUADS);

			glTexCoord2f(0, 0);
			glVertex2f(0, 0);

			glTexCoord2f(1, 0);
			glVertex2f(1, 0);

			glTexCoord2f(1, 1);
			glVertex2f(1, 1);

			glTexCoord2f(0, 1);
			glVertex2f(0, 1);

			glEnd();
		}
#endif // 0
	};


	struct bitmap_character_rgba : public character
	{
		image::rgba*	m_image;

		bitmap_character_rgba() : m_image(0) {}

		void	display(const display_info& di)
		{
			printf("bitmap_character_rgba display\n");
			/* TODO */
		}
	};


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 21);
		
		Uint16	character_id = in->read_u16();

		in->align();
		IF_DEBUG(printf("define_bits_jpeg2_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->m_id = character_id;

		//
		// Read the image data.
		//
		
		ch->m_image = image::read_swf_jpeg2(in->m_input);
//		unsigned int	bitmap_id = swf::render::create_bitmap_id(image);
//		delete image;

		m->add_bitmap_character(character_id, ch);

//		IF_DEBUG(printf("id = %d, bm = 0x%X, width = %d, height = %d, pitch = %d\n",
//				character_id,
//				(unsigned) ch->m_image,
//				ch->m_image->m_width,
//				ch->m_image->m_height,
//				ch->m_image->m_pitch));

//		// add image to movie, under character id.
//		m->add_character(character_id, ch);
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
		ch->m_id = character_id;
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


	//
	// shape stuff
	//


	// Together with the previous anchor, defines a quadratic
	// curve segment.
	struct edge
	{
		// *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
		float	m_cx, m_cy;		// "control" point
		float	m_ax, m_ay;		// "anchor" point
//		int	m_fill0, m_fill1, m_line;

		// An edge with m_fill0 == m_fill1 == m_line == -1 means "move to".

		edge()
			:
			m_cx(0), m_cy(0),
			m_ax(0), m_ay(0)
//			m_fill0(-1), m_fill1(-1), m_line(-1)
		{}

		edge(float cx, float cy, float ax, float ay)
			:
			m_cx(cx), m_cy(cy),
			m_ax(ax), m_ay(ay)
//			m_fill0(fill0),
//			m_fill1(fill1),
//			m_line(line)
		{
		}

#if 0
		bool	is_blank() const
		// Returns true if this is a "blank" edge; i.e. it
		// functions as a move-to and doesn't render anything
		// itself.
		{
			if (m_fill0 == -1 && m_fill1 == -1 && m_line == -1)
			{
				assert(m_cx == 0);
				assert(m_cy == 0);
				return true;
			}
			return false;
		}
#endif // 0

		void	emit_curve() const
		// Send this segment to the renderer.
		{
			swf::render::add_curve_segment(m_cx, m_cy, m_ax, m_ay);
		}
	};


	struct path
	{
		int	m_fill0, m_fill1, m_line;
		float	m_ax, m_ay;	// starting point
		array<edge>	m_edges;
		bool	m_new_shape;

		path()
			:
			m_new_shape(false)
		{
			reset(0, 0, -1, -1, -1);
		}

		path(float ax, float ay, int fill0, int fill1, int line)
		{
			reset(ax, ay, fill0, fill1, line);
		}


		void	reset(float ax, float ay, int fill0, int fill1, int line)
		// Reset all our members to the given values, and clear our edge list.
		{
			m_ax = ax;
			m_ay = ay,
			m_fill0 = fill0;
			m_fill1 = fill1;
			m_line = line;

			m_edges.resize(0);

			assert(is_empty());
		}


		bool	is_empty() const
		// Return true if we have no edges.
		{
			return m_edges.size() == 0;
		}

		
		void	display(
			const display_info& di,
			const array<fill_style>& fill_styles,
			const array<line_style>& line_styles) const
		// Render this path.
		{
			if (m_fill0 > 0)
			{
				fill_styles[m_fill0 - 1].apply(0);
//				swf::render::fill_style0_color(fill_styles[m_fill0 - 1].m_color);
			}
			else
			{
				swf::render::fill_style_disable(0);
			}

			if (m_fill1 > 0)
			{
				fill_styles[m_fill1 - 1].apply(1);
//				swf::render::fill_style1_color(fill_styles[m_fill1 - 1].m_color);
			}
			else
			{
				swf::render::fill_style_disable(1);
			}

			if (m_line > 0)
			{
				line_styles[m_line - 1].apply();
//				swf::render::line_style_color(line_styles[m_line - 1].m_color);
			}
			else
			{
				swf::render::line_style_disable();
			}

			swf::render::begin_path(m_ax, m_ay);
			for (int i = 0; i < m_edges.size(); i++)
			{
				m_edges[i].emit_curve();
			}
			swf::render::end_path();
		}
	};


	static void	read_fill_styles(array<fill_style>* styles, stream* in, int tag_type, movie* m)
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
			(*styles)[(*styles).size() - 1].read(in, tag_type, m);
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


	// Represents the outline of one or more shapes, along with
	// information on fill and line styles.
	struct shape_character : public character
	{
		rect	m_bound;
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<path>	m_paths;
//		array<edge>	m_edges;

		shape_character()
		{
		}

		virtual ~shape_character() {}

		void	read(stream* in, int tag_type, bool with_style, movie* m)
		{
			if (with_style)
			{
				m_bound.read(in);
				read_fill_styles(&m_fill_styles, in, tag_type, m);
				read_line_styles(&m_line_styles, in, tag_type);
			}

			//
			// SHAPE
			//
			int	num_fill_bits = in->read_uint(4);
			int	num_line_bits = in->read_uint(4);

			IF_DEBUG(printf("scr: nfb = %d, nlb = %d\n", num_fill_bits, num_line_bits));

			// These are state variables that keep the
			// current position & style of the shape
			// outline, and vary as we read the edge data.
			//
			// At the moment we just store each edge with
			// the full necessary info to render it, which
			// is simple but not optimally efficient.
			int	fill_base = 0;
			int	line_base = 0;
			float	x = 0, y = 0;
			path	current_path;

#define SHAPE_LOG 0
			// SHAPERECORDS
			for (;;) {
				int	type_flag = in->read_uint(1);
				if (type_flag == 0)
				{
					// Parse the record.
					int	flags = in->read_uint(5);
					if (flags == 0) {
						// End of shape records.

						// Store the current path if any.
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);
						}

						break;
					}
					if (flags & 0x01)
					{
						// move_to = 1;

						// Store the current path if any, and prepare a fresh one.
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);
						}

						int	num_move_bits = in->read_uint(5);
						int	move_x = in->read_sint(num_move_bits);
						int	move_y = in->read_sint(num_move_bits);

						x = move_x;
						y = move_y;

						// Set the beginning of the path.
						current_path.m_ax = x;
						current_path.m_ay = y;

						if (SHAPE_LOG) IF_DEBUG(printf("scr: moveto %4g %4g\n", x, y));
					}
					if ((flags & 0x02)
					    && num_fill_bits > 0)
					{
						// fill_style_0_change = 1;
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);
							current_path.m_ax = x;
							current_path.m_ay = y;
						}
						int	style = in->read_uint(num_fill_bits);
						if (style > 0)
						{
							style += fill_base;
						}
						current_path.m_fill0 = style;
						if (SHAPE_LOG) IF_DEBUG(printf("scr: fill0 = %d\n", current_path.m_fill0));
					}
					if ((flags & 0x04)
					    && num_fill_bits) {
						// fill_style_1_change = 1;
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);
							current_path.m_ax = x;
							current_path.m_ay = y;
						}
						int	style = in->read_uint(num_fill_bits);
						if (style > 0)
						{
							style += fill_base;
						}
						current_path.m_fill1 = style;
						if (SHAPE_LOG) IF_DEBUG(printf("scr: fill1 = %d\n", current_path.m_fill1));
					}
					if ((flags & 0x08)
					    && num_line_bits > 0)
					{
						// line_style_change = 1;
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);
							current_path.m_ax = x;
							current_path.m_ay = y;
						}
						int	style = in->read_uint(num_line_bits);
						if (style > 0)
						{
							style += line_base;
						}
						current_path.m_line = style;
						if (SHAPE_LOG) IF_DEBUG(printf("scr: line = %d\n", current_path.m_line));
					}
					if (flags & 0x10) {
						assert(tag_type >= 22);

						IF_DEBUG(printf("scr: more fill styles\n"));

						// Store the current path if any.
						if (! current_path.is_empty())
						{
							m_paths.push_back(current_path);
							current_path.m_edges.resize(0);

							// Clear styles.
							current_path.m_fill0 = -1;
							current_path.m_fill1 = -1;
							current_path.m_line = -1;
						}
						// Tack on an empty path signalling a new shape.
						// @@ need better understanding of whether this is correct??!?!!
						// @@ i.e., we should just start a whole new shape here, right?
						m_paths.push_back(path());
						m_paths.back().m_new_shape = true;

						fill_base = m_fill_styles.size();
						line_base = m_line_styles.size();
						read_fill_styles(&m_fill_styles, in, tag_type, m);
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

						if (SHAPE_LOG) IF_DEBUG(printf("scr: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", x, y, cx, cy, ax, ay));

						current_path.m_edges.push_back(edge(cx, cy, ax, ay));	

						x = ax;
						y = ay;
					}
					else
					{
						// straight edge
						int	num_bits = 2 + in->read_uint(4);
						int	line_flag = in->read_uint(1);
						float	dx = 0, dy = 0;
						if (line_flag)
						{
							// General line.
							dx = in->read_sint(num_bits);
							dy = in->read_sint(num_bits);
						}
						else
						{
							int	vert_flag = in->read_uint(1);
							if (vert_flag == 0) {
								// Horizontal line.
								dx = in->read_sint(num_bits);
							} else {
								// Vertical line.
								dy = in->read_sint(num_bits);
							}
						}

						if (SHAPE_LOG) IF_DEBUG(printf("scr: straight edge = %4g %4g - %4g %4g\n", x, y, x + dx, y + dy));

						current_path.m_edges.push_back(edge(x + dx/2, y + dy/2, x + dx, y + dy));

						x += dx;
						y += dy;
					}
				}
			}
		}


		void	display(const display_info& di)
		// Draw the shape using the given environment.
		{
			display(di, m_fill_styles);
		}

		
		void	display(const display_info& di, const array<fill_style>& fill_styles)
		// Display our shape.  Use the fill_styles arg to
		// override our default set of fill styles (e.g. when
		// rendering text).
		{
			// @@ set color transform into the renderer...
			// @@ set matrix into the renderer...

			// @@ emit edges to the renderer...

//			glMatrixMode(GL_MODELVIEW);
//			glPushMatrix();

//			di.m_matrix.ogl_multiply();
			swf::render::push_apply_matrix(di.m_matrix);
			swf::render::push_apply_cxform(di.m_color_transform);

			swf::render::begin_shape();
			for (int i = 0; i < m_paths.size(); i++)
			{
				if (m_paths[i].m_new_shape == true)
				{
					// @@ this is awful -- need a better shape loader!!!
					swf::render::end_shape();
					swf::render::begin_shape();
				}
				else
				{
					m_paths[i].display(di, fill_styles, m_line_styles);
				}
			}
			swf::render::end_shape();

			swf::render::pop_cxform();
			swf::render::pop_matrix();
//			glPopMatrix();
		}
	};

	
	void	define_shape_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 2
		       || tag_type == 22
		       || tag_type == 32);

		Uint16	character_id = in->read_u16();

		shape_character*	ch = new shape_character;
		ch->m_id = character_id;
		ch->read(in, tag_type, true, m);

		IF_DEBUG(printf("shape_loader: id = %d, rect ", character_id);
			 ch->m_bound.print(stdout));
		IF_DEBUG(printf("shape_loader: fill style ct = %d, line style ct = %d, path ct = %d\n",
				ch->m_fill_styles.size(), ch->m_line_styles.size(), ch->m_paths.size()));

		m->add_character(character_id, ch);
	}


	//
	// font
	//
	
	struct font
	{
		array<shape_character*>	m_glyphs;
		char*	m_name;
		// etc

		font()
			:
			m_name(NULL)
		{
		}

		~font()
		{
			// Iterate over m_glyphs and free the shapes.
			for (int i = 0; i < m_glyphs.size(); i++)
			{
				delete m_glyphs[i];
			}
			m_glyphs.resize(0);

			// Delete the name string.
			if (m_name)
			{
				delete [] m_name;
			}
		}

		shape_character*	get_glyph(int index)
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

		void	read(stream* in, int tag_type, movie* m)
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
//					assert(new_pos >= in->get_position());	// if we're seeking backwards, then that looks like a bug.
					in->set_position(new_pos);

					// Create & read the shape.
					shape_character*	s = new shape_character;
					s->read(in, 2, false, m);

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

				char*	m_name = in->read_string_with_length();

				// Avoid warnings.
				m_name = m_name;
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
					shape_character*	s = new shape_character;
					s->read(in, 22, false, m);

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
	

	void	define_font_loader(stream* in, int tag_type, movie* m)
	// Load a DefineFont or DefineFont2 tag.
	{
		assert(tag_type == 10 || tag_type == 48);

		Uint16	font_id = in->read_u16();
		
		font*	f = new font;
		f->read(in, tag_type, m);

//		IF_DEBUG(0);

		m->add_font(font_id, f);
	}


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
				m_glyphs[i].m_glyph_advance = in->read_sint(advance_bits);
			}
		}
	};


	struct text_character : public character
	{
		rect	m_rect;
		matrix	m_matrix;
		array<text_glyph_record>	m_text_glyph_records;
		array<fill_style>	m_dummy_style;	// used to pass a color on to shape_character::display()

		text_character()
		{
			m_dummy_style.push_back(fill_style());
		}

		void	read(stream* in, int tag_type, movie* m)
		{
			assert(m != NULL);
			assert(tag_type == 11);

			m_rect.read(in);
			m_matrix.read(in);

			int	glyph_bits = in->read_u8();
			int	advance_bits = in->read_u8();

			IF_DEBUG(printf("begin text records\n"));

			text_style	style;
			for (;;)
			{
				int	first_byte = in->read_u8();
				
				if (first_byte == 0)
				{
					// This is the end of the text records.
					IF_DEBUG(printf("end text records\n"));
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

					IF_DEBUG(printf("text style change\n"));

					if (has_font)
					{
						Uint16	font_id = in->read_u16();
						style.m_font = m->get_font(font_id);
						if (style.m_font == NULL)
						{
							printf("error: text style with undefined font; font_id = %d\n", font_id);
						}

						IF_DEBUG(printf("has_font: font id = %d\n", font_id));
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
						IF_DEBUG(printf("has_color\n"));
					}
					if (has_x_offset)
					{
						style.m_has_x_offset = true;
						style.m_x_offset = in->read_s16();
						IF_DEBUG(printf("has_x_offset = %g\n", style.m_x_offset));
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
						IF_DEBUG(printf("has_y_offset = %g\n", style.m_y_offset));
					}
					else
					{
						style.m_has_y_offset = false;
						style.m_y_offset = 0.0f;
					}
					if (has_font)
					{
						style.m_text_height = in->read_u16();
						IF_DEBUG(printf("text_height = %g\n", style.m_text_height));
					}
				}
				else
				{
					// Read the glyph record.
					int	glyph_count = first_byte & 0x7F;
					m_text_glyph_records.resize(m_text_glyph_records.size() + 1);
					m_text_glyph_records.back().m_style = style;
					m_text_glyph_records.back().read(in, glyph_count, glyph_bits, advance_bits);

					IF_DEBUG(printf("glyph_records: count = %d\n", glyph_count));
				}
			}
		}


		void	display(const display_info& di)
		// Draw the string.
		{
			// @@ it would probably be nicer to just parse
			// this tag from the original SWF data,
			// on-the-fly as we render.

			display_info	sub_di = di;
			sub_di.m_matrix.concatenate(m_matrix);

			matrix	base_matrix = sub_di.m_matrix;

			float	scale = 1.0f;
			float	x = 0.0f;
			float	y = 0.0f;

			for (int i = 0; i < m_text_glyph_records.size(); i++)
			{
				text_glyph_record&	rec = m_text_glyph_records[i];

				if (rec.m_style.m_font == NULL) continue;

				sub_di.m_matrix = base_matrix;

				scale = rec.m_style.m_text_height / 1024.0f;	// the EM square is 1024 x 1024
				if (rec.m_style.m_has_x_offset)
				{
					x = rec.m_style.m_x_offset;
				}
				if (rec.m_style.m_has_y_offset)
				{
					y = rec.m_style.m_y_offset;
				}
				sub_di.m_matrix.concatenate_translation(x, y);
				sub_di.m_matrix.m_[0][0] *= scale;
				sub_di.m_matrix.m_[1][1] *= scale;

				m_dummy_style[0].m_color = rec.m_style.m_color;

				for (int j = 0; j < rec.m_glyphs.size(); j++)
				{
					int	index = rec.m_glyphs[j].m_glyph_index;
					shape_character*	glyph = rec.m_style.m_font->get_glyph(index);

					// Draw the character.
					if (glyph) glyph->display(sub_di, m_dummy_style);

					float	dx = rec.m_glyphs[j].m_glyph_advance;
					x += dx;
					sub_di.m_matrix.concatenate_translation(dx / scale, 0);
				}
			}
		}
	};


	void	define_text_loader(stream* in, int tag_type, movie* m)
	// Read a DefineText tag.
	{
		assert(tag_type == 11 || tag_type == 33);

		Uint16	character_id = in->read_u16();
		
		text_character*	ch = new text_character;
		ch->m_id = character_id;
		IF_DEBUG(printf("text_character, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		// IF_DEBUG(print some stuff);

		m->add_character(character_id, ch);
	}


	//
	// place_object_2
	//
	
	struct place_object_2 : public execute_tag
	{
		char*	m_name;
		float	m_ratio;
		cxform	m_color_transform;
		matrix	m_matrix;
		bool	m_has_matrix;
		bool	m_has_cxform;
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
			m_has_matrix(false),
			m_has_cxform(false),
			m_depth(0),
			m_character_id(0),
			m_place_type(PLACE)
		{
		}

		void	read(stream* in)
		{
			in->align();

			in->read_uint(1);	// reserved flag
			bool	has_clip_bracket = in->read_uint(1) ? true : false;
			bool	has_name = in->read_uint(1) ? true : false;
			bool	has_ratio = in->read_uint(1) ? true : false;
			bool	has_cxform = in->read_uint(1) ? true : false;
			bool	has_matrix = in->read_uint(1) ? true : false;
			bool	has_char = in->read_uint(1) ? true : false;
			bool	flag_move = in->read_uint(1) ? true : false;

			m_depth = in->read_u16();

			if (has_char) {
				m_character_id = in->read_u16();
			}
			if (has_matrix) {
				m_has_matrix = true;
				m_matrix.read(in);
			}
			if (has_cxform) {
				m_has_cxform = true;
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

		
		void	execute(movie* m)
		// Place/move/whatever our object in the given movie.
		{
			switch (m_place_type)
			{
			case PLACE:
//				IF_DEBUG(printf("  place: cid %2d depth %2d\n", m_character_id, m_depth));
				m->add_display_object(m_character_id,
						      m_depth,
						      m_color_transform,
						      m_matrix,
						      m_ratio);
				break;

			case MOVE:
//				IF_DEBUG(printf("   move: depth %2d\n", m_depth));
				m->move_display_object(m_depth,
						       m_has_cxform,
						       m_color_transform,
						       m_has_matrix,
						       m_matrix,
						       m_ratio);
				break;

			case REPLACE:
			{
//				IF_DEBUG(printf("replace: cid %d depth %d\n", m_character_id, m_depth));
				m->replace_display_object(m_character_id,
							  m_depth,
							  m_has_cxform,
							  m_color_transform,
							  m_has_matrix,
							  m_matrix,
							  m_ratio);
				break;
			}

			}
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
	//
	// The sprite implementation is divided into a
	// sprite_definition and a sprite_instance.  The _definition
	// holds the immutable data for a sprite, while the _instance
	// contains the state for a specific instance being updated
	// and displayed in the parent movie's display list.


	struct sprite_definition : public character
	{
		movie_impl*	m_movie;		// parent movie.
		array<array<execute_tag*> >	m_playlist;	// movie control events for each frame.
		int	m_frame_count;
		int	m_current_frame;

		sprite_definition(movie_impl* m)
			:
			m_movie(m),
			m_frame_count(0),
			m_current_frame(0)
		{
			assert(m_movie);
		}

		bool	is_definition() const { return true; }
		character*	create_instance();

		void	add_character(int id, character* ch)
		{
			fprintf(stderr, "error: sprite::add_character() is illegal\n");
			assert(0);
		}

		void	add_execute_tag(execute_tag* c)
		{
			m_playlist[m_current_frame].push_back(c);
		}

		void	read(stream* in)
		// Read the sprite info.  Consists of a series of tags.
		{
			int	tag_end = in->get_tag_end_position();

			m_frame_count = in->read_u16();
			m_playlist.resize(m_frame_count);	// need a playlist for each frame

			IF_DEBUG(printf("sprite: frames = %d\n", m_frame_count));

			m_current_frame = 0;

			while ((Uint32) in->get_position() < (Uint32) tag_end)
			{
				int	tag_type = in->open_tag();
				loader_function lf = NULL;
				if (tag_type == 1)
				{
					// show frame tag -- advance to the next frame.
					m_current_frame++;
				}
				else if (s_tag_loaders.get(tag_type, &lf))
				{
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


	struct sprite_instance : public character
	{
		sprite_definition*	m_def;
		int	m_current_frame;
		array<display_object_info>	m_display_list;	// active characters, ordered by depth.
		array<action_buffer*>	m_action_list;

		float	m_time;

		virtual ~sprite_instance() {}

		sprite_instance(sprite_definition* def)
			:
			m_def(def),
			m_current_frame(-1),
			m_time(0)
		{
			assert(m_def);
			m_id = def->m_id;
		}

		bool	is_instance() const { return true; }

		int	get_width() { assert(0); return 0; }
		int	get_height() { assert(0); return 0; }

		int	get_current_frame() const { return m_current_frame; }

		void	restart()
		{
			m_display_list.resize(0);
			m_action_list.resize(0);
			m_current_frame = -1;
			m_time = 0;
		}


		void	advance(float delta_time)
		{
			assert(m_def && m_def->m_movie);

			// Call advance() on each object in our
			// display list.  Takes care of sprites.
			for (int i = 0; i < m_display_list.size(); i++)
			{
				m_display_list[i].m_character->advance(delta_time);
			}

			m_time += delta_time;
			int	target_frame_number = (int) floorf(m_time * m_def->m_movie->m_frame_rate);

			if (target_frame_number >= m_def->m_frame_count
			    || target_frame_number < 0)
			{
				// Loop.  Correct?  Or should we hang?
				target_frame_number = 0;
				restart();
//				// Hang.
//				target_frame_number = m_def->m_frame_count - 1;
//				m_display_list.resize(0);
			}

			goto_frame(target_frame_number);
		}


		void	goto_frame(int target_frame_number)
		// Set the movie state at the specified frame number.
		// If the target frame number is in the future, we can
		// do this incrementally; otherwise we have to rewind
		// and advance from the beginning.
		{
			if (target_frame_number < m_current_frame)
			{
				// Can't incrementally update, so start from the beginning.
				restart();
			}

			assert(target_frame_number >= m_current_frame);
			assert(target_frame_number < m_def->m_frame_count);

			for (int frame = m_current_frame + 1; frame <= target_frame_number; frame++)
			{
				// Handle the normal frame updates.
				array<execute_tag*>&	playlist = m_def->m_playlist[frame];
				for (int i = 0; i < playlist.size(); i++)
				{
					execute_tag*	e = playlist[i];
					e->execute(this);
				}

				m_current_frame = frame;

				// Take care of this frame's actions.
				if (execute_actions(this, m_action_list))
				{
					// Some action called goto_frame() or restart() or something like that.
					// Exit this function immediately before undoing desired changes.
					return;
				}
				m_action_list.resize(0);
			}

//			IF_DEBUG(printf("m_time = %f, fr = %d\n", m_time, m_current_frame));
		}

		void	display()
		{
			// don't call this one; call display(const display_info&);
			assert(0);
		}

		void	display(const display_info& di)
		{
			// Show the character id at each depth.
			IF_DEBUG(
			{
				printf("s %2d | ", m_id);
				for (int i = 1; i < 10; i++)
				{
					int	id = get_id_at_depth(i);
					if (id == -1) { printf("%d:     ", i); }
					else { printf("%d: %2d  ", i, id); }
				}
				printf("\n");
			});

			// Show the display list.
			for (int i = 0; i < m_display_list.size(); i++)
			{
				display_object_info&	dobj = m_display_list[i];

				display_info	sub_di = di;
				sub_di.concatenate(dobj);
				dobj.m_character->display(sub_di);
			}
		}


		void	add_display_object(Uint16 character_id,
					   Uint16 depth,
					   const cxform& color_transform,
					   const matrix& matrix,
					   float ratio)
		// Add an object to the display list.
		{
			assert(m_def && m_def->m_movie);

			character*	ch = NULL;
			if (m_def->m_movie->m_characters.get(character_id, &ch) == false)
			{
				fprintf(stderr, "sprite::add_display_object(): unknown cid = %d\n", character_id);
				return;
			}
			assert(ch);

			swf::add_display_object(&m_display_list, ch, depth, color_transform, matrix, ratio);
		}


		void	move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio)
		// Updates the transform properties of the object at
		// the specified depth.
		{
			swf::move_display_object(&m_display_list, depth, use_cxform, color_xform, use_matrix, mat, ratio);
		}


		void	replace_display_object(Uint16 character_id,
					       Uint16 depth,
					       bool use_cxform,
					       const cxform& color_transform,
					       bool use_matrix,
					       const matrix& mat,
					       float ratio)
		{
			assert(m_def && m_def->m_movie);

			character*	ch = NULL;
			if (m_def->m_movie->m_characters.get(character_id, &ch) == false)
			{
				fprintf(stderr, "sprite::add_display_object(): unknown cid = %d\n", character_id);
				return;
			}
			assert(ch);

			swf::replace_display_object(&m_display_list, ch, depth, use_cxform, color_transform, use_matrix, mat, ratio);
		}


		void	remove_display_object(Uint16 depth)
		// Remove the object at the specified depth.
		{
			swf::remove_display_object(&m_display_list, depth);
		}


		void	add_action_buffer(action_buffer* a)
		// Add the given action buffer to the list of action
		// buffers to be processed at the end of the next
		// frame advance.
		{
			m_action_list.push_back(a);
		}


		int	get_id_at_depth(int depth)
		// For debugging -- return the id of the character at the specified depth.
		// Return -1 if nobody's home.
		{
			int	index = find_display_index(m_display_list, depth);
			if (index >= m_display_list.size()
			    || m_display_list[index].m_depth != depth)
			{
				// No object at that depth.
				return -1;
			}

			character*	ch = m_display_list[index].m_character;

			return ch->m_id;
		}
	};


	character*	sprite_definition::create_instance()
	// Create a (mutable) instance of our definition.  The
	// instance is created so live (temporarily) on some level on
	// the parent movie's display list.
	{
		return new sprite_instance(this);
	}


	void	sprite_loader(stream* in, int tag_type, movie* m)
	// Create and initialize a sprite, and add it to the movie.
	{
		assert(tag_type == 39);

		int	character_id = in->read_u16();
		UNUSED(character_id);

		movie_impl*	mi = dynamic_cast<movie_impl*>(m);
		assert(mi);

		sprite_definition*	ch = new sprite_definition(mi);
		ch->m_id = character_id;
		ch->read(in);

		IF_DEBUG(printf("sprite: char id = %d\n", character_id));

		m->add_character(character_id, ch);
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

		void	execute(movie* m)
		{
//			IF_DEBUG(printf(" remove: depth %2d\n", m_depth));
			m->remove_display_object(m_depth);
		}
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 28);

		remove_object_2*	t = new remove_object_2;
		t->read(in);

		m->add_execute_tag(t);
	}


	//
	// action stuff
	//

	void	action_buffer::read(stream* in)
	{
		// Read action bytes.
		for (;;)
		{
			int	action_id = in->read_u8();
			m_buffer.push_back(action_id);
			if (action_id == 0)
			{
				// end of action buffer.
				return;
			}
			if (action_id & 0x80)
			{
				// Action contains extra data.  Read it.
				int	length = in->read_u16();
				m_buffer.push_back(length & 0x0FF);
				m_buffer.push_back((length >> 8) & 0x0FF);
				for (int i = 0; i < length; i++)
				{
					m_buffer.push_back(in->read_u8());
				}
			}
		}
	}


	void	action_buffer::execute(movie* m)
	// Interpret the actions in this action buffer, and apply them
	// to the given movie.
	{
		assert(m);

		movie*	original_movie = m;

		// Avoid warnings.
		original_movie = original_movie;

		for (int pc = 0; pc < m_buffer.size(); )
		{
			int	action_id = m_buffer[pc];
			if ((action_id & 0x80) == 0)
			{
				IF_DEBUG(printf("aid = 0x%02x\n", action_id));

				// Simple action; no extra data.
				switch (action_id)
				{
				default:
				case 0x08:	// toggle quality
				case 0x09:	// stop sounds
					break;

				case 0x00:	// end of actions.
					return;

				case 0x04:	// next frame.
					m->goto_frame(m->get_current_frame() + 1);
					break;

				case 0x05:	// prev frame.
					m->goto_frame(m->get_current_frame() - 1);
					break;

				case 0x06:	// action play
//					m->set_playing(true);
					break;

				case 0x07:	// action stop
//					m->set_playing(false);
					break;
				}
				pc++;	// advance to next action.
			}
			else
			{
				IF_DEBUG(printf("aid = 0x%02x ", action_id));

				// Action containing extra data.
				int	length = m_buffer[pc + 1] | (m_buffer[pc + 2] << 8);
				int	next_pc = pc + length + 3;

				IF_DEBUG({for (int i = 0; i < imin(length, 8); i++) { printf("0x%02x ", m_buffer[pc + 3 + i]); } printf("\n"); });
				
				switch (action_id)
				{
				default:
					break;

				case 0x81:	// goto frame
				{
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					m->goto_frame(frame);
					break;
				}

				case 0x83:	// get url
				{
					// @@ TODO should call back into client app, probably...
					break;
				}

				case 0x8A:	// wait for frame
				{
					// @@ TODO I think this has to deal with incremental loading;
					// i.e. we don't really care about it.
					break;
				}

				case 0x8B:	// set target
				{
					// Change the movie we're working on.
					// @@ TODO
					// char* target_name = &m_buffer[pc + 3];
					// if (target_name[0] == 0) { m = original_movie; }
					// else {
					//	m = m->find_labeled_target(target_name);
					//	if (m == NULL) m = original_movie;
					// }
					break;
				}

				case 0x8C:	// go to labeled frame
				{
					// char*	frame_label = (char*) &m_buffer[pc + 3];
					// m->goto_labeled_frame(frame_label);
					break;
				}
				
				}
				pc = next_pc;
			}
		}
	}


	//
	// do_action
	//

	// Thin wrapper around action_buffer.
	struct do_action : public execute_tag
	{
		action_buffer	m_buf;

		void	read(stream* in)
		{
			m_buf.read(in);
		}

		void	execute(movie* m)
		{
			m->add_action_buffer(&m_buf);
		}
	};

	void	do_action_loader(stream* in, int tag_type, movie* m)
	{
		IF_DEBUG(printf("tag %d: do_action_loader\n", tag_type));

		assert(in);
		assert(tag_type == 12);
		assert(m);
		
		do_action*	da = new do_action;
		da->read(in);

		m->add_execute_tag(da);
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
