// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include "gameswf_movie.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>


namespace jpeg { struct input; }


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
	class Timer;

	struct sound_sample : public resource //virtual public ref_counted
	{
		virtual sound_sample*	cast_to_sound_sample() { return this; }
	};

	struct stream;
	struct swf_event;

	void save_extern_movie(movie_interface* m);

	// Extra internal interfaces added to movie_definition
	struct movie_definition_sub : public movie_definition
	{
		virtual const array<execute_tag*>&	get_playlist(int frame_number) = 0;
		virtual const array<execute_tag*>*	get_init_actions(int frame_number) = 0;
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& symbol) = 0;
		virtual character_def*	get_character_def(int id) = 0;

		virtual bool	get_labeled_frame(const char* label, int* frame_number) = 0;

		// For use during creation.
		virtual int	get_loading_frame() const = 0;
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
	};


	// For internal use.
	movie_definition_sub*	create_movie_sub(const char* filename);
	movie_definition_sub*	create_library_movie_sub(const char* filename);
	movie_interface*	create_library_movie_inst_sub(movie_definition_sub* md);

	// for extern movies
	movie_interface*	create_library_movie_inst(movie_definition* md);
	movie_interface*	get_current_root();
	void set_current_root(movie_interface* m);
	const char* get_workdir();
	void set_workdir(const char* dir);
	void delete_unused_root();

	struct swf_event
	{
		// NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
		// array<>!  They cannot be moved!  The private
		// operator=(const swf_event&) should help guard
		// against that.

		event_id	m_event;
		action_buffer	m_action_buffer;
		as_value	m_method;

		swf_event()
		{
		}

		void	attach_to(character* ch) const
		{
			ch->set_event_handler(m_event, m_method);
		}

	private:
		// DON'T USE THESE
		swf_event(const swf_event& s) { assert(0); }
		void	operator=(const swf_event& s) { assert(0); }
	};


	// For characters that don't store unusual state in their instances.
	struct generic_character : public character
	{
		character_def*	m_def;

		generic_character(character_def* def, movie* parent, int id)
			:
		character(parent, id),
			m_def(def)
		{
			assert(m_def);
		}

		virtual void	display()
		{
			m_def->display(this);	// pass in transform info
			do_display_callback();
		}

		// @@ tulrich: these are used for finding bounds; TODO
		// need to do this using enclose_transformed_rect(),
		// not by scaling the local height/width!

		virtual float	get_height()
		{
			matrix	m = get_world_matrix();
			float	h = m_def->get_height_local() * m.m_[1][1];
			return h;
		}

		virtual float	get_width()
		{
			matrix	m = get_world_matrix();
			float	w = m_def->get_width_local() * m.m_[0][0];
			return w;
		}

		virtual movie*	get_topmost_mouse_entity(float x, float y)
		{
			assert(get_visible());	// caller should check this.

			matrix	m = get_matrix();
			point	p;
			m.transform_by_inverse(&p, point(x, y));

			if (m_def->point_test_local(p.m_x, p.m_y))
			{
				// The mouse is inside the shape.
				return this;
			}
			return NULL;
		}
	};


	struct bitmap_character_def : public character_def
	{
		virtual gameswf::bitmap_info*	get_bitmap_info() = 0;
	};

#if 1
	// Bitmap character
	struct bitmap_character : public bitmap_character_def
	{
		bitmap_character(bitmap_info* bi)
			:
		m_bitmap_info(bi)
		{
		}

		//		bitmap_character(image::rgb* image)
		//		{
		//			assert(image != 0);

		//			// Create our bitmap info, from our image.
		//			m_bitmap_info = gameswf::render::create_bitmap_info_rgb(image);
		//		}

		//		bitmap_character(image::rgba* image)
		//		{
		//			assert(image != 0);

		//			// Create our bitmap info, from our image.
		//			m_bitmap_info = gameswf::render::create_bitmap_info_rgba(image);
		//		}

		gameswf::bitmap_info*	get_bitmap_info()
		{
			return m_bitmap_info.get_ptr();
		}

	private:
		smart_ptr<gameswf::bitmap_info>	m_bitmap_info;
	};

#endif

	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
		virtual void	execute_state(movie* m) {}
		virtual void	execute_state_reverse(movie* m, int frame) { execute_state(m); }
		virtual bool	is_remove_tag() const { return false; }
		virtual bool	is_action_tag() const { return false; }
		virtual uint32	get_depth_id_of_replace_or_add_tag() const { return static_cast<uint32>(-1); }
	};

	//
	// Helper to generate mouse events, given mouse state & history.
	//

	struct mouse_button_state
	{
		weak_ptr<movie>	m_active_entity;	// entity that currently owns the mouse pointer
		weak_ptr<movie>	m_topmost_entity;	// what's underneath the mouse right now

		bool	m_mouse_button_state_last;		// previous state of mouse button
		bool	m_mouse_button_state_current;		// current state of mouse button

		bool	m_mouse_inside_entity_last;	// whether mouse was inside the active_entity last frame

		mouse_button_state()
			:
			m_mouse_button_state_last(0),
			m_mouse_button_state_current(0),
			m_mouse_inside_entity_last(false)
		{
		}
	};

	void	generate_mouse_button_events(mouse_button_state* ms);

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
		int	m_frame_count;
		int	m_version;
		int	m_loading_frame;
		uint32	m_file_length;

		jpeg::input*	m_jpeg_in;

		movie_def_impl(create_bitmaps_flag cbf, create_font_shapes_flag cfs);
		~movie_def_impl();

		movie_interface*	movie_def_impl::create_instance();

		int	get_frame_count() const;
		float	get_frame_rate() const;
		float	get_width_pixels() const;
		float	get_height_pixels() const;
		virtual int	get_version() const;
		virtual int	get_loading_frame() const;
		uint32	get_file_bytes() const;
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
		void	get_owned_fonts(array<font*>* fonts);

		void	generate_font_bitmaps();
		void	output_cached_data(tu_file* out, const cache_options& options);
		void	input_cached_data(tu_file* in);


	};

	//
	// Loader callbacks.
	//

	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	typedef void (*loader_function)(stream* input, int tag_type, movie_definition_sub* m);
	void	register_tag_loader(int tag_type, loader_function lf);


	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	end_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	export_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	import_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	start_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	// sound_stream_loader();	// head, head2, block
	void	define_video_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	video_loader(stream* in, int tag_type, movie_definition_sub* m);

}	// end namespace gameswf


#endif // GAMESWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
