// gameswf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://www.openswf.org

// @@ Need to break this file into pieces


#include "base/tu_file.h"
#include "base/utility.h"
#include "gameswf_action.h"
#include "gameswf_button.h"
#include "gameswf_impl.h"
#include "gameswf_font.h"
#include "gameswf_fontlib.h"
#include "gameswf_log.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_styles.h"
#include "gameswf_dlist.h"
#include "base/image.h"
#include "base/jpeg.h"
#include "base/zlib_adapter.h"
#include <string.h>	// for memset
#include <zlib.h>
#include <typeinfo>
#include <float.h>


namespace gameswf
{
	bool	s_verbose_action = false;
	bool	s_verbose_parse = false;

#ifndef NDEBUG
	bool	s_verbose_debug = true;
#else
	bool	s_verbose_debug = false;
#endif


	void	set_verbose_action(bool verbose)
	// Enable/disable log messages re: actions.
	{
		s_verbose_action = verbose;
	}


	void	set_verbose_parse(bool verbose)
	// Enable/disable log messages re: parsing the movie.
	{
		s_verbose_parse = verbose;
	}


	static bool	s_use_cache_files = true;

	void	set_use_cache_files(bool use_cache)
	// Enable/disable attempts to read cache files when loading
	// movies.
	{
		s_use_cache_files = use_cache;
	}


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


	//
	// file_opener callback stuff
	//

	static file_opener_function	s_opener_function = NULL;

	void	register_file_opener_callback(file_opener_function opener)
	// Host calls this to register a function for opening files,
	// for loading movies.
	{
		s_opener_function = opener;
	}


	//
	// some utility stuff
	//


	static void	execute_actions(as_environment* env, const array<action_buffer*>& action_list)
	// Execute the actions in the action list, in the given
	// environment.
	{
		for (int i = 0; i < action_list.size(); i++)
		{
			action_list[i]->execute(env);
		}
	}


	static void	dump_tag_bytes(stream* in)
	// Log the contents of the current tag, in hex.
	{
		static const int	ROW_BYTES = 16;
		char	row_buf[ROW_BYTES];
		int	row_count = 0;

		while(in->get_position() < in->get_tag_end_position())
		{
			int	c = in->read_u8();
			log_msg("%02X", c);

			if (c < 32) c = '.';
			if (c > 127) c = '.';
			row_buf[row_count] = c;
			
			row_count++;
			if (row_count >= ROW_BYTES)
			{
				log_msg("    ");
				for (int i = 0; i < ROW_BYTES; i++)
				{
					log_msg("%c", row_buf[i]);
				}

				log_msg("\n");
				row_count = 0;
			}
			else
			{
				log_msg(" ");
			}
		}

		if (row_count > 0)
		{
			log_msg("\n");
		}
	}


	character*	character_def::create_character_instance(movie* parent)
	// Default.  Make a generic_character.
	{
		return new generic_character(this, parent);
	}


