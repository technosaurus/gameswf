// swf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation details for SWF parser.


#ifndef SWF_IMPL_H
#define SWF_IMPL_H


#include "swf.h"
#include "swf_types.h"
#include "SDL.h"
#include <assert.h>
#include "engine/container.h"
#include "engine/utility.h"


namespace swf
{
	struct stream;
	struct display_info;
	struct font;
	struct action_buffer;

	struct movie : public movie_interface
	{
		virtual void	add_character(int id, character* ch) {}
		virtual void	add_font(int id, font* ch) {}
		virtual font*	get_font(int id) { return NULL; }
		virtual void	add_execute_tag(execute_tag* c) {}

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

		// Movie interfaces.  By default do nothing.  sprite will override these.
		int	get_width() { return 0; }
		int	get_height() { return 0; }
		int	get_current_frame() const { assert(0); return 0; }
		void	restart() { assert(0); }
		void	advance(float delta_time) {}
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


	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
	};


	struct movie_container;

	void	set_background_color_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m);
	void	define_shape_loader(stream* in, int tag_type, movie* m);
	void	define_font_loader(stream* in, int tag_type, movie* m);
	void	define_text_loader(stream* in, int tag_type, movie* m);
	void	place_object_2_loader(stream* in, int tag_type, movie* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m);
	void	sprite_loader(stream* in, int tag_type, movie* m);
	void	end_loader(stream* in, int tag_type, movie* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie* m);
	void	do_action_loader(stream* in, int tag_type, movie* m);
};


#endif // SWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
