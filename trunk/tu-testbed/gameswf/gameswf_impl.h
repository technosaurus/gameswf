// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"


namespace jpeg { struct input; }


namespace gameswf
{
	struct stream;
	struct display_info;
	struct font;
	struct action_buffer;
	struct bitmap_character;
	struct sound_sample { virtual ~sound_sample() {} };
        struct bitmap_info;

	struct movie : public movie_interface
	{
		virtual void	export_resource(const tu_string& symbol, resource* res) {}
		virtual resource*	get_exported_resource(const tu_string& symbol)
		{
			return NULL;
		}

		virtual float	get_pixel_scale() const { return 1.0f; }
		virtual void	add_character(int id, character* ch) {}
		virtual character*	get_character(int id) { return NULL; }
		virtual void	add_font(int id, font* ch) {}
		virtual font*	get_font(int id) { return NULL; }
		virtual void	add_execute_tag(execute_tag* c) {}
		virtual void	add_frame_name(const char* name) {}

		virtual void	set_jpeg_loader(jpeg::input* j_in) {}
		virtual jpeg::input*	get_jpeg_loader() { return NULL; }

		virtual bitmap_character*	get_bitmap_character(int character_id) { return 0; }
		virtual void	add_bitmap_character(int character_id, bitmap_character* ch) {}

		virtual sound_sample*	get_sound_sample(int character_id) { return 0; }
		virtual void	add_sound_sample(int character_id, sound_sample* sam) {}

		virtual void	add_display_object(Uint16 character_id,
						   Uint16 depth,
						   const cxform& color_transform,
						   const matrix& mat,
						   float ratio,
                                                   Uint16 clip_depth)
		{
		}

		virtual void	move_display_object(Uint16 depth,
						    bool use_cxform,
						    const cxform& color_transform,
						    bool use_matrix,
						    const matrix& mat,
						    float ratio,
                                                    Uint16 clip_depth)
		{
		}

		virtual void	replace_display_object(Uint16 character_id,
						       Uint16 depth,
						       bool use_cxform,
						       const cxform& color_transform,
						       bool use_matrix,
						       const matrix& mat,
						       float ratio,
                                                       Uint16 clip_depth)
		{
		}

		virtual void	remove_display_object(Uint16 depth)
		{
		}

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }

		virtual void	set_background_color(const rgba& color)
		{
		}

		virtual void	set_background_alpha(float alpha)
		{
		}

		virtual float	get_background_alpha() const
		{
			return 1.0f;
		}

		virtual void	set_display_viewport(int x0, int y0, int width, int height)
		{
		}

		virtual void	goto_frame(int target_frame_number) { assert(0); }

		enum play_state
		{
			PLAY,
			STOP
		};
		virtual void	set_play_state(play_state s)
		{
		}

		virtual void	notify_mouse_state(int x, int y, int buttons)
		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		{
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		// Use this to retrieve the last state of the mouse, as set via
		// notify_mouse_state().
		{
			assert(0);
		}

		virtual int	get_mouse_capture(void)
		// Use this to retrive the character that has the mouse focus.
		{
			assert(0);
			return -1;
		}

		virtual void	set_mouse_capture(int cid)
		// Set the focus to the given character.
		{
			assert(0);
		}

		virtual bool	set_edit_text(const char* var_name, const char* new_text)
		{
			assert(0);
			return false;
		}
	};


	// SWF movies contain "characters" to represent the various elements.
	struct character : public movie
	{
	private:
		int	m_id;
		tu_string	m_name;
		
	public:
		character()
			:
			m_id(-1)
		{
		}

		virtual ~character() {}

		void	set_id(int id) { m_id = id; }
		int	get_id() const { return m_id; }

		void	set_name(const char* name) { m_name = name; }
		const char*	get_name() const { return m_name.c_str(); }

		virtual void	display(const display_info& di) {}	// renderer state contains context
		virtual void	advance(float delta_time, movie* m, const matrix& mat) {}	// for buttons and sprites
		virtual bool	point_test(float x, float y) { return false; }	// return true if the point is inside our shape.

		// Movie interfaces.  By default do nothing.  sprite will override these.
		int	get_width() { return 0; }
		int	get_height() { return 0; }
		int	get_current_frame() const { assert(0); return 0; }
		int	get_frame_count() const { assert(0); return 0; }
		void	restart() { /*assert(0);*/ }
		void	advance(float delta_time) { assert(0); }	// should only be called on movie_impl's.
		void	goto_frame(int target_frame) {}
		void	display() {}

		// Instance/definition interface; useful for things
		// like sprites, which have an immutable definition
		// which lives in the character table, and possibly
		// more than one mutable instance which contain
		// individual state and live in the display list.
		//
		// Ordinary characters are totally immutable, so don't
		// need the extra instance/def logic.
		virtual bool	is_definition() const { return false; }
		virtual bool	is_instance() const { return false; }
		virtual character*	create_instance() { assert(0); return 0; }
		virtual character*	get_definition() { return this; }

		// An interface designed to be used by
		// edit_text_character, or any other character that
		// can reasonably accept a text value.
		// Return true on success.
		virtual bool	set_edit_text(const char* new_text) { assert(0); return false; }
	};


	struct bitmap_character : public character
	{
		virtual gameswf::bitmap_info*	get_bitmap_info() = 0;
	};


	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
		virtual void	execute_state(movie* m) {}
	};


	struct movie_container;


	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie* m);
	void	set_background_color_loader(stream* in, int tag_type, movie* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie* m);
	void	define_shape_loader(stream* in, int tag_type, movie* m);
	void	define_font_loader(stream* in, int tag_type, movie* m);
	void	define_font_info_loader(stream* in, int tag_type, movie* m);
	void	define_text_loader(stream* in, int tag_type, movie* m);
	void	define_edit_text_loader(stream* in, int tag_type, movie* m);
	void	place_object_2_loader(stream* in, int tag_type, movie* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m);
	void	sprite_loader(stream* in, int tag_type, movie* m);
	void	end_loader(stream* in, int tag_type, movie* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie* m);
	void	do_action_loader(stream* in, int tag_type, movie* m);
	void	button_character_loader(stream* in, int tag_type, movie* m);
	void	frame_label_loader(stream* in, int tag_type, movie* m);
	void	export_loader(stream* in, int tag_type, movie* m);
	void	import_loader(stream* in, int tag_type, movie* m);
	void	define_sound_loader(stream* in, int tag_type, movie* m);
	void	start_sound_loader(stream* in, int tag_type, movie* m);
	// sound_stream_loader();	// head, head2, block


	struct texture_glyph;
	namespace fontlib
	{
		// For adding fonts.
		void	add_font(font* f);

		// For drawing a textured glyph w/ current render transforms.
		void	draw_glyph(const matrix& m, const texture_glyph* g, rgba color);

		// Return the pixel height of text, such that the
		// texture glyphs are sampled 1-to-1 texels-to-pixels.
		// I.e. the height of the glyph box, in texels.
		float	get_nominal_texture_glyph_height();
	}


	// Information about how to display an element.
	struct display_info
	{
		movie*	m_movie;
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
		int	m_display_number;
                Uint16 	m_clip_depth;

		display_info()
			:
			m_movie(NULL),
			m_depth(0),
			m_ratio(0.0f),
			m_display_number(0),
                        m_clip_depth(0)
		{
		}

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			m_depth = di.m_depth;
			m_color_transform.concatenate(di.m_color_transform);
			m_matrix.concatenate(di.m_matrix);
			m_ratio = di.m_ratio;
                        m_clip_depth = di.m_clip_depth;
		}
	};

}	// end namespace gameswf


#endif // GAMESWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
