// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf.h"
#include "gameswf_types.h"
#include "SDL.h"
#include <assert.h>
#include "engine/container.h"
#include "engine/utility.h"


namespace jpeg { struct input; }


namespace gameswf
{
	struct stream;
	struct display_info;
	struct font;
	struct action_buffer;
	struct bitmap_character;
	namespace render { struct bitmap_info; }


	struct movie : public movie_interface
	{
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

		virtual void	add_display_object(Uint16 character_id,
						   Uint16 depth,
						   const cxform& color_transform,
						   const matrix& mat,
						   float ratio)
		{
		}

		virtual void	move_display_object(Uint16 depth,
						    bool use_cxform,
						    const cxform& color_transform,
						    bool use_matrix,
						    const matrix& mat,
						    float ratio)
		{
		}

		virtual void	replace_display_object(Uint16 character_id,
						       Uint16 depth,
						       bool use_cxform,
						       const cxform& color_transform,
						       bool use_matrix,
						       const matrix& mat,
						       float ratio)
		{
		}

		virtual void	remove_display_object(Uint16 depth)
		{
		}

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }

		virtual void	set_background_color(const rgba& bg_color)
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
	};


	// SWF movies contain "characters" to represent the various elements.
	struct character : public movie
	{
		int	m_id;
		
		character()
			:
			m_id(-1)
		{
		}

		virtual ~character() {}
		virtual void	display(const display_info& di) {}	// renderer state contains context
		virtual void	advance(float delta_time, movie* m, const matrix& mat) {}	// for buttons and sprites
		virtual bool	point_test(float x, float y) { return false; }	// return true if the point is inside our shape.

		// Movie interfaces.  By default do nothing.  sprite will override these.
		int	get_width() { return 0; }
		int	get_height() { return 0; }
		int	get_current_frame() const { assert(0); return 0; }
		int	get_frame_count() const { assert(0); return 0; }
		void	restart() { assert(0); }
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
	};


	struct bitmap_character : public character
	{
		virtual gameswf::render::bitmap_info*	get_bitmap_info() = 0;
	};


	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
	};


	struct movie_container;


	// Factory functions and some operations for opaque-ish types.
	struct shape_character;
	shape_character*	create_shape_character(stream* in, int tag_type, bool with_style, movie* m);
	void	delete_shape_character(shape_character* sh);
	void	display_shape_character_in_white(const shape_character* sh);
	void	get_shape_bounds(rect* result, const shape_character* sh);

	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie* m);
	void	set_background_color_loader(stream* in, int tag_type, movie* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m);
	void	define_shape_loader(stream* in, int tag_type, movie* m);
	void	define_font_loader(stream* in, int tag_type, movie* m);
	void	define_font_info_loader(stream* in, int tag_type, movie* m);
	void	define_text_loader(stream* in, int tag_type, movie* m);
	void	place_object_2_loader(stream* in, int tag_type, movie* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m);
	void	sprite_loader(stream* in, int tag_type, movie* m);
	void	end_loader(stream* in, int tag_type, movie* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie* m);
	void	do_action_loader(stream* in, int tag_type, movie* m);
	void	button_character_loader(stream* in, int tag_type, movie* m);
	void	frame_label_loader(stream* in, int tag_type, movie* m);


	struct texture_glyph;
	namespace fontlib
	{
		// For adding fonts.
		void	add_font(font* f);

		// For drawing a textured glyph w/ current render transforms.
		void	draw_glyph(const texture_glyph* g, rgba color);
	}


	// Information about how to display an element.
	//
	// @@ this type might be useless... if not, it might belong
	// @@ somewhere else.  Hm.
	struct display_info
	{
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;

		display_info()
			:
			m_depth(0),
			m_ratio(0.0f)
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
