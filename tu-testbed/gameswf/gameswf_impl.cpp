// gameswf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://www.openswf.org

// @@ Need to break this file into pieces


#include "engine/ogl.h"
#include "engine/tu_file.h"
#include "gameswf_action.h"
#include "gameswf_button.h"
#include "gameswf_impl.h"
#include "gameswf_font.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_styles.h"
#include "engine/image.h"
#include "engine/jpeg.h"
#include <string.h>	// for memset
#include <zlib.h>
#include <typeinfo>
#include <float.h>


namespace gameswf
{
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
			IF_DEBUG(printf("error: move_display_object() -- no objects on display list\n"));
//			assert(0);
			return;
		}

		int	index = find_display_index(*display_list, depth);
		if (index < 0 || index >= size)
		{
			// error.
			IF_DEBUG(printf("error: move_display_object() -- can't find object at depth %d\n", depth));
//			assert(0);
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
			IF_DEBUG(printf("remove_display_object: no characters in display list"));
			return;
		}

		int	index = find_display_index(*display_list, depth);
		if (index < 0 || index >= size)
		{
			// error -- no character at the given depth.
			IF_DEBUG(printf("remove_display_object: no character at depth %d\n", depth));
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


	static bool	execute_actions(movie* m, const array<action_buffer*>& action_list)
	// Execute the actions in the action list, on the given movie.
	// Return true if the actions did something to change the
	// current_frame or play state of the movie.
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
		string_hash<int>	m_named_frames;

		// array<int>	m_frame_start;	// tag indices of frame starts. ?

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_frame_count;
		int	m_version;
		rgba	m_background_color;

		play_state	m_play_state;
		int	m_current_frame;
		int	m_next_frame;
		int	m_total_display_count;
		float	m_time_remainder;
		bool	m_update_frame;
		int	m_mouse_x, m_mouse_y, m_mouse_buttons;

		jpeg::input*	m_jpeg_in;


		movie_impl()
			:
			m_frame_rate(30.0f),
			m_frame_count(0),
			m_version(0),
			m_background_color(0.0f, 0.0f, 0.0f, 1.0f),
			m_play_state(PLAY),
			m_current_frame(0),
			m_next_frame(0),
			m_total_display_count(0),
			m_time_remainder(0.0f),
			m_update_frame(true),
			m_mouse_x(0),
			m_mouse_y(0),
			m_mouse_buttons(0),
			m_jpeg_in(0)
		{
		}

		virtual ~movie_impl()
		{
			if (m_jpeg_in)
			{
				delete m_jpeg_in;
			}
		}

		int	get_current_frame() const { return m_current_frame; }
		int	get_frame_count() const { return m_frame_count; }

		void	notify_mouse_state(int x, int y, int buttons)
		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		{
			m_mouse_x = x;
			m_mouse_y = y;
			m_mouse_buttons = buttons;
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		// Use this to retrieve the last state of the mouse, as set via
		// notify_mouse_state().  Coordinates are in PIXELS, NOT TWIPS.
		{
			assert(x);
			assert(y);
			assert(buttons);

			*x = m_mouse_x;
			*y = m_mouse_y;
			*buttons = m_mouse_buttons;
		}

		void	add_character(int character_id, character* c)
		{
			assert(c);
			assert(c->m_id == character_id);
			m_characters.add(character_id, c);
		}

		character*	get_character(int character_id)
		{
			character*	ch = NULL;
			m_characters.get(character_id, &ch);
			return ch;
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

		void	add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
		{
			assert(m_current_frame >= 0 && m_current_frame < m_frame_count);

			tu_string	n = name;
			assert(m_named_frames.get(n, NULL) == false);	// frame should not already have a name (?)
			m_named_frames.add(n, m_current_frame);
		}

		
		void	set_jpeg_loader(jpeg::input* j_in)
		// Set an input object for later loading DefineBits
		// images (JPEG images without the table info).
		{
			assert(m_jpeg_in == NULL);
			m_jpeg_in = j_in;
		}

		jpeg::input*	get_jpeg_loader()
		// Get the jpeg input loader, to load a DefineBits
		// image (one without table info).
		{
			return m_jpeg_in;
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

			gameswf::add_display_object(&m_display_list, ch, depth, color_transform, matrix, ratio);
		}


		void	move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio)
		// Updates the transform properties of the object at
		// the specified depth.
		{
			gameswf::move_display_object(&m_display_list, depth, use_cxform, color_xform, use_matrix, mat, ratio);
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

			gameswf::replace_display_object(&m_display_list, ch, depth, use_cxform, color_transform, use_matrix, mat, ratio);
		}


		void	remove_display_object(Uint16 depth)
		// Remove the object at the specified depth.
		{
			gameswf::remove_display_object(&m_display_list, depth);
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

		void	set_play_state(play_state s)
		// Stop or play the movie.
		{
			if (m_play_state != s)
			{
				m_time_remainder = 0;
			}

			m_play_state = s;
		}
		
		void	restart()
		{
		//	m_display_list.resize(0);
		//	m_action_list.resize(0);
			m_current_frame = 0;
			m_next_frame = 0;
			m_time_remainder = 0;
			m_update_frame = true;
		}

		void	advance(float delta_time)
		{
			m_time_remainder += delta_time;
			const float	frame_time = 1.0f / m_frame_rate;

			// Check for the end of frame
			if (m_time_remainder >= frame_time)
			{
				m_time_remainder -= frame_time;
				m_update_frame = true;
			}

			while (m_update_frame)
			{
				m_update_frame = false;
				m_current_frame = m_next_frame;
				m_next_frame = m_current_frame + 1;

			//	IF_DEBUG(printf("### frame updated: current_frame=%d\n", m_current_frame));

				if (m_current_frame == 0)
				{
					m_display_list.clear();
					m_action_list.clear();
				}


				// Execute the current frame's tags.
				if (m_play_state == PLAY) 
				{
					execute_frame_tags(m_current_frame);

					// Perform frame actions
					do_actions();
				}


				// Advance everything in the display list.
				for (int i = 0; i < m_display_list.size(); i++)
				{
					m_display_list[i].m_character->advance(frame_time, this, m_display_list[i].m_matrix);
				}

				// Perform button actions
				do_actions();


				// Update next frame according to actions
				if (m_play_state == STOP)
				{
					m_next_frame = m_current_frame;
					if (m_next_frame >= m_frame_count)
					{
						m_next_frame = m_frame_count;
					}
				}
				else if (m_next_frame >= m_frame_count)	// && m_play_state == PLAY
				{
  					m_next_frame = 0;
					if (m_frame_count > 1)
					{
						// Avoid infinite loop on single frame movies
						m_update_frame = true;
					}
				}

			//	printf( "### display list size = %d\n", m_display_list.size() );

				// Check again for the end of frame
				if (m_time_remainder >= frame_time)
				{
					m_time_remainder -= frame_time;
					m_update_frame = true;
				}
			}
		}


		void	execute_frame_tags(int frame, bool state_only=false)
		// Execute the tags associated with the specified frame.
		{
			assert(frame >= 0);
			assert(frame < m_frame_count);

			array<execute_tag*>&	playlist = m_playlist[frame];
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (state_only)
				{
					e->execute_state(this);
				}
				else
				{
					e->execute(this);
				}
			}
		}


		void	do_actions()
		// Take care of this frame's actions.
		{
			execute_actions(this, m_action_list);
			m_action_list.resize(0);
		}


		void	goto_frame(int target_frame_number)
		// Set the movie state at the specified frame number.
		{
			IF_DEBUG(printf("goto_frame(%d)\n", target_frame_number));//xxxxx

			if (target_frame_number != m_current_frame
			    && target_frame_number >= 0
			    && target_frame_number < m_frame_count)
			{
				if (m_play_state == STOP)
				{
					target_frame_number++;	// if stopped, update_frame won't increase it
					m_current_frame = target_frame_number;
				}
				m_next_frame = target_frame_number;

				m_display_list.clear();
				for (int f = 0; f < target_frame_number; f++)
				{
					execute_frame_tags(f, true);
				}
			}

			m_update_frame = true;
			m_time_remainder = 0 /* @@ frame time */;
		}

		void	display()
		// Show our display list.
		{
			gameswf::render::begin_display(
				m_background_color,
				m_frame_size.m_x_min, m_frame_size.m_x_max,
				m_frame_size.m_y_min, m_frame_size.m_y_max);

			// Display all display objects.  Lower depths
			// are obscured by higher depths.
			for (int i = 0; i < m_display_list.size(); i++)
			{
				display_object_info&	di = m_display_list[i];
				di.m_display_number = m_total_display_count;
				di.m_character->display(di);

//				printf("display %s\n", typeid(*(di.m_character)).name());
			}

			gameswf::render::end_display();

			m_total_display_count++;
		}


		void	read(tu_file* in)
		// Read a .SWF movie.
		{
			Uint32	header = in->read_le32();
			Uint32	file_length = in->read_le32();

			int	m_version = (header >> 24) & 255;
			if ((header & 0x0FFFFFF) != 0x00535746)
			{
				// ERROR
				IF_DEBUG(printf("gameswf::movie_impl::read() -- file does not start with a SWF header!\n"));
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

			if (m_jpeg_in)
			{
				delete m_jpeg_in;
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
			register_tag_loader(4, place_object_2_loader);
			register_tag_loader(6, define_bits_jpeg_loader);	// turn this off; needs debugging
			register_tag_loader(7, button_character_loader);
			register_tag_loader(8, jpeg_tables_loader);		// turn this off; needs debugging
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(10, define_font_loader);
			register_tag_loader(11, define_text_loader);
			register_tag_loader(12, do_action_loader);
			register_tag_loader(20, define_bits_lossless_2_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
			register_tag_loader(22, define_shape_loader);
			register_tag_loader(24, null_loader);	// "protect" tag; we're not an authoring tool so we don't care.
			register_tag_loader(26, place_object_2_loader);
			register_tag_loader(28, remove_object_2_loader);
			register_tag_loader(32, define_shape_loader);
			register_tag_loader(33, define_text_loader);
			register_tag_loader(34, button_character_loader);
			register_tag_loader(35, define_bits_jpeg3_loader);
			register_tag_loader(36, define_bits_lossless_2_loader);
			register_tag_loader(39, sprite_loader);
			register_tag_loader(43, frame_label_loader);
			register_tag_loader(48, define_font_loader);
		}
	}


	movie_interface*	create_movie(tu_file* in)
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


	void	null_loader(stream* in, int tag_type, movie* m)
	// Silently ignore the contents of this tag.
	{
	}

	void	frame_label_loader(stream* in, int tag_type, movie* m)
	// Label the current frame of m with the name from the stream.
	{
		char*	n = in->read_string();
		m->add_frame_name(n);
		delete [] n;
	}

	struct set_background_color : public execute_tag
	{
		rgba	m_color;

		void	execute(movie* m)
		{
			m->set_background_color(m_color);
		}

		void	execute_state(movie* m) {
			execute(m);
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
		gameswf::render::bitmap_info*	m_bitmap_info;

		bitmap_character_rgb()
			:
			m_image(0),
			m_bitmap_info(0)
		{
		}

		gameswf::render::bitmap_info*	get_bitmap_info()
		{
			if (m_image != 0)
			{
				// Create our bitmap info, from our image.
				m_bitmap_info = gameswf::render::create_bitmap_info(m_image);
				delete m_image;
				m_image = 0;
			}
			return m_bitmap_info;
		}
	};


	struct bitmap_character_rgba : public bitmap_character
	{
		image::rgba*	m_image;
		gameswf::render::bitmap_info*	m_bitmap_info;

		bitmap_character_rgba() : m_image(0) {}

		gameswf::render::bitmap_info*	get_bitmap_info()
		{
			if (m_image != 0)
			{
				// Create our bitmap info, from our image.
				m_bitmap_info = gameswf::render::create_bitmap_info(m_image);
				delete m_image;
				m_image = 0;
			}
			return m_bitmap_info;
		}
	};


	void	jpeg_tables_loader(stream* in, int tag_type, movie* m)
	// Load JPEG compression tables that can be used to load
	// images further along in the stream.
	{
		assert(tag_type == 8);

		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2_header_only(in->m_input);
		assert(j_in);

		m->set_jpeg_loader(j_in);
	}


	void	define_bits_jpeg_loader(stream* in, int tag_type, movie* m)
	// A JPEG image without included tables; those should be in an
	// existing jpeg::input object stored in the movie.
	{
		assert(tag_type == 6);

		Uint16	character_id = in->read_u16();

		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->m_id = character_id;

		//
		// Read the image data.
		//
		
		jpeg::input*	j_in = m->get_jpeg_loader();
		assert(j_in);
		j_in->discard_partial_buffer();
		ch->m_image = image::read_swf_jpeg2_with_tables(j_in);
		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 21);
		
		Uint16	character_id = in->read_u16();

		IF_DEBUG(printf("define_bits_jpeg2_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->m_id = character_id;

		//
		// Read the image data.
		//
		
		ch->m_image = image::read_swf_jpeg2(in->m_input);
		m->add_bitmap_character(character_id, ch);
	}


	void	inflate_wrapper(tu_file* in, void* buffer, int buffer_bytes)
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
			buf[0] = in->read_byte();
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


	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie* m)
	// loads a define_bits_jpeg3 tag. This is a jpeg file with an alpha
	// channel using zlib compression.
	{
		assert(tag_type == 35);

		Uint16	character_id = in->read_u16();

		IF_DEBUG(printf("define_bits_jpeg3_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		Uint32	jpeg_size = in->read_u32();
		Uint32	alpha_position = in->get_position() + jpeg_size;

		bitmap_character_rgba*	ch = new bitmap_character_rgba();
		ch->m_id = character_id;

		//
		// Read the image data.
		//
		
		ch->m_image = image::read_swf_jpeg3(in->m_input);

		in->set_position(alpha_position);
		
		int	buffer_bytes = ch->m_image->m_width * ch->m_image->m_height;
		Uint8*	buffer = new Uint8[buffer_bytes];

		inflate_wrapper(in->m_input, buffer, buffer_bytes);

		for (int i = 0; i < buffer_bytes; i++)
		{
			ch->m_image->m_data[4*i+3] = buffer[i];
		}

		delete [] buffer;

		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 20 || tag_type == 36);

		Uint16	character_id = in->read_u16();
		Uint8	bitmap_format = in->read_u8();	// 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
		Uint16	width = in->read_u16();
		Uint16	height = in->read_u16();

		IF_DEBUG(printf("dbl2l: tag_type = %d, id = %d, fmt = %d, w = %d, h = %d\n",
				tag_type,
				character_id,
				bitmap_format,
				width,
				height));

		if (tag_type == 20)
		{
			// RGB image data.
			bitmap_character_rgb*	ch = new bitmap_character_rgb();
			ch->m_id = character_id;
			ch->m_image = image::create_rgb(width, height);

			if (bitmap_format == 3)
			{
				// 8-bit data, preceded by a palette.

				const int	bytes_per_pixel = 1;
				int	color_table_size = in->read_u8();
				color_table_size += 1;	// !! SWF stores one less than the actual size

				int	pitch = (width * bytes_per_pixel + 3) & ~3;

				int	buffer_bytes = color_table_size * 3 + pitch * height;
				Uint8*	buffer = new Uint8[buffer_bytes];

				inflate_wrapper(in->m_input, buffer, buffer_bytes);
				assert(in->get_tag_end_position() == in->get_position());

				Uint8*	color_table = buffer;

				for (int j = 0; j < height; j++)
				{
					Uint8*	image_in_row = buffer + color_table_size * 3 + j * pitch;
					Uint8*	image_out_row = image::scanline(ch->m_image, j);
					for (int i = 0; i < width; i++)
					{
						Uint8	pixel = image_in_row[i * bytes_per_pixel];
						image_out_row[i * 3 + 0] = color_table[pixel * 3 + 0];
						image_out_row[i * 3 + 1] = color_table[pixel * 3 + 1];
						image_out_row[i * 3 + 2] = color_table[pixel * 3 + 2];
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
						image_out_row[i * 3 + 0] = (pixel >> 8) & 0xF8;	// red
						image_out_row[i * 3 + 1] = (pixel >> 3) & 0xFC;	// green
						image_out_row[i * 3 + 2] = (pixel << 3) & 0xF8;	// blue
					}
				}
			
				delete [] buffer;
			}
			else if (bitmap_format == 5)
			{
				// 32 bits / pixel, input is ARGB format (???)
				const int	bytes_per_pixel = 4;
				int	pitch = width * bytes_per_pixel;

				int	buffer_bytes = pitch * height;
				Uint8*	buffer = new Uint8[buffer_bytes];

				inflate_wrapper(in->m_input, buffer, buffer_bytes);
				assert(in->get_tag_end_position() == in->get_position());
			
				// Need to re-arrange ARGB into RGB.
				for (int j = 0; j < height; j++)
				{
					Uint8*	image_in_row = buffer + j * pitch;
					Uint8*	image_out_row = image::scanline(ch->m_image, j);
					for (int i = 0; i < width; i++)
					{
						Uint8	a = image_in_row[i * 4 + 0];
						Uint8	r = image_in_row[i * 4 + 1];
						Uint8	g = image_in_row[i * 4 + 2];
						Uint8	b = image_in_row[i * 4 + 3];
						image_out_row[i * 3 + 0] = r;
						image_out_row[i * 3 + 1] = g;
						image_out_row[i * 3 + 2] = b;
						a = a;	// Inhibit warning.
					}
				}

				delete [] buffer;
			}

			// add image to movie, under character id.
			m->add_bitmap_character(character_id, ch);
		}
		else
		{
			// RGBA image data.
			assert(tag_type == 36);

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
			m->add_bitmap_character(character_id, ch);
		}
	}


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
	// font loaders
	//


	void	define_font_loader(stream* in, int tag_type, movie* m)
	// Load a DefineFont or DefineFont2 tag.
	{
		assert(tag_type == 10 || tag_type == 48);

		Uint16	font_id = in->read_u16();
		
		font*	f = new font;
		f->read(in, tag_type, m);

//		IF_DEBUG(0);

		m->add_font(font_id, f);

		fontlib::add_font(f);
	}


	void	define_font_info_loader(stream* in, int tag_type, movie* m)
	// Load a DefineFontInfo tag.  This adds information to an
	// existing font.
	{
		assert(tag_type == 13);

		Uint16	font_id = in->read_u16();
		
		font*	f = m->get_font(font_id);
		if (f)
		{
			f->read_font_info(in);
		}
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
			assert(tag_type == 11 || tag_type == 33);

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

				scale = rec.m_style.m_text_height / 1024.0f;	// the EM square is 1024 x 1024
				if (rec.m_style.m_has_x_offset)
				{
					x = rec.m_style.m_x_offset;
				}
				if (rec.m_style.m_has_y_offset)
				{
					y = rec.m_style.m_y_offset;
				}

				m_dummy_style[0].set_color(rec.m_style.m_color);

				for (int j = 0; j < rec.m_glyphs.size(); j++)
				{
					int	index = rec.m_glyphs[j].m_glyph_index;
					const texture_glyph*	tg = rec.m_style.m_font->get_texture_glyph(index);
					
					sub_di.m_matrix = base_matrix;
					sub_di.m_matrix.concatenate_translation( x, y );
					sub_di.m_matrix.concatenate_scale( scale );


					if (tg)
					{
						// Draw the glyph using the cached texture-map info.
						gameswf::render::push_apply_matrix(sub_di.m_matrix);
						gameswf::render::push_apply_cxform(sub_di.m_color_transform);
						fontlib::draw_glyph(tg, rec.m_style.m_color);
						gameswf::render::pop_cxform();
						gameswf::render::pop_matrix();
					}
					else
					{
						shape_character*	glyph = rec.m_style.m_font->get_glyph(index);

						// Draw the character using the filled outline.
						if (glyph) glyph->display(sub_di, m_dummy_style);
					}

					x += rec.m_glyphs[j].m_glyph_advance;
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

		void	read(stream* in, int tag_type)
		{
			assert(tag_type == 4 || tag_type == 26);

			if (tag_type == 4)
			{
				// Original place_object tag; very simple.
				m_character_id = in->read_u16();
				m_depth = in->read_u16();
				m_matrix.read(in);

				if (in->get_position() < in->get_tag_end_position())
				{
					m_color_transform.read_rgb(in);
				}
			}
			else if (tag_type == 26)
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

		void	execute_state(movie* m)
		{
			execute(m);
		}
	};


	
	void	place_object_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 4 || tag_type == 26);

		place_object_2*	ch = new place_object_2;
		ch->read(in, tag_type);

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
		array<display_object_info>	m_display_list;	// active characters, ordered by depth.
		array<action_buffer*>	m_action_list;

		play_state	m_play_state;
		int	m_current_frame;
		int	m_next_frame;
		float	m_time_remainder;
		bool	m_update_frame;

		virtual ~sprite_instance() {}

		sprite_instance(sprite_definition* def)
			:
			m_def(def),
			m_play_state(PLAY),
			m_current_frame(0),
			m_next_frame(0),
			m_time_remainder(0),
			m_update_frame(true)
		{
			assert(m_def);
			m_id = def->m_id;
		}

		bool	is_instance() const { return true; }

		int	get_width() { assert(0); return 0; }
		int	get_height() { assert(0); return 0; }

		int	get_current_frame() const { return m_current_frame; }
		int	get_frame_count() const { return m_def->m_frame_count; }

		void	set_play_state(play_state s)
		// Stop or play the movie.
		{
			if (m_play_state != s)
			{
				m_time_remainder = 0;
			}

			m_play_state = s;
		}

		void	restart()
		{
			m_current_frame = 0;
			m_next_frame = 0;
			m_time_remainder = 0;
			m_update_frame = true;
		}


		void	advance(float delta_time, movie* m, const matrix& mat)
		{
			assert(m_def && m_def->m_movie);

			m_time_remainder += delta_time;
			const float	frame_time = 1.0f / m_def->m_movie->m_frame_rate;

			// Check for the end of frame
			if (m_time_remainder >= frame_time)
			{
				m_time_remainder -= frame_time;
				m_update_frame = true;
			}

			while (m_update_frame)
			{
				m_update_frame = false;
				m_current_frame = m_next_frame;
				m_next_frame = m_current_frame + 1;

				if (m_current_frame == 0)
				{
					m_display_list.clear();
					m_action_list.clear();
				}

				// Execute the current frame's tags.
				if (m_play_state == PLAY) 
				{
					execute_frame_tags(m_current_frame);

					// Perform frame actions
					do_actions();
				}


				// Advance everything in the display list.
				for (int i = 0; i < m_display_list.size(); i++)
				{
					matrix	sub_matrix = mat;
					sub_matrix.concatenate(m_display_list[i].m_matrix);
					m_display_list[i].m_character->advance(frame_time, this, sub_matrix);
				}

				// Perform button actions
				do_actions();


				// Update next frame according to actions
				if (m_play_state == STOP)
				{
					m_next_frame = m_current_frame;
					if (m_next_frame >= m_def->m_frame_count)
					{
						m_next_frame = m_def->m_frame_count;
					}
				}
				else if (m_next_frame >= m_def->m_frame_count)	// && m_play_state == PLAY
				{
  					m_next_frame = 0;
					if (m_def->m_frame_count > 1)
					{
						// Avoid infinite loop on single frame sprites?
						m_update_frame = true;
					}
				}

				// Check again for the end of frame
				if (m_time_remainder >= frame_time)
				{
					m_time_remainder -= frame_time;
					m_update_frame = true;
				}
			}
		}


		void	execute_frame_tags(int frame, bool state_only=false)
		// Execute the tags associated with the specified frame.
		{
			assert(frame >= 0);
			assert(frame < m_def->m_frame_count);

			array<execute_tag*>&	playlist = m_def->m_playlist[frame];
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (state_only)
				{
					e->execute_state(this);
				}
				else
				{
					e->execute(this);
				}
			}
		}


		void	do_actions()
		// Take care of this frame's actions.
		{
			execute_actions(this, m_action_list);
			m_action_list.resize(0);
		}


		void	goto_frame(int target_frame_number)
		// Set the sprite state at the specified frame number.
		{
			IF_DEBUG(printf("goto_frame(%d)\n", target_frame_number));//xxxxx

			if (target_frame_number != m_current_frame
			    && target_frame_number >= 0
			    && target_frame_number < m_def->m_frame_count)
			{
				if (m_play_state == STOP)
				{
					target_frame_number++;	// if stopped, update_frame won't increase it
					m_current_frame = target_frame_number;
				}
				m_next_frame = target_frame_number;

				m_display_list.clear();
				for (int f = 0; f < target_frame_number; f++)
				{
					execute_frame_tags(f, true);
				}
			}

			m_update_frame = true;
			m_time_remainder = 0 /* @@ frame time */;
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

			gameswf::add_display_object(&m_display_list, ch, depth, color_transform, matrix, ratio);
		}


		void	move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio)
		// Updates the transform properties of the object at
		// the specified depth.
		{
			gameswf::move_display_object(&m_display_list, depth, use_cxform, color_xform, use_matrix, mat, ratio);
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

			gameswf::replace_display_object(&m_display_list, ch, depth, use_cxform, color_transform, use_matrix, mat, ratio);
		}


		void	remove_display_object(Uint16 depth)
		// Remove the object at the specified depth.
		{
			gameswf::remove_display_object(&m_display_list, depth);
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

		void	get_mouse_state(int* x, int* y, int* buttons)
		// Use this to retrieve the last state of the mouse, as set via
		// notify_mouse_state().
		{
			m_def->m_movie->get_mouse_state(x, y, buttons);
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

		void	execute_state(movie* m)
		{
			execute(m);
		}
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 28);

		remove_object_2*	t = new remove_object_2;
		t->read(in);

		m->add_execute_tag(t);
	}


	void	button_character_loader(stream* in, int tag_type, movie* m)
	{
		assert(tag_type == 7 || tag_type == 34);

		int	character_id = in->read_u16();

		button_character_definition*	ch = new button_character_definition;
		ch->m_id = character_id;
		ch->read(in, tag_type, m);

		m->add_character(character_id, ch);
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

		void	execute_state(movie* m)
		{
			// left empty because actions don't have to be replayed when seeking the movie.
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
