// gameswf_movie_def.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_MOVIE_DEF_IMPL_H
#define GAMESWF_MOVIE_DEF_IMPL_H


#include "gameswf/gameswf.h"
#include "gameswf/gameswf_mutex.h"
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"

namespace gameswf
{

	struct action_buffer;
	struct bitmap_character_def;
	struct bitmap_info;
	struct character;
	struct character_def;
	struct display_info;
	struct execute_tag;
	struct font;
	struct movie_root;
	struct movie_definition_sub;

	
	int movie_def_loader(void* arg);

	typedef void (*loader_function)(stream* input, int tag_type, movie_definition_sub* m);
	bool get_tag_loader(int tag_type, loader_function* lf);

	bool get_break_loading();

	// Extra internal interfaces added to movie_definition
	struct movie_definition_sub : public movie_definition 
	{
		movie_definition_sub() :
			m_frame_count(0),
			m_loading_frame(0),
			m_ss_id(-1),
			m_ss_format(sound_handler::FORMAT_RAW),
			m_ss_start(-1)
		{
		}

		~movie_definition_sub()
		{
			sound_handler* sound = get_sound_handler();
			if (sound)
			{
				sound->delete_sound(m_ss_id);
			}
		}

		void	wait_frame(int frame)
		{
			while (frame > m_loading_frame - 1)
			{
//				printf("wait for frame %d, loaded %d\n", frame + 1, m_loading_frame);
				m_frame.wait();
			}
		}

		inline int	get_loading_frame() const
		{
			assert(m_loading_frame >= 0 && m_loading_frame < m_frame_count);
			return m_loading_frame; 
		}
		void	inc_loading_frame() 
		{ 
			m_loading_frame++;
			signal();
		}

		inline int	get_frame_count() const { return m_frame_count; }
		void	set_frame_count(int frames)
		{
			m_frame_count = frames;

			// ALEX: some SWF files have been seen that have 0-frame sprites.
			// The Macromedia player behaves as if they have 1 frame.
			if (m_frame_count < 1)
			{
				m_frame_count = 1;
			}
		}

		virtual const array<execute_tag*>&	get_playlist(int frame_number) = 0;
		virtual const array<execute_tag*>*	get_init_actions(int frame_number) = 0;
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& symbol) = 0;
		virtual character_def*	get_character_def(int id) = 0;

		virtual bool	get_labeled_frame(const char* label, int* frame_number) = 0;

		// For use during creation.
		virtual void	add_character(int id, character_def* ch) = 0;
		virtual void	add_font(int id, font* ch) = 0;
		virtual font*	get_font(int id) = 0;
		virtual void	add_execute_tag(execute_tag* c) = 0;
		virtual void	add_init_action(int sprite_id, execute_tag* c) = 0;
		virtual void	add_frame_name(const char* name) = 0;
		virtual void	set_jpeg_loader(jpeg::input* j_in) = 0;
		virtual jpeg::input*	get_jpeg_loader() = 0;
		virtual bitmap_character_def*	get_bitmap_character(int character_id) = 0;
		virtual void	add_bitmap_character(int character_id, bitmap_character_def* ch) = 0;
		virtual sound_sample*	get_sound_sample(int character_id) = 0;
		virtual void	add_sound_sample(int character_id, sound_sample* sam) = 0;
		virtual void	export_resource(const tu_string& symbol, resource* res) = 0;
		virtual void	add_import(const char* source_url, int id, const char* symbol_name) = 0;
		virtual void	add_bitmap_info(bitmap_info* ch) = 0;

		virtual create_bitmaps_flag	get_create_bitmaps() const = 0;
		virtual create_font_shapes_flag	get_create_font_shapes() const = 0;

		// stream sound variables
		int m_ss_id;	// stream sound handler_id
		sound_handler::format_type m_ss_format;	// used in tag 19
		int m_ss_start;	// start frame

		private:

		int	m_frame_count;
		int	m_loading_frame;
		tu_condition m_frame;