	void	character::do_mouse_drag()
	// Implement mouse-dragging for this movie.
	{
		drag_state	st;
		get_drag_state(&st);
		if (this == st.m_character)
		{
			// We're being dragged!
			int	x, y, buttons;
			get_root_movie()->get_mouse_state(&x, &y, &buttons);

			point	world_mouse(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
			if (st.m_bound)
			{
				// Clamp mouse coords within a defined rect.
				world_mouse.m_x =
					fclamp(world_mouse.m_x, st.m_bound_x0, st.m_bound_x1);
				world_mouse.m_y =
					fclamp(world_mouse.m_y, st.m_bound_y0, st.m_bound_y1);
			}

			if (st.m_lock_center)
			{
				matrix	world_mat = get_world_matrix();
				point	local_mouse;
				world_mat.transform_by_inverse(&local_mouse, world_mouse);

				matrix	parent_world_mat;
				if (m_parent)
				{
					parent_world_mat = m_parent->get_world_matrix();
				}

				point	parent_mouse;
				parent_world_mat.transform_by_inverse(&parent_mouse, world_mouse);
					
				// Place our origin so that it coincides with the mouse coords
				// in our parent frame.
				matrix	local = get_matrix();
				local.m_[0][2] = parent_mouse.m_x;
				local.m_[1][2] = parent_mouse.m_y;
				set_matrix(local);
			}
			else
			{
				// Implement relative drag...
			}
		}
	}


	//
	// movie_def_impl
	//
	// This class holds the immutable definition of a movie's
	// contents.  It cannot be played directly, and does not hold
	// current state; for that you need to call create_instance()
	// to get a movie_instance.
	//


	struct movie_def_impl : public movie_definition_sub
	{
		hash<int, character_def*>	m_characters;
//		string_hash< array<int>* >	m_named_characters;
		hash<int, font*>	m_fonts;
		hash<int, bitmap_character_def*>	m_bitmap_characters;
		hash<int, sound_sample*>	m_sound_samples;
		array<array<execute_tag*> >	m_playlist;	// A list of movie control events for each frame.
		string_hash<int>	m_named_frames;
		string_hash<resource*>	m_exports;

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_frame_count;
		int	m_version;
		int	m_loading_frame;

		jpeg::input*	m_jpeg_in;

		movie_def_impl()
			:
			m_frame_rate(30.0f),
			m_frame_count(0),
			m_version(0),
			m_loading_frame(0),
			m_jpeg_in(0)
		{
		}

		movie_interface*	create_instance();

		// ...
		int	get_frame_count() const { return m_frame_count; }
		float	get_frame_rate() const { return m_frame_rate; }
		int	get_width() const { return (int) ceilf(TWIPS_TO_PIXELS(m_frame_size.width())); }
		int	get_height() const { return (int) ceilf(TWIPS_TO_PIXELS(m_frame_size.height())); }

		virtual int	get_loading_frame() const { return m_loading_frame; }

		virtual void	export_resource(const tu_string& symbol, resource* res)
		// Expose one of our resources under the given symbol,
		// for export.  Other movies can import it.
		{
			// SWF sometimes exports the same thing more than once!
			if (m_exports.get(symbol, NULL) == false)
			{
				m_exports.add(symbol, res);
			}
		}


		virtual resource*	get_exported_resource(const tu_string& symbol)
		// Get the named exported resource, if we expose it.
		// Otherwise return NULL.
		{
			resource*	res = NULL;
			m_exports.get(symbol, &res);
			return res;
		}


		void	add_character(int character_id, character_def* c)
		{
			assert(c);
			assert(c->get_id() == character_id);
			m_characters.add(character_id, c);
		}

		character_def*	get_character_def(int character_id)
		{
			character_def*	ch = NULL;
			m_characters.get(character_id, &ch);
			return ch;
		}

		bool	get_labeled_frame(const char* label, int* frame_number)
		{
			return m_named_frames.get(label, frame_number);
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

		bitmap_character_def*	get_bitmap_character(int character_id)
		{
			bitmap_character_def*	ch = 0;
			m_bitmap_characters.get(character_id, &ch);
			return ch;
		}

		void	add_bitmap_character(int character_id, bitmap_character_def* ch)
		{
			m_bitmap_characters.add(character_id, ch);
		}

		sound_sample*	get_sound_sample(int character_id)
		{
			sound_sample*	ch = 0;
			m_sound_samples.get(character_id, &ch);
			return ch;
		}

		virtual void	add_sound_sample(int character_id, sound_sample* sam)
		{
			m_sound_samples.add(character_id, sam);
		}

		void	add_execute_tag(execute_tag* e)
		{
			assert(e);
			m_playlist[m_loading_frame].push_back(e);
		}

		void	add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
		{
			assert(m_loading_frame >= 0 && m_loading_frame < m_frame_count);

			tu_string	n = name;
			assert(m_named_frames.get(n, NULL) == false);	// frame should not already have a name (?)
			m_named_frames.add(n, m_loading_frame);
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


		virtual const array<execute_tag*>&	get_playlist(int frame_number) { return m_playlist[frame_number]; }


		void	read(tu_file* in)
		// Read a .SWF movie.
		{
			Uint32	header = in->read_le32();
			Uint32	file_length = in->read_le32();

			m_version = (header >> 24) & 255;
			if ((header & 0x0FFFFFF) != 0x00535746
			    && (header & 0x0FFFFFF) != 0x00535743)
			{
				// ERROR
				log_error("gameswf::movie_def_impl::read() -- file does not start with a SWF header!\n");
				return;
			}
			bool	compressed = (header & 255) == 'C';

			IF_VERBOSE_PARSE(log_msg("version = %d, file_length = %d\n", m_version, file_length));

			tu_file*	original_in = NULL;
			if (compressed)
			{
				IF_VERBOSE_PARSE(log_msg("file is compressed.\n"));
				original_in = in;

				// Uncompress the input as we read it.
				in = zlib_adapter::make_inflater(original_in);

				// Subtract the size of the 8-byte header, since
				// it's not included in the compressed
				// stream length.
				file_length -= 8;
			}

			stream	str(in);

			m_frame_size.read(&str);
			m_frame_rate = str.read_u16() / 256.0f;
			m_frame_count = str.read_u16();

			m_playlist.resize(m_frame_count);

			IF_VERBOSE_PARSE(m_frame_size.print());
			IF_VERBOSE_PARSE(log_msg("frame rate = %f, frames = %d\n", m_frame_rate, m_frame_count));

			while ((Uint32) str.get_position() < file_length)
			{
				int	tag_type = str.open_tag();
				loader_function	lf = NULL;
				IF_VERBOSE_PARSE(log_msg("tag_type = %d\n", tag_type));
				if (tag_type == 1)
				{
					// show frame tag -- advance to the next frame.
					m_loading_frame++;
				}
				else if (s_tag_loaders.get(tag_type, &lf))
				{
					// call the tag loader.  The tag loader should add
					// characters or tags to the movie data structure.
					(*lf)(&str, tag_type, this);

				} else {
					// no tag loader for this tag type.
					IF_VERBOSE_PARSE(log_msg("*** no tag loader for type %d\n", tag_type));
					IF_VERBOSE_PARSE(dump_tag_bytes(&str));
				}

				str.close_tag();

				if (tag_type == 0)
				{
					if ((unsigned int)str.get_position() != file_length)
					{
						// Safety break, so we don't read past the end of the
						// movie.
						log_msg("warning: hit stream-end tag, but not at the "
							"end of the file yet; stopping for safety\n");
						break;
					}
				}
			}

			if (m_jpeg_in)
			{
				delete m_jpeg_in;
			}

			if (original_in)
			{
				// Done with the zlib_adapter.
				delete in;
			}
		}


		void	get_owned_fonts(array<font*>* fonts)
		// Fill up *fonts with fonts that we own.
		{
			for (hash<int, font*>::iterator it = m_fonts.begin();
			     it != m_fonts.end();
			     ++it)
			{
				font*	f = it.get_value();
				if (f->get_owning_movie() == this)
				{
					fonts->push_back(f);
				}
			}
		}


		void	generate_font_bitmaps()
		// Generate bitmaps for our fonts, if necessary.
		{
			// Collect list of fonts.
			array<font*>	fonts;
			get_owned_fonts(&fonts);
			fontlib::generate_font_bitmaps(fonts);
		}


		// Increment this when the cache data format changes.
		#define CACHE_FILE_VERSION 1


		void	output_cached_data(tu_file* out)
		// Dump our cached data into the given stream.
		{
			// Write a little header.
			char	header[5];
			strcpy(header, "gscX");
			header[3] = CACHE_FILE_VERSION;
			compiler_assert(CACHE_FILE_VERSION < 256);

			out->write_bytes(header, 4);

			// Write font data.
			array<font*>	fonts;
			get_owned_fonts(&fonts);
			fontlib::output_cached_data(out, fonts);

			// Write character data.
			{for (hash<int, character_def*>::iterator it = m_characters.begin();
			     it != m_characters.end();
			     ++it)
			{
				out->write_le16(it.get_key());
				it.get_value()->output_cached_data(out);
			}}

			out->write_le16((Sint16) -1);	// end of characters marker
		}


		void	input_cached_data(tu_file* in)
		// Read in cached data and use it to prime our loaded characters.
		{
			// Read the header & check version.
			unsigned char	header[4];
			in->read_bytes(header, 4);
			if (header[0] != 'g' || header[1] != 's' || header[2] != 'c')
			{
				log_error("cache file does not have the correct format; skipping\n");
				return;
			}
			else if (header[3] != CACHE_FILE_VERSION)
			{
				log_error(
					"cached data is version %d, but we require version %d; skipping\n",
					int(header[3]), CACHE_FILE_VERSION);
				return;
			}

			// Read the cached font data.
			array<font*>	fonts;
			get_owned_fonts(&fonts);
			fontlib::input_cached_data(in, fonts);

			// Read the cached character data.
			for (;;)
			{
				if (in->get_error() != TU_FILE_NO_ERROR)
				{
					log_error("error reading cache file (characters); skipping\n");
					return;
				}
				if (in->get_eof())
				{
					log_error("unexpected eof reading cache file (characters); skipping\n");
					return;
				}

				Sint16	id = in->read_le16();
				if (id == (Sint16) -1) { break; }	// done

				character_def* ch = NULL;
				m_characters.get(id, &ch);
				if (ch)
				{
					ch->input_cached_data(in);
				}
				else
				{
					log_error("sync error in cache file (reading characters)!  "
						  "Skipping rest of cache data.\n");
					return;
				}
			}
		}

	};


	//
	// movie_root
	//
	// Global, shared root state for a movie and all its characters.
	//

	struct movie_root : public movie_interface
	{
		movie_def_impl*	m_def;
		movie*	m_movie;
		int	m_viewport_x0, m_viewport_y0, m_viewport_width, m_viewport_height;
		float	m_pixel_scale;

		rgba	m_background_color;
		float	m_timer;
		int	m_mouse_x, m_mouse_y, m_mouse_buttons;
		int	m_mouse_capture_id;

		movie::drag_state	m_drag_state;

		movie_root(movie_def_impl* def)
			:
			m_def(def),
			m_movie(NULL),
			m_viewport_x0(0),
			m_viewport_y0(0),
			m_viewport_width(1),
			m_viewport_height(1),
			m_pixel_scale(1.0f),
			m_background_color(0, 0, 0, 255),
			m_timer(0.0f),
			m_mouse_x(0),
			m_mouse_y(0),
			m_mouse_buttons(0),
			m_mouse_capture_id(-1)
		{
			set_display_viewport(0, 0, m_def->get_width(), m_def->get_height());
		}


		void	set_root_movie(movie* root_movie) { m_movie = root_movie; assert(m_movie); }

		void	set_display_viewport(int x0, int y0, int w, int h)
		{
			m_viewport_x0 = x0;
			m_viewport_y0 = y0;
			m_viewport_width = w;
			m_viewport_height = h;

			// Recompute pixel scale.
			float	scale_x = m_viewport_width / TWIPS_TO_PIXELS(m_def->m_frame_size.width());
			float	scale_y = m_viewport_height / TWIPS_TO_PIXELS(m_def->m_frame_size.height());
			m_pixel_scale = fmax(scale_x, scale_y);
		}


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

		movie*	get_root_movie() { return m_movie; }

		int	get_mouse_capture()
		// Use this to retrive the character that has captured the mouse.
		{
			return m_mouse_capture_id;
		}

		void	set_mouse_capture(int cid)
		// Set the mouse capture to the given character.
		{
			m_mouse_capture_id = cid;
		}

		
		void	stop_drag()
		{
			m_drag_state.m_character = NULL;
		}


		movie_definition*	get_movie_definition() { return m_movie->get_movie_definition(); }

		int	get_current_frame() const { return m_movie->get_current_frame(); }
		float	get_frame_rate() const { return m_def->get_frame_rate(); }

		virtual float	get_pixel_scale() const
		// Return the size of a logical movie pixel as
		// displayed on-screen, with the current device
		// coordinates.
		{
			return m_pixel_scale;
		}

		character_def*	get_character_def(int character_id)
		{
			return m_def->get_character_def(character_id);
		}

		// @@ Is this one necessary?
		character*	get_character(int character_id)
		{
			return m_movie->get_character(character_id);
		}

		void	set_background_color(const rgba& color)
		{
			m_background_color = color;
		}

		void	set_background_alpha(float alpha)
		{
			m_background_color.m_a = iclamp(frnd(alpha * 255.0f), 0, 255);
		}

		float	get_background_alpha() const
		{
			return m_background_color.m_a / 255.0f;
		}

		float	get_timer() const { return m_timer; }

		void	restart() { m_movie->restart(); }
		void	advance(float delta_time)
		{
			m_timer += delta_time;
			m_movie->advance(delta_time);
		}
		void	goto_frame(int target_frame_number) { m_movie->goto_frame(target_frame_number); }

		virtual bool	has_looped() const { return m_movie->has_looped(); }

		void	display()
		{
			gameswf::render::begin_display(
				m_background_color,
				m_viewport_x0, m_viewport_y0,
				m_viewport_width, m_viewport_height,
				m_def->m_frame_size.m_x_min, m_def->m_frame_size.m_x_max,
				m_def->m_frame_size.m_y_min, m_def->m_frame_size.m_y_max);

			m_movie->display();

			gameswf::render::end_display();
		}

		virtual void	goto_labeled_frame(const char* label)
		{
			int	target_frame = -1;
			if (m_def->get_labeled_frame(label, &target_frame))
			{
				goto_frame(target_frame);
			}
			else
			{
				log_error("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label);
			}
		}

		virtual void	set_play_state(play_state s) { m_movie->set_play_state(s); }
		virtual play_state	get_play_state() const { return m_movie->get_play_state(); }
		virtual bool	set_edit_text(const char* varname, const char* text)
		{
			return m_movie->set_edit_text(varname, text);
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
			register_tag_loader(6, define_bits_jpeg_loader);
			register_tag_loader(7, button_character_loader);
			register_tag_loader(8, jpeg_tables_loader);
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(10, define_font_loader);
			register_tag_loader(11, define_text_loader);
			register_tag_loader(12, do_action_loader);
			register_tag_loader(13, define_font_info_loader);
			register_tag_loader(14, define_sound_loader);
			register_tag_loader(15, start_sound_loader);
			register_tag_loader(20, define_bits_lossless_2_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
			register_tag_loader(22, define_shape_loader);
			register_tag_loader(24, null_loader);	// "protect" tag; we're not an authoring tool so we don't care.
			register_tag_loader(26, place_object_2_loader);
			register_tag_loader(28, remove_object_2_loader);
			register_tag_loader(32, define_shape_loader);
			register_tag_loader(33, define_text_loader);
			register_tag_loader(37, define_edit_text_loader);
			register_tag_loader(34, button_character_loader);
			register_tag_loader(35, define_bits_jpeg3_loader);
			register_tag_loader(36, define_bits_lossless_2_loader);
			register_tag_loader(39, sprite_loader);
			register_tag_loader(43, frame_label_loader);
			register_tag_loader(48, define_font_loader);
			register_tag_loader(56, export_loader);
			register_tag_loader(57, import_loader);
		}
	}



	void	get_movie_info(
		const char* filename,
		int* version,
		int* width,
		int* height,
		float* frames_per_second,
		int* frame_count
		)
	// Attempt to read the header of the given .swf movie file.
	// Put extracted info in the given vars.
	// Sets *version to 0 if info can't be extracted.
	{
		if (s_opener_function == NULL)
		{
			log_error("error: get_movie_info(): no file opener function registered\n");
			if (version) *version = 0;
			return;
		}

		tu_file*	in = s_opener_function(filename);
		if (in == NULL || in->get_error() != TU_FILE_NO_ERROR)
		{
			log_error("error: get_movie_info(): can't open '%s'\n", filename);
			if (version) *version = 0;
			delete in;
			return;
		}

		Uint32	header = in->read_le32();
		Uint32	file_length = in->read_le32();

		int	local_version = (header >> 24) & 255;
		if ((header & 0x0FFFFFF) != 0x00535746
		    && (header & 0x0FFFFFF) != 0x00535743)
		{
			// ERROR
			log_error("error: get_movie_info(): file '%s' does not start with a SWF header!\n", filename);
			if (version) *version = 0;
			delete in;
			return;
		}
		bool	compressed = (header & 255) == 'C';

		tu_file*	original_in = NULL;
		if (compressed)
		{
			original_in = in;

			// Uncompress the input as we read it.
			in = zlib_adapter::make_inflater(original_in);

			// Subtract the size of the 8-byte header, since
			// it's not included in the compressed
			// stream length.
			file_length -= 8;
		}

		stream	str(in);

		rect	frame_size;
		frame_size.read(&str);

		float	local_frame_rate = str.read_u16() / 256.0f;
		int	local_frame_count = str.read_u16();

		if (version) *version = local_version;
		if (width) *width = int(frame_size.width() / 20.0f + 0.5f);
		if (height) *height = int(frame_size.height() / 20.0f + 0.5f);
		if (frames_per_second) *frames_per_second = local_frame_rate;
		if (frame_count) *frame_count = local_frame_count;

		delete in;
		delete original_in;
	}



	movie_definition*	create_movie(const char* filename)
	// Create the movie definition from the specified .swf file.
	{
		return create_movie_sub(filename);
	}


	movie_definition_sub*	create_movie_sub(const char* filename)
	{
		if (s_opener_function == NULL)
		{
			// Don't even have a way to open the file.
			log_error("error: no file opener function; can't create movie.  "
				  "See gameswf::register_file_opener_callback\n");
			return NULL;
		}

		tu_file* in = s_opener_function(filename);
		if (in == NULL)
		{
			log_error("failed to open '%s'; can't create movie.\n", filename);
			return NULL;
		}
		else if (in->get_error())
		{
			log_error("error: file opener can't open '%s'\n", filename);
			return NULL;
		}

		ensure_loaders_registered();

		movie_def_impl*	m = new movie_def_impl;
		m->read(in);

		delete in;

		if (m && s_use_cache_files)
		{
			// Try to load a .gsc file.
			tu_string	cache_filename(filename);
			cache_filename += ".gsc";
			tu_file*	cache_in = s_opener_function(cache_filename.c_str());
			if (cache_in == NULL
			    || cache_in->get_error() != TU_FILE_NO_ERROR)
			{
				// Can't open cache file; don't sweat it.
				IF_VERBOSE_PARSE(log_msg("note: couldn't open cache file '%s'\n", cache_filename.c_str()));

				m->generate_font_bitmaps();	// can't read cache, so generate font texture data.
			}
			else
			{
				// Load the cached data.
				m->input_cached_data(cache_in);
			}

			delete cache_in;
		}

		return m;
	}


	//
	// library stuff, for sharing resources among different movies.
	//


	static string_hash<movie_definition_sub*>	s_movie_library;

	movie_definition*	create_library_movie(const char* filename)
	// Try to load a movie from the given url, if we haven't
	// loaded it already.  Add it to our library on success, and
	// return a pointer to it.
	{
		return create_library_movie_sub(filename);
	}


	movie_definition_sub*	create_library_movie_sub(const char* filename)
	{
		tu_string	fn(filename);

		// Is the movie already in the library?
		{
			movie_definition_sub*	m = NULL;
			s_movie_library.get(fn, &m);
			if (m)
			{
				// Return cached movie.
				return m;
			}
		}

		// Try to open a file under the filename.
		movie_definition_sub*	mov = create_movie_sub(filename);

		if (mov == NULL)
		{
			log_error("error: couldn't load library movie '%s'\n", filename);
		}
		else
		{
			s_movie_library.add(fn, mov);
		}

		return mov;
	}
	

	//
	// Some tag implementations
	//


	void	null_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Silently ignore the contents of this tag.
	{
	}

	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m)
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
			float	current_alpha = m->get_background_alpha();
			m_color.m_a = frnd(current_alpha * 255.0f);
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


	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 9);
		assert(m);

		set_background_color*	t = new set_background_color;
		t->read(in);

		m->add_execute_tag(t);
	}


	// Bitmap character
	struct bitmap_character_rgb : public bitmap_character_def
	{
		image::rgb*	m_image;
		gameswf::bitmap_info*	m_bitmap_info;

		bitmap_character_rgb()
			:
			m_image(0),
			m_bitmap_info(0)
		{
		}

		gameswf::bitmap_info*	get_bitmap_info()
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


	struct bitmap_character_rgba : public bitmap_character_def
	{
		image::rgba*	m_image;
		gameswf::bitmap_info*	m_bitmap_info;

		bitmap_character_rgba() : m_image(0) {}

		gameswf::bitmap_info*	get_bitmap_info()
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


	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load JPEG compression tables that can be used to load
	// images further along in the stream.
	{
		assert(tag_type == 8);

		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2_header_only(in->get_underlying_stream());
		assert(j_in);

		m->set_jpeg_loader(j_in);
	}


	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m)
	// A JPEG image without included tables; those should be in an
	// existing jpeg::input object stored in the movie.
	{
		assert(tag_type == 6);

		Uint16	character_id = in->read_u16();

		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->set_id(character_id);

		//
		// Read the image data.
		//
		
		jpeg::input*	j_in = m->get_jpeg_loader();
		assert(j_in);
		j_in->discard_partial_buffer();
		ch->m_image = image::read_swf_jpeg2_with_tables(j_in);
		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 21);
		
		Uint16	character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("define_bits_jpeg2_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		bitmap_character_rgb*	ch = new bitmap_character_rgb();
		ch->set_id(character_id);

		//
		// Read the image data.
		//
		
		ch->m_image = image::read_swf_jpeg2(in->get_underlying_stream());
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
			log_error("error: inflate_wrapper() inflateInit() returned %d\n", err);
			return;
		}

		Uint8	buf[1];

		for (;;) {
			// Fill a one-byte (!) buffer.
			buf[0] = in->read_byte();
			d_stream.next_in = &buf[0];
			d_stream.avail_in = 1;

			err = inflate(&d_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END) break;
			if (err != Z_OK)
			{
				log_error("error: inflate_wrapper() inflate() returned %d\n", err);
			}
		}

		err = inflateEnd(&d_stream);
		if (err != Z_OK)
		{
			log_error("error: inflate_wrapper() inflateEnd() return %d\n", err);
		}
	}


	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m)
	// loads a define_bits_jpeg3 tag. This is a jpeg file with an alpha
	// channel using zlib compression.
	{
		assert(tag_type == 35);

		Uint16	character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("define_bits_jpeg3_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		Uint32	jpeg_size = in->read_u32();
		Uint32	alpha_position = in->get_position() + jpeg_size;

		bitmap_character_rgba*	ch = new bitmap_character_rgba();
		ch->set_id(character_id);

		//
		// Read the image data.
		//
		
		ch->m_image = image::read_swf_jpeg3(in->get_underlying_stream());

		in->set_position(alpha_position);
		
		int	buffer_bytes = ch->m_image->m_width * ch->m_image->m_height;
		Uint8*	buffer = new Uint8[buffer_bytes];

		inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);

		for (int i = 0; i < buffer_bytes; i++)
		{
			ch->m_image->m_data[4*i+3] = buffer[i];
		}

		delete [] buffer;

		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 20 || tag_type == 36);

		Uint16	character_id = in->read_u16();
		Uint8	bitmap_format = in->read_u8();	// 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
		Uint16	width = in->read_u16();
		Uint16	height = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("dbl2l: tag_type = %d, id = %d, fmt = %d, w = %d, h = %d\n",
				tag_type,
				character_id,
				bitmap_format,
				width,
				height));

