// gameswf_movie_def.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

// @@ Need to break this file into pieces


#include "base/tu_file.h"
#include "gameswf/gameswf_font.h"
#include "gameswf/gameswf_fontlib.h"
#include "gameswf/gameswf_stream.h"
#include "base/jpeg.h"
#include "base/zlib_adapter.h"

#if TU_CONFIG_LINK_TO_ZLIB
#include <zlib.h>
#endif // TU_CONFIG_LINK_TO_ZLIB


namespace gameswf
{

	// it's running in loader thread
	int movie_def_loader(void* arg)
	{
		movie_def_impl* m = (movie_def_impl*) arg;
		m->read_tags();
		return 0;
	}

	// Keep a table of loader functions for the different tag types.
	static hash<int, loader_function>	s_tag_loaders;

	void clears_tag_loaders()
	{
		s_tag_loaders.clear();
	}

	bool get_tag_loader(int tag_type, loader_function* lf)
	{
		return s_tag_loaders.get(tag_type, lf);
	}

	// it's used when user has pressed Esc button to break the loading of the .swf file
	static bool s_break_loading = false;
	bool get_break_loading()
	{
		return s_break_loading;
	}

	void	register_tag_loader(int tag_type, loader_function lf)
		// Associate the specified tag type with the given tag loader
		// function.
	{
		assert(s_tag_loaders.get(tag_type, NULL) == false);
		assert(lf != NULL);

		s_tag_loaders.add(tag_type, lf);
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

	movie_def_impl::movie_def_impl(create_bitmaps_flag cbf, create_font_shapes_flag cfs)
		:
	m_create_bitmaps(cbf),
		m_create_font_shapes(cfs),
		m_frame_rate(30.0f),
		m_version(0),
		m_loaded_length(0),
		m_jpeg_in(0),
		m_str(NULL),
		m_file_end_pos(0),
		m_zlib_in(NULL),
		m_origin_in(NULL),
		m_thread(NULL)
	{
	}

	movie_def_impl::~movie_def_impl()
	{

		// terminate thread
		// it is used when user has pressed Esc button
		s_break_loading = true;
		if (m_thread)
		{
			m_thread->wait();
			delete m_thread;
		}

		// Release our playlist data.
		{for (int i = 0, n = m_playlist.size(); i < n; i++)
		{
			for (int j = 0, m = m_playlist[i].size(); j < m; j++)
			{
				delete m_playlist[i][j];
			}
		}}

		// Release init action data.
		{for (int i = 0, n = m_init_action_list.size(); i < n; i++)
		{
			for (int j = 0, m = m_init_action_list[i].size(); j < m; j++)
			{
				delete m_init_action_list[i][j];
			}
		}}

		assert(m_jpeg_in == NULL);	// It's supposed to be cleaned up in read()
	}

	// ...
	float	movie_def_impl::get_frame_rate() const { return m_frame_rate; }
	float	movie_def_impl::get_width_pixels() const { return ceilf(TWIPS_TO_PIXELS(m_frame_size.width())); }
	float	movie_def_impl::get_height_pixels() const { return ceilf(TWIPS_TO_PIXELS(m_frame_size.height())); }

	int	movie_def_impl::get_version() const { return m_version; }
	uint32	movie_def_impl::get_file_bytes() const { return m_file_length; }
	uint32	movie_def_impl::get_loaded_bytes() const { return m_loaded_length; }

	/* movie_def_impl */
	create_bitmaps_flag	movie_def_impl::get_create_bitmaps() const
		// Returns DO_CREATE_BITMAPS if we're supposed to
		// initialize our bitmap infos, or DO_NOT_INIT_BITMAPS
		// if we're supposed to create blank placeholder
		// bitmaps (to be init'd later explicitly by the host
		// program).
	{
		return m_create_bitmaps;
	}

	/* movie_def_impl */
	create_font_shapes_flag	movie_def_impl::get_create_font_shapes() const
		// Returns DO_LOAD_FONT_SHAPES if we're supposed to
		// initialize our font shape info, or
		// DO_NOT_LOAD_FONT_SHAPES if we're supposed to not
		// create any (vector) font glyph shapes, and instead
		// rely on precached textured fonts glyphs.
	{
		return m_create_font_shapes;
	}

	void	movie_def_impl::add_bitmap_info(bitmap_info* bi)
	// All bitmap_info's used by this movie should be
	// registered with this API.
	{
		m_bitmap_list.push_back(bi);
	}


	int	movie_def_impl::get_bitmap_info_count() const { return m_bitmap_list.size(); }
	bitmap_info*	movie_def_impl::get_bitmap_info(int i) const
	{
		return m_bitmap_list[i].get_ptr();
	}

	void	movie_def_impl::export_resource(const tu_string& symbol, resource* res)
	// Expose one of our resources under the given symbol,
	// for export.	Other movies can import it.
	{
		// SWF sometimes exports the same thing more than once!
		m_exports.set(symbol, res);
	}

	smart_ptr<resource>	movie_def_impl::get_exported_resource(const tu_string& symbol)
	// Get the named exported resource, if we expose it.
	// Otherwise return NULL.
	{
		smart_ptr<resource>	res;
		m_exports.get(symbol, &res);
		return res;
	}

	void	movie_def_impl::add_import(const char* source_url, int id, const char* symbol)
	// Adds an entry to a table of resources that need to
	// be imported from other movies.  Client code must
	// call resolve_import() later, when the source movie
	// has been loaded, so that the actual resource can be
	// used.
	{
		assert(in_import_table(id) == false);

		m_imports.push_back(import_info(source_url, id, symbol));
	}

	bool	movie_def_impl::in_import_table(int character_id)
	// Debug helper; returns true if the given
	// character_id is listed in the import table.
	{
		for (int i = 0, n = m_imports.size(); i < n; i++)
		{
			if (m_imports[i].m_character_id == character_id)
			{
				return true;
			}
		}
		return false;
	}

	void	movie_def_impl::visit_imported_movies(import_visitor* visitor)
	// Calls back the visitor for each movie that we
	// import symbols from.
	{
		stringi_hash<bool>	visited;	// ugh!

		for (int i = 0, n = m_imports.size(); i < n; i++)
		{
			import_info&	inf = m_imports[i];
			if (visited.find(inf.m_source_url) == visited.end())
			{
				// Call back the visitor.
				visitor->visit(inf.m_source_url.c_str());
				visited.set(inf.m_source_url, true);
			}
		}
	}

	void	movie_def_impl::resolve_import(const char* source_url, movie_definition* source_movie)
		// Grabs the stuff we want from the source movie.
	{
		// @@ should be safe, but how can we verify
		// it?	Compare a member function pointer, or
		// something?
		movie_def_impl*	def_impl = static_cast<movie_def_impl*>(source_movie);
		movie_definition_sub*	def = static_cast<movie_definition_sub*>(def_impl);

		// Iterate in reverse, since we remove stuff along the way.
		for (int i = m_imports.size() - 1; i >= 0; i--)
		{
			const import_info&	inf = m_imports[i];
			if (inf.m_source_url == source_url)
			{
				// Do the import.
				smart_ptr<resource> res = def->get_exported_resource(inf.m_symbol);
				bool	 imported = true;

				if (res == NULL)
				{
					log_error("import error: resource '%s' is not exported from movie '%s'\n",
						inf.m_symbol.c_str(), source_url);
				}
				else if (font* f = res->cast_to_font())
				{
					// Add this shared font to our fonts.
					add_font(inf.m_character_id, f);
					imported = true;
				}
				else if (character_def* ch = res->cast_to_character_def())
				{
					// Add this character to our characters.
					add_character(inf.m_character_id, ch);
					imported = true;
				}
				else
				{
					log_error("import error: resource '%s' from movie '%s' has unknown type\n",
						inf.m_symbol.c_str(), source_url);
				}

				if (imported)
				{
					m_imports.remove(i);

					// Hold a ref, to keep this source movie_definition alive.
					m_import_source_movies.push_back(source_movie);
				}
			}
		}
	}

	void	movie_def_impl::add_character(int character_id, character_def* c)
	{
		assert(c);
		m_characters.add(character_id, c);
	}

	character_def*	movie_def_impl::get_character_def(int character_id)
	{
#ifndef NDEBUG
		// make sure character_id is resolved
		if (in_import_table(character_id))
		{
			log_error("get_character_def(): character_id %d is still waiting to be imported\n",
				character_id);
		}
#endif // not NDEBUG

		smart_ptr<character_def>	ch;
		m_characters.get(character_id, &ch);
		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get_ptr();
	}

	bool	movie_def_impl::get_labeled_frame(const char* label, int* frame_number)
		// Returns 0-based frame #
	{
		return m_named_frames.get(label, frame_number);
	}

	void	movie_def_impl::add_font(int font_id, font* f)
	{
		assert(f);
		m_fonts.add(font_id, f);
	}

	font*	movie_def_impl::get_font(int font_id)
	{
#ifndef NDEBUG
		// make sure font_id is resolved
		if (in_import_table(font_id))
		{
			log_error("get_font(): font_id %d is still waiting to be imported\n",
				font_id);
		}
#endif // not NDEBUG

		smart_ptr<font>	f;
		m_fonts.get(font_id, &f);
		assert(f == NULL || f->get_ref_count() > 1);
		return f.get_ptr();
	}

	bitmap_character_def*	movie_def_impl::get_bitmap_character(int character_id)
	{
		smart_ptr<bitmap_character_def>	ch;
		m_bitmap_characters.get(character_id, &ch);
		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get_ptr();
	}

	void	movie_def_impl::add_bitmap_character(int character_id, bitmap_character_def* ch)
	{
		assert(ch);
		m_bitmap_characters.add(character_id, ch);

		add_bitmap_info(ch->get_bitmap_info());
	}

	sound_sample*	movie_def_impl::get_sound_sample(int character_id)
	{
		smart_ptr<sound_sample>	ch;
		m_sound_samples.get(character_id, &ch);
		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get_ptr();
	}

	void	movie_def_impl::add_sound_sample(int character_id, sound_sample* sam)
	{
		assert(sam);
		m_sound_samples.add(character_id, sam);
	}

	void	movie_def_impl::add_execute_tag(execute_tag* e)
	{
		assert(e);
		m_playlist[get_loading_frame()].push_back(e);
	}

	void	movie_def_impl::add_init_action(int sprite_id, execute_tag* e)
		// Need to execute the given tag before entering the
		// currently-loading frame for the first time.
		//
		// @@ AFAIK, the sprite_id is totally pointless -- correct?
	{
		assert(e);
		m_init_action_list[get_loading_frame()].push_back(e);
	}

	void	movie_def_impl::add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.	A copy of the name string is made and
		// kept in this object.
	{
		tu_string	n = name;
		assert(m_named_frames.get(n, NULL) == false);	// frame should not already have a name (?)
		m_named_frames.add(n, get_loading_frame());	// stores 0-based frame #
	}

	void	movie_def_impl::set_jpeg_loader(jpeg::input* j_in)
		// Set an input object for later loading DefineBits
		// images (JPEG images without the table info).
	{
		assert(m_jpeg_in == NULL);
		m_jpeg_in = j_in;
	}

	jpeg::input*	movie_def_impl::get_jpeg_loader()
		// Get the jpeg input loader, to load a DefineBits
		// image (one without table info).
	{
		return m_jpeg_in;
	}


	const array<execute_tag*>&	movie_def_impl::get_playlist(int frame_number) { return m_playlist[frame_number]; }

	const array<execute_tag*>*	movie_def_impl::get_init_actions(int frame_number)
	{
		return &m_init_action_list[frame_number];
	}

	/* movie_def_impl */
	void	movie_def_impl::read(tu_file* in)
		// Read a .SWF movie.
	{
		m_origin_in = in;
		Uint32	file_start_pos = in->get_position();
		Uint32	header = in->read_le32();
		m_file_length = in->read_le32();
		m_file_end_pos = file_start_pos + m_file_length;

		m_version = (header >> 24) & 255;
		if ((header & 0x0FFFFFF) != 0x00535746
			&& (header & 0x0FFFFFF) != 0x00535743)
		{
			// ERROR
			log_error("gameswf::movie_def_impl::read() -- file does not start with a SWF header!\n");
			return;
		}
		bool	compressed = (header & 255) == 'C';

		IF_VERBOSE_PARSE(log_msg("version = %d, file_length = %d\n", m_version, m_file_length));

		m_zlib_in = NULL;
		if (compressed)
		{
#if TU_CONFIG_LINK_TO_ZLIB == 0
			log_error("movie_def_impl::read(): unable to read zipped SWF data; TU_CONFIG_LINK_TO_ZLIB is 0\n");
			return;
#endif

			IF_VERBOSE_PARSE(log_msg("file is compressed.\n"));

			// Uncompress the input as we read it.
			in = zlib_adapter::make_inflater(in);
			m_zlib_in = in;

			// Subtract the size of the 8-byte header, since
			// it's not included in the compressed
			// stream length.
			m_file_end_pos = m_file_length - 8;
		}

		m_str = new stream(in);

		m_frame_size.read(m_str);
		m_frame_rate = m_str->read_u16() / 256.0f;

		set_frame_count(m_str->read_u16());

		m_playlist.resize(get_frame_count());
		m_init_action_list.resize(get_frame_count());

		IF_VERBOSE_PARSE(m_frame_size.print());
		IF_VERBOSE_PARSE(log_msg("frame rate = %f, frames = %d\n", m_frame_rate, get_frame_count()));

		m_thread = new tu_thread(movie_def_loader, this);
//		read_tags();
	}

	// is running in loader thread
	void	movie_def_impl::read_tags()
	{

		while ((Uint32) m_str->get_position() < m_file_end_pos && get_break_loading() == false)
		{
			int	tag_type = m_str->open_tag();
			loader_function	lf = NULL;
			//IF_VERBOSE_PARSE(log_msg("tag_type = %d\n", tag_type));
			if (tag_type == 1)
			{
				// show frame tag -- advance to the next frame.
				IF_VERBOSE_PARSE(log_msg("  show_frame\n"));
				inc_loading_frame();
			}
			else
			if (s_tag_loaders.get(tag_type, &lf))
			{
				// call the tag loader.	 The tag loader should add
				// characters or tags to the movie data structure.
				(*lf)(m_str, tag_type, this);

			}
			else
			{
				// no tag loader for this tag type.
				log_msg("*** no tag loader for type %d\n", tag_type);
				dump_tag_bytes(m_str);
			}

			m_str->close_tag();

			if (tag_type == 0)
			{
				if ((unsigned int) m_str->get_position() != m_file_end_pos)
				{
					// Safety break, so we don't read past the end of the
					// movie.
					log_msg("warning: hit stream-end tag, but not at the "
						"end of the file yet; stopping for safety\n");
					break;
				}
			}

			// 8 is (file_start_pos(4 bytes) + header(4 bytes))
			m_loaded_length = m_str->get_position() + 8;

		}

		if (m_jpeg_in)
		{
			delete m_jpeg_in;
			m_jpeg_in = NULL;
		}

		if (m_zlib_in)
		{
			// Done with the zlib_adapter.
			delete m_zlib_in;
		}

		delete m_str;
		delete m_origin_in;
	}


	void	movie_def_impl::get_owned_fonts(array<font*>* fonts)
		// Fill up *fonts with fonts that we own.
	{
		assert(fonts);
		fonts->resize(0);

		array<int>	font_ids;

		for (hash<int, smart_ptr<font> >::iterator it = m_fonts.begin();
			it != m_fonts.end();
			++it)
		{
			font*	f = it->second.get_ptr();
			if (f->get_owning_movie() == this)
			{
				// Sort by character id, so the ordering is
				// consistent for cache read/write.
				int	id = it->first;

				// Insert in correct place.
				int	insert;
				for (insert = 0; insert < font_ids.size(); insert++)
				{
					if (font_ids[insert] > id)
					{
						// We want to insert here.
						break;
					}
				}
				fonts->insert(insert, f);
				font_ids.insert(insert, id);
			}
		}
	}


	/* movie_def_impl */
	void	movie_def_impl::generate_font_bitmaps()
		// Generate bitmaps for our fonts, if necessary.
	{
		// Collect list of fonts.
		array<font*>	fonts;
		get_owned_fonts(&fonts);
		fontlib::generate_font_bitmaps(fonts, this);
	}


	// Increment this when the cache data format changes.
#define CACHE_FILE_VERSION 6


	/* movie_def_impl */
	void	movie_def_impl::output_cached_data(tu_file* out, const cache_options& options)
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
		fontlib::output_cached_data(out, fonts, this, options);

		// Write character data.
		{for (hash<int, smart_ptr<character_def> >::iterator it = m_characters.begin();
		it != m_characters.end();
		++it)
		{
			out->write_le16(it->first);
			it->second->output_cached_data(out, options);
		}}

		out->write_le16((Sint16) -1);	// end of characters marker
	}


	/* movie_def_impl */
	void	movie_def_impl::input_cached_data(tu_file* in)
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
		fontlib::input_cached_data(in, fonts, this);

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

			smart_ptr<character_def> ch;
			m_characters.get(id, &ch);
			if (ch != NULL)
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
}