		void	signal()
		{
			// printf("signal %d frames is loaded\n", m_loading_frame);
			m_frame.signal();
		}


	};

	//
	// Helper for movie_def_impl
	//
	struct import_info
	{
		tu_string	m_source_url;
		int		m_character_id;
		tu_string	m_symbol;

		import_info():
		m_character_id(-1)
		{
		}

		import_info(const char* source, int id, const char* symbol):
			m_source_url(source),
			m_character_id(id),
			m_symbol(symbol)
		{
		}
	};


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
		hash<int, smart_ptr<character_def> >	m_characters;
		hash<int, smart_ptr<font> >	 m_fonts;
		hash<int, smart_ptr<bitmap_character_def> >	m_bitmap_characters;
		hash<int, smart_ptr<sound_sample> >	m_sound_samples;
		array<array<execute_tag*> >	   m_playlist;	// A list of movie control events for each frame.
		array<array<execute_tag*> >	   m_init_action_list;	// Init actions for each frame.
		stringi_hash<int>		   m_named_frames;	// 0-based frame #'s
		stringi_hash<smart_ptr<resource> > m_exports;

		// Items we import.
		array<import_info>	m_imports;

		// Movies we import from; hold a ref on these, to keep them alive
		array<smart_ptr<movie_definition> >	m_import_source_movies;

		// Bitmaps used in this movie; collected in one place to make
		// it possible for the host to manage them as textures.
		array<smart_ptr<bitmap_info> >	m_bitmap_list;

		create_bitmaps_flag	m_create_bitmaps;
		create_font_shapes_flag	m_create_font_shapes;

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_version;

		uint32	m_file_length;
		volatile uint32	m_loaded_length;

		jpeg::input*	m_jpeg_in;

		stream*	m_str;
		Uint32	m_file_end_pos;
		tu_file*	m_zlib_in;
		tu_file*	m_origin_in;
		tu_thread* m_thread;

		movie_def_impl(create_bitmaps_flag cbf, create_font_shapes_flag cfs);
		~movie_def_impl();

		movie_interface*	create_instance();

		float	get_frame_rate() const;
		float	get_width_pixels() const;
		float	get_height_pixels() const;
		virtual int	get_version() const;
		uint32	get_file_bytes() const;
		uint32	get_loaded_bytes() const;
		virtual create_bitmaps_flag	get_create_bitmaps() const;
		virtual create_font_shapes_flag	get_create_font_shapes() const;
		virtual void	add_bitmap_info(bitmap_info* bi);
		virtual int	get_bitmap_info_count() const;
		virtual bitmap_info*	get_bitmap_info(int i) const;
		virtual void	export_resource(const tu_string& symbol, resource* res);
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& symbol);
		virtual void	add_import(const char* source_url, int id, const char* symbol);
		bool	in_import_table(int character_id);
		virtual void	visit_imported_movies(import_visitor* visitor);
		virtual void	resolve_import(const char* source_url, movie_definition* source_movie);
		void	add_character(int character_id, character_def* c);
		character_def*	get_character_def(int character_id);
		bool	get_labeled_frame(const char* label, int* frame_number);
		void	add_font(int font_id, font* f);
		font*	get_font(int font_id);
		bitmap_character_def*	get_bitmap_character(int character_id);
		void	add_bitmap_character(int character_id, bitmap_character_def* ch);
		sound_sample*	get_sound_sample(int character_id);
		virtual void	add_sound_sample(int character_id, sound_sample* sam);
		void	add_execute_tag(execute_tag* e);
		void	add_init_action(int sprite_id, execute_tag* e);
		void	add_frame_name(const char* name);

		void	set_jpeg_loader(jpeg::input* j_in);
		jpeg::input*	get_jpeg_loader();

		virtual const array<execute_tag*>&	get_playlist(int frame_number);

		virtual const array<execute_tag*>*	get_init_actions(int frame_number);
		void	read(tu_file* in);
		void	read_tags();
		void	get_owned_fonts(array<font*>* fonts);

		void	generate_font_bitmaps();
		void	output_cached_data(tu_file* out, const cache_options& options);
		void	input_cached_data(tu_file* in);

		virtual movie_def_impl* cast_to_movie_def_impl() { return this; }

	};

}

#endif