		if (tag_type == 20)
		{
			// RGB image data.
			bitmap_character_rgb*	ch = new bitmap_character_rgb();
			ch->set_id(character_id);
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

				inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
				assert(in->get_position() <= in->get_tag_end_position());

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

				inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
				assert(in->get_position() <= in->get_tag_end_position());
			
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

				inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
				assert(in->get_position() <= in->get_tag_end_position());
			
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
			ch->set_id(character_id);
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

				inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
				assert(in->get_position() <= in->get_tag_end_position());

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

				inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
				assert(in->get_position() <= in->get_tag_end_position());
			
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

				inflate_wrapper(in->get_underlying_stream(), ch->m_image->m_data, width * height * 4);
				assert(in->get_position() <= in->get_tag_end_position());
			
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


	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 2
		       || tag_type == 22
		       || tag_type == 32);

		Uint16	character_id = in->read_u16();

		shape_character_def*	ch = new shape_character_def;
		ch->set_id(character_id);
		ch->read(in, tag_type, true, m);

		IF_VERBOSE_PARSE(log_msg("shape_loader: id = %d, rect ", character_id);
				 ch->get_bound().print());

		m->add_character(character_id, ch);
	}


	//
	// font loaders
	//


	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load a DefineFont or DefineFont2 tag.
	{
		assert(tag_type == 10 || tag_type == 48);

		Uint16	font_id = in->read_u16();
		
		font*	f = new font;
		f->read(in, tag_type, m);

		m->add_font(font_id, f);

		fontlib::add_font(f);
	}


	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m)
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
		else
		{
			log_error("define_font_info_loader: can't find font w/ id %d\n", font_id);
		}
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
                Uint16 	m_clip_depth;
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
                        m_clip_depth(0),                        
			m_place_type(PLACE)
		{
		}

		~place_object_2()
		{
			delete [] m_name;
			m_name = NULL;
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

				in->read_uint(1);	// reserved flag -- "has actions" in versions >= 5
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
                                
                                if (has_name) {
					m_name = in->read_string();
				}
                                  
				if (has_clip_bracket) {
					m_clip_depth = in->read_u16(); 
					IF_VERBOSE_PARSE(log_msg("HAS CLIP BRACKET!\n"));
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
                                
				//log_msg("place object at depth %i\n", m_depth);

				IF_VERBOSE_PARSE({log_msg("po2r: name = %s\n", m_name ? m_name : "<null>");
					 log_msg("po2r: char id = %d, mat:\n", m_character_id);
					 m_matrix.print();}
					);
			}
		}

		
		void	execute(movie* m)
		// Place/move/whatever our object in the given movie.
		{
			switch (m_place_type)
			{
			case PLACE:
				m->add_display_object(
					m_character_id,
					m_name,
					m_depth,
					m_color_transform,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;

			case MOVE:
				m->move_display_object(
					m_depth,
					m_has_cxform,
					m_color_transform,
					m_has_matrix,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;

			case REPLACE:
			{
				m->replace_display_object(
					m_character_id,
					m_name,
					m_depth,
					m_has_cxform,
					m_color_transform,
					m_has_matrix,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;
			}

			}
		}

		void	execute_state(movie* m)
		{
			execute(m);
		}
	};


	
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 4 || tag_type == 26);

		place_object_2*	ch = new place_object_2;
		ch->read(in, tag_type);

		m->add_execute_tag(ch);
	}


	//
	// sprite_definition
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


	struct sprite_definition : public character_def, public movie_definition_sub
	{
		movie_definition_sub*	m_movie_def;		// parent movie.
		array<array<execute_tag*> >	m_playlist;	// movie control events for each frame.
		string_hash<int>	m_named_frames;
		int	m_frame_count;
		int	m_loading_frame;

		sprite_definition(movie_definition_sub* m)
			:
			m_movie_def(m),
			m_frame_count(0),
			m_loading_frame(0)
		{
			assert(m_movie_def);
		}

		// overloads from movie_definition
		virtual int	get_width() const { return 1; }
		virtual int	get_height() const { return 1; }
		virtual int	get_frame_count() const { return m_frame_count; }
		virtual float	get_frame_rate() const { return m_movie_def->get_frame_rate(); }
		virtual int	get_loading_frame() const { return m_loading_frame; }
		virtual void	add_character(int id, character_def* ch) { log_error("add_character tag appears in sprite tags!\n"); }
		virtual void	add_font(int id, font* ch) { log_error("add_font tag appears in sprite tags!\n"); }
		virtual font*	get_font(int id) { return m_movie_def->get_font(id); }
		virtual void	set_jpeg_loader(jpeg::input* j_in) { assert(0); }
		virtual jpeg::input*	get_jpeg_loader() { return NULL; }
		virtual bitmap_character_def*	get_bitmap_character(int id) { return m_movie_def->get_bitmap_character(id); }
		virtual void	add_bitmap_character(int id, bitmap_character_def* ch) { log_error("add_bc appears in sprite tags!\n"); }
		virtual sound_sample*	get_sound_sample(int id) { return m_movie_def->get_sound_sample(id); }
		virtual void	add_sound_sample(int id, sound_sample* sam) { log_error("add sam appears in sprite tags!\n"); }
		virtual void	export_resource(const tu_string& symbol, resource* res) { log_error("can't export from sprite\n"); }
		virtual resource*	get_exported_resource(const tu_string& sym) { return m_movie_def->get_exported_resource(sym); }
		virtual character_def*	get_character_def(int id) { return m_movie_def->get_character_def(id); }
		virtual void	generate_font_bitmaps() { assert(0); }
		virtual void	output_cached_data(tu_file* out) { assert(0); }
		virtual void	input_cached_data(tu_file* in) { assert(0); }

		movie_interface*	create_instance() { return NULL; }

		// overloads from character_def
		virtual character*	create_character_instance(movie* parent);


		/* sprite_definition */
		virtual void	add_execute_tag(execute_tag* c)
		{
			m_playlist[m_loading_frame].push_back(c);
		}


		/* sprite_definition */
		virtual void	add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
		{
			assert(m_loading_frame >= 0 && m_loading_frame < m_frame_count);

			tu_string	n = name;
			assert(m_named_frames.get(n, NULL) == false);	// frame should not already have a name (?)
			m_named_frames.add(n, m_loading_frame);
		}

		/* sprite_definition */
		bool	get_labeled_frame(const char* label, int* frame_number)
		{
			return m_named_frames.get(label, frame_number);
		}

		const array<execute_tag*>&	get_playlist(int frame_number) { return m_playlist[frame_number]; }


		/* sprite_definition */
		void	read(stream* in)
		// Read the sprite info.  Consists of a series of tags.
		{
			int	tag_end = in->get_tag_end_position();

			m_frame_count = in->read_u16();
			m_playlist.resize(m_frame_count);	// need a playlist for each frame

			IF_VERBOSE_PARSE(log_msg("sprite: frames = %d\n", m_frame_count));

			m_loading_frame = 0;

			while ((Uint32) in->get_position() < (Uint32) tag_end)
			{
				int	tag_type = in->open_tag();
				loader_function lf = NULL;
				if (tag_type == 1)
				{
					// show frame tag -- advance to the next frame.
					m_loading_frame++;
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
					IF_VERBOSE_PARSE(log_msg("*** no tag loader for type %d\n", tag_type));
				}

				in->close_tag();
			}
		}
	};


	struct sprite_instance : public character
	{
		movie_definition_sub*	m_def;
		movie_root*	m_root;

		// @@ Ack, really I want to refer to the data in
		// sprite_definition, if m_def is one.  But it may not
		// be.
		int	m_id;

		display_list	m_display_list;
		array<action_buffer*>	m_action_list;

		play_state	m_play_state;
		int	m_current_frame;
		int	m_next_frame;
		float	m_time_remainder;
		bool	m_update_frame;
		bool	m_has_looped;
		bool	m_accept_anim_moves;	// once we've been moved by ActionScript, don't accept moves from anim tags.

		as_environment	m_as_environment;

		virtual ~sprite_instance()
		{
			m_display_list.clear();
		}

		sprite_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id)
			:
			character(parent),
			m_def(def),
			m_root(r),
			m_id(id),
			m_play_state(PLAY),
			m_current_frame(0),
			m_next_frame(0),
			m_time_remainder(0),
			m_update_frame(true),
			m_has_looped(false),
			m_accept_anim_moves(true)
		{
			assert(m_def);
			assert(m_root);
			m_as_environment.set_target(this);
		}

		movie_root*	get_root() { return m_root; }
		movie*	get_root_movie() { return m_root->get_root_movie(); }

		virtual int	get_id() const { return m_id; }

		movie_definition*	get_movie_definition() { return m_def; }

		int	get_width() { assert(0); return 0; }
		int	get_height() { assert(0); return 0; }

		int	get_current_frame() const { return m_current_frame; }
		int	get_frame_count() const { return m_def->get_frame_count(); }

		void	set_play_state(play_state s)
		// Stop or play the sprite.
		{
			if (m_play_state != s)
			{
				m_time_remainder = 0;
			}

			m_play_state = s;
		}
		play_state	get_play_state() const { return m_play_state; }


		character*	get_character(int character_id)
		{
//			return m_def->get_character_def(character_id);
			// @@ TODO -- look through our dlist for a match
			return NULL;
		}

		float	get_background_alpha() const
		{
			return m_root->get_background_alpha();
		}

		float	get_pixel_scale() const { return m_root->get_pixel_scale(); }

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			m_root->get_mouse_state(x, y, buttons);
		}

		virtual int	get_mouse_capture(void)
		// Use this to retrive the character that has captured the mouse.
		{
			return m_root->get_mouse_capture();
		}

		virtual void	set_mouse_capture(int cid)
		// Set the mouse capture to the given character.
		{
			m_root->set_mouse_capture(cid);
		}

		void	set_background_color(const rgba& color)
		{
			m_root->set_background_color(color);
		}

		float	get_timer() const { return m_root->get_timer(); }

		void	restart()
		{
			m_current_frame = 0;
			m_next_frame = 0;
			m_time_remainder = 0;
			m_update_frame = true;
			m_has_looped = false;
		}

		virtual bool	has_looped() const { return m_has_looped; }

		virtual bool	get_accept_anim_moves() const { return m_accept_anim_moves; }

		/* sprite_instance */
		virtual void	advance(float delta_time)
		{
			assert(m_def && m_root);

			// mouse drag.
			character::do_mouse_drag();

			m_time_remainder += delta_time;
			const float	frame_time = 1.0f / m_root->get_frame_rate();

			// Check for the end of frame
			if (m_time_remainder >= frame_time)
			{
				m_time_remainder -= frame_time;
				m_update_frame = true;
			}

			while (m_update_frame)
			{
				m_update_frame = false;

				// Update current and next frames.
				m_current_frame = m_next_frame;
				m_next_frame = m_current_frame + 1;

				// Execute the current frame's tags.
				if (m_play_state == PLAY) 
				{
					execute_frame_tags(m_current_frame);

					// Dispatch onEnterFrame event.
					on_enter_frame();

					// Perform frame actions
					do_actions();
				}

				m_display_list.update();


				// Advance everything in the display list.
				m_display_list.advance(frame_time);

				// Perform button actions (????)
				do_actions();


				// Update next frame according to actions
				if (m_play_state == STOP)
				{
					m_next_frame = m_current_frame;
					if (m_next_frame >= m_def->get_frame_count())
					{
						m_next_frame = m_def->get_frame_count();
					}
				}
				else if (m_next_frame >= m_def->get_frame_count())	// && m_play_state == PLAY
				{
  					m_next_frame = 0;
					m_has_looped = true;

					/*if (m_def->m_frame_count > 1)
					{
						// Avoid infinite loop on single frame sprites?
						m_update_frame = true;
					}*/
					m_display_list.reset();
				}

				// Check again for the end of frame
				if (m_time_remainder >= frame_time)
				{
					m_time_remainder -= frame_time;
					m_update_frame = true;
				}
			}
		}


		void	execute_frame_tags(int frame, bool state_only = false)
		// Execute the tags associated with the specified frame.
		{
			assert(frame >= 0);
			assert(frame < m_def->get_frame_count());

			const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
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


		void	execute_remove_tags(int frame)
		// Execute any remove-object tags associated with the specified frame.
		{
			assert(frame >= 0);
			assert(frame < m_def->get_frame_count());

			const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (e->is_remove_tag())
				{
					e->execute_state(this);
				}
			}
		}


		void	do_actions()
		// Take care of this frame's actions.
		{
			execute_actions(&m_as_environment, m_action_list);
			m_action_list.resize(0);
		}


		void	goto_frame(int target_frame_number)
		// Set the sprite state at the specified frame number.
		{
//			IF_VERBOSE_DEBUG(log_msg("sprite::goto_frame(%d)\n", target_frame_number));//xxxxx

			/* does STOP need a special case?
			if (m_play_state == STOP)
			{
				target_frame_number++;	// if stopped, update_frame won't increase it
				m_current_frame = target_frame_number;
			}*/

			if (target_frame_number < m_current_frame)
			{
				// Apply any intervening remove-object tags, to clean out undesired
				// objects.
				for (int f = m_current_frame - 1; f > target_frame_number; f--)
				{
					execute_remove_tags(f);
				}
				m_display_list.update();

				// Advance the display list to the target frame.
				m_display_list.reset();
				{for (int f = 0; f < target_frame_number; f++)
				{
					execute_frame_tags(f, true);
				}}
				execute_frame_tags(target_frame_number, false);
				m_display_list.update();
			}
			else if (target_frame_number > m_current_frame)
			{
				for (int f = m_current_frame; f < target_frame_number; f++)
				{
					execute_frame_tags(f, true);
				}
				execute_frame_tags(target_frame_number, false);
				m_display_list.update();
			}


			// Set current and next frames.
			m_current_frame = target_frame_number;
			m_next_frame = target_frame_number + 1;

			// goto_frame stops by default.
			m_play_state = STOP;
		}


		void	goto_labeled_frame(const char* label)
		// Look up the labeled frame, and jump to it.
		{
			int	target_frame = -1;
			if (m_def->get_labeled_frame(label, &target_frame))
			{
				goto_frame(target_frame);
			}
			else
			{
				log_error("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label);
			}
		}

		
		/* sprite_instance */
		void	on_enter_frame()
		// Dispatch onEnterFrame handler, if any.
		{
			static tu_string	s_enter_frame_method_name("onEnterFrame");

			as_value	method;
			if (get_member(s_enter_frame_method_name, &method))
			{
				call_method0(method, &m_as_environment, this);
			}
		}


		void	display()
		{
			m_display_list.display();
		}

		void	add_display_object(
			Uint16 character_id,
			const char* name,
			Uint16 depth,
			const cxform& color_transform,
			const matrix& matrix,
			float ratio,
			Uint16 clip_depth)
		// Add an object to the display list.
		{
			assert(m_def && m_root);

			character_def*	cdef = m_root->get_character_def(character_id);
			if (cdef == NULL)
			{
				log_error("sprite::add_display_object(): unknown cid = %d\n", character_id);
				return;
			}

			// If we already have this object on this
			// plane, then move it instead of replacing
			// it.
			character*	existing_char = m_display_list.get_character_at_depth(depth);
			if (existing_char
			    && existing_char->get_id() == character_id
			    && ((name == NULL && existing_char->get_name().length() == 0)
				|| (name && existing_char->get_name() == name)))
			{
//				IF_VERBOSE_DEBUG(log_msg("add changed to move on depth %d\n", depth));//xxxxxx
				move_display_object(depth, true, color_transform, true, matrix, ratio, clip_depth);
				return;
			}

			assert(cdef);
			character*	ch = cdef->create_character_instance(this);
			assert(ch);
			if (name != NULL && name[0] != 0)
			{
				ch->set_name(name);
			}
			m_display_list.add_display_object(ch, depth, color_transform, matrix, ratio, clip_depth);
		}


		void	move_display_object(
			Uint16 depth,
			bool use_cxform,
			const cxform& color_xform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth)
		// Updates the transform properties of the object at
		// the specified depth.
		{
			//gameswf::move_display_object(&m_display_list, depth, use_cxform, color_xform, use_matrix, mat, ratio);
			m_display_list.move_display_object(depth, use_cxform, color_xform, use_matrix, mat, ratio, clip_depth);
		}


		void	replace_display_object(
			Uint16 character_id,
			const char* name,
			Uint16 depth,
			bool use_cxform,
			const cxform& color_transform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth)
		{
			assert(m_def && m_root);

			character_def*	cdef = m_root->get_character_def(character_id);
			if (cdef == NULL)
			{
				log_error("sprite::replace_display_object(): unknown cid = %d\n", character_id);
				return;
			}
			assert(cdef);

			character*	ch = cdef->create_character_instance(this);
			assert(ch);

			if (name != NULL && name[0] != 0)
			{
				ch->set_name(name);
			}

			m_display_list.replace_display_object(
				ch,
				depth,
				use_cxform,
				color_transform,
				use_matrix,
				mat,
				ratio,
				clip_depth);
		}


		void	remove_display_object(Uint16 depth)
		// Remove the object at the specified depth.
		{
			//gameswf::remove_display_object(&m_display_list, depth);
			m_display_list.remove_display_object(depth);
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
			int	index = m_display_list.get_display_index(depth);
			if (index == -1)
			{
				return -1;
			}

			character*	ch = m_display_list.get_display_object(index).m_character;

			return ch->get_id();
		}


		//
		// ActionScript support
		//


		// don't override set_self_value; it's nonsensical for sprite_instance.


		/* sprite_instance */
		virtual bool	get_self_value(as_value* val)
		// Return a reference to ourself.
		{
			val->set(static_cast<as_object_interface*>(this));
			return true;
		}

		
		/* sprite_instance */
		virtual void	set_member(const tu_string& name, const as_value& val)
		// Set the named member to the value.  Return true if we have
		// that member; false otherwise.
		{
			if (name == "_x")
			{
				matrix	m = get_matrix();
				m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);

				m_accept_anim_moves = false;

				return;
			}
			else if (name == "_y")
			{
				matrix	m = get_matrix();
				m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);

				m_accept_anim_moves = false;

				return;
			}
			else if (name == "_xscale")
			{
//				matrix	m = get_world_matrix();
//				// @@ quick and dirty; test/fix this
//				val->set(m.m_[0][0] * 100);	// percent!
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_yscale")
			{
//				matrix	m = get_world_matrix();
//				// @@ quick and dirty; test/fix this
//				val->set(m.m_[1][1] * 100);	// percent!
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_alpha")
			{
				set_background_alpha((float) val.to_number());
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_visible")
			{
//				val->set(true);	// @@
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_width")
			{
//				matrix	m = get_world_matrix();
//				// @@ run our bounding box through m and return transformed bound...
//				val->set(1.0);
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_height")
			{
//				matrix	m = get_world_matrix();
//				// @@ run our bounding box through m and return transformed bound...
//				val->set(1.0);
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_rotation")
			{
//				// Rotation angle in DEGREES.
//				matrix	m = get_world_matrix();
//				// @@ TODO some trig in here...
//				val->set(0.0);
				m_accept_anim_moves = false;
				return;
			}
			else if (name == "_highquality")
			{
				// @@ global { 0, 1, 2 }
//				// Whether we're in high quality mode or not.
//				val->set(true);
				return;
			}
			else if (name == "_focusrect")
			{
//				// Is a yellow rectangle visible around a focused movie clip (?)
//				val->set(false);
				return;
			}
			else if (name == "_soundbuftime")
			{
				// @@ global
//				// Number of seconds before sound starts to stream.
//				val->set(0.0);
				return;
			}

			// Not a built-in property.  Is it a named character in the display list?
			{
				bool	success = false;
				for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
				{
					character*	ch = m_display_list.get_character(i);
					if (name == ch->get_variable_name())
					{
						success = ch->set_self_value(val) || success;
					}
				}
				if (success) return;
			}

			// If that didn't work, set a variable within this environment.
			m_as_environment.set_member(name, val);
		}


		/* sprite_instance */
		virtual bool	get_member(const tu_string& name, as_value* val)
		// Set *val to the value of the named member and
		// return true, if we have the named member.
		// Otherwise leave *val alone and return false.
		{
			if (name == "_x")
			{
				matrix	m = get_matrix();
				val->set(TWIPS_TO_PIXELS(m.m_[0][2]));
				return true;
			}
			else if (name == "_y")
			{
				matrix	m = get_matrix();
				val->set(TWIPS_TO_PIXELS(m.m_[1][2]));
				return true;
			}
			else if (name == "_xscale")
			{
				matrix	m = get_world_matrix();
				// @@ quick and dirty; test/fix this
				val->set(m.m_[0][0] * 100);	// percent!
				return true;
			}
			else if (name == "_yscale")
			{
				matrix	m = get_world_matrix();
				// @@ quick and dirty; test/fix this
				val->set(m.m_[1][1] * 100);	// percent!
				return true;
			}
			else if (name == "_currentframe")
			{
				val->set(m_current_frame + 1);
				return true;
			}
			else if (name == "_totalframes")
			{
				// number of frames.  Read only.
				val->set(m_def->get_frame_count());
				return true;
			}
			else if (name == "_alpha")
			{
				val->set(get_background_alpha());
				return true;
			}
			else if (name == "_visible")
			{
				val->set(true);	// @@
				return true;
			}
			else if (name == "_width")
			{
				matrix	m = get_world_matrix();
				// @@ run our bounding box through m and return transformed bound...
				val->set(100.0);
				return true;
			}
			else if (name == "_height")
			{
				matrix	m = get_world_matrix();
				// @@ run our bounding box through m and return transformed bound...
				val->set(100.0);
				return true;
			}
			else if (name == "_rotation")
			{
				// Rotation angle in DEGREES.
				matrix	m = get_world_matrix();
				// @@ TODO some trig in here...
				val->set(0.0);
				return true;
			}
			else if (name == "_target")
			{
				// Full path to this object; e.g. "/_level0/sprite1/sprite2/ourSprite"
				val->set("/_root");
				return true;
			}
			else if (name == "_framesloaded")
			{
				val->set(m_def->get_frame_count());
				return true;
			}
			else if (name == "_name")
			{
				val->set(get_name());
				return true;
			}
			else if (name == "_droptarget")
			{
				// Absolute path in slash syntax where we were last dropped (?)
				// @@ TODO
				val->set("/_root");
				return true;
			}
			else if (name == "_url")
			{
				// our URL.
				val->set("gameswf");
				return true;
			}
			else if (name == "_highquality")
			{
				// Whether we're in high quality mode or not.
				val->set(true);
				return true;
			}
			else if (name == "_focusrect")
			{
				// Is a yellow rectangle visible around a focused movie clip (?)
				val->set(false);
				return true;
			}
			else if (name == "_soundbuftime")
			{
				// Number of seconds before sound starts to stream.
				val->set(0.0);
				return true;
			}
			else if (name == "_xmouse")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				assert(m_root);
				m_root->get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;
				
				m.transform_by_inverse(&b, a);

				val->set(TWIPS_TO_PIXELS(b.m_x));
				return true;
			}
			else if (name == "_ymouse")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				assert(m_root);
				m_root->get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;
				
				m.transform_by_inverse(&b, a);

				val->set(TWIPS_TO_PIXELS(b.m_y));
				return true;
			}

			if (name == "_parent")
			{
				val->set(static_cast<as_object_interface*>(m_parent));
				return true;
			}

			// Not a built-in property.  Try named characters in the display list.
			bool	success = false;
			for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
			{
				character*	ch = m_display_list.get_character(i);
				if (name == ch->get_name())
				{
					// Found one.
					if (ch->get_self_value(val))
					{
						return true;
					}
				}
			}

			// Not a built-in or a character.  Try variables.
			if (m_as_environment.get_member(name, val))
			{
				return true;
			}

			return false;
		}


		/* sprite_instance */
		virtual movie*	get_relative_target(const tu_string& name)
		// Find the movie which is one degree removed from us,
		// given the relative pathname.
		//
		// If the pathname is "..", then return our parent.
		// If the pathname is ".", then return ourself.  If
		// the pathname is "_level0" or "_root", then return
		// the root movie.
		//
		// Otherwise, the name should refer to one our our
		// named characters, so we return it.
		//
		// @@ what's the real deal with sprites?  How to
		// distinguish between multiple instances of the same
		// sprite???
		{
			if (name == ".")
			{
				return this;
			}
			else if (name == "..")
			{
				return get_parent();
			}
			else if (name == "_level0"
				 || name == "_root")
			{
				return m_root->m_movie;
			}
			else
			{
				// See if we have a match on the display list.
				for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
				{
					character*	ch = m_display_list.get_character(i);
					if (strcmp(ch->get_name(), name) == 0)
					{
						// Found it.
						return ch;
					}
				}
			}

			return NULL;
		}


		/* sprite_instance */
		virtual void	call_frame_actions(const as_value& frame_spec)
		// Execute the actions for the specified frame.  The
		// frame_spec could be an integer or a string.
		{
			int	frame_number = -1;

			// Figure out what frame to call.
			if (frame_spec.get_type() == as_value::STRING)
			{
				if (m_def->get_labeled_frame(frame_spec.to_string(), &frame_number) == false)
				{
					// Try converting to integer.
					frame_number = (int) frame_spec.to_number();
				}
			}
			else
			{
				frame_number = (int) frame_spec.to_number();
			}

			if (frame_number < 0 || frame_number >= m_def->get_frame_count())
			{
				// No dice.
				log_error("error: call_frame('%s') -- unknown frame\n", frame_spec.to_string());
				return;
			}

			assert(m_action_list.size() == 0);

			// Execute the actions.
			const array<execute_tag*>&	playlist = m_def->get_playlist(frame_number);
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (e->is_action_tag())
				{
					e->execute(this);
				}
				do_actions();
			}
			
		}


		/* sprite_instance */
		virtual void	set_drag_state(const drag_state& st)
		{
			m_root->m_drag_state = st;
		}

		/* sprite_instance */
		virtual void	stop_drag()
		{
			assert(m_parent == NULL);	// we must be the root movie!!!
			
			m_root->stop_drag();
		}


		/* sprite_instance */
		virtual void	get_drag_state(drag_state* st)
		{
			*st = m_root->m_drag_state;
		}
	};


	character*	sprite_definition::create_character_instance(movie* parent)
	// Create a (mutable) instance of our definition.  The
	// instance is created to live (temporarily) on some level on
	// the parent movie's display list.
	{
		sprite_instance*	si = new sprite_instance(this, parent->get_root(), parent, get_id());

		return si;
	}


	movie_interface*	movie_def_impl::create_instance()
	// Create a playable movie instance from a def.
	{
		movie_root*	m = new movie_root(this);
		character*	root_movie = new sprite_instance(this, m, NULL, -1);
		root_movie->set_name("_root");
		m->set_root_movie(root_movie);

		return m;
	}


	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Create and initialize a sprite, and add it to the movie.
	{
		assert(tag_type == 39);
                
		IF_VERBOSE_PARSE(log_msg("sprite\n"));

		int	character_id = in->read_u16();

		sprite_definition*	ch = new sprite_definition(m);	// @@ combine sprite_definition with movie_def_impl
		ch->set_id(character_id);
		ch->read(in);

		IF_VERBOSE_PARSE(log_msg("sprite: char id = %d\n", character_id));

		m->add_character(character_id, ch);
	}



	//
	// end_tag
	//

	// end_tag doesn't actually need to exist.

	void	end_loader(stream* in, int tag_type, movie_definition_sub* m)
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

		virtual void	execute(movie* m)
		{
//			IF_DEBUG(log_msg(" remove: depth %2d\n", m_depth));
			m->remove_display_object(m_depth);
		}

		virtual void	execute_state(movie* m)
		{
			execute(m);
		}

		virtual bool	is_remove_tag() const { return true; }
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 28);

		remove_object_2*	t = new remove_object_2;
		t->read(in);

		IF_VERBOSE_PARSE(log_msg("remove_object_2(%d)\n", t->m_depth));

		m->add_execute_tag(t);
	}


	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 7 || tag_type == 34);

		int	character_id = in->read_u16();

		button_character_definition*	ch = new button_character_definition;
		ch->set_id(character_id);
		ch->read(in, tag_type, m);

		m->add_character(character_id, ch);
	}


	//
	// export
	//


	void	export_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load an export tag (for exposing internal resources of m)
	{
		assert(tag_type == 56);

		int	count = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("export: count = %d\n", count));

		// Read the exports.
		for (int i = 0; i < count; i++)
		{
			Uint16	id = in->read_u16();
			char*	symbol_name = in->read_string();
			IF_VERBOSE_PARSE(log_msg("export: id = %d, name = %s\n", id, symbol_name));

			if (font* f = m->get_font(id))
			{
				// Expose this font for export.
				m->export_resource(tu_string(symbol_name), f);
			}
			else
			{
				log_error("export error: don't know how to export resource '%s'\n",
					  symbol_name);
			}

			delete [] symbol_name;
		}
	}


	//
	// import
	//


	void	import_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load an import tag (for pulling in external resources)
	{
		assert(tag_type == 57);

		char*	source_url = in->read_string();
		int	count = in->read_u16();

		log_msg("import: source_url = %s, count = %d\n", source_url, count);

		// Try to load the source movie into the movie library.
		movie_definition_sub*	source_movie = create_library_movie_sub(source_url);
		if (source_movie == NULL)
		{
			// Give up on imports.
			log_error("can't import movie from url %s\n", source_url);
			return;
		}

		// Get the imports.
		for (int i = 0; i < count; i++)
		{
			Uint16	id = in->read_u16();
			char*	symbol_name = in->read_string();
			log_msg("import: id = %d, name = %s\n", id, symbol_name);

			resource* res = source_movie->get_exported_resource(symbol_name);
			if (res == NULL)
			{
				log_error("import error: resource '%s' is not exported from movie '%s'\n",
					  symbol_name, source_url);
			}
			else if (font* f = res->cast_to_font())
			{
				// Add this shared font to the currently-loading movie.
				m->add_font(id, f);
			}
			else
			{
				log_error("import error: resource '%s' from movie '%s' has unknown type\n",
					  symbol_name, source_url);
			}

			delete [] symbol_name;
		}

		delete [] source_url;
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
