// swf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation details for SWF parser.


#ifndef SWF_IMPL_H
#define SWF_IMPL_H


#include "swf.h"
#include "SDL.h"
#include <assert.h>
#include "engine/container.h"
#include "engine/utility.h"


// ifdef this out to get rid of debug spew.
#define IF_DEBUG(exp) exp


#define TWIPS_TO_PIXELS(x)	((x) / 20.f)


namespace swf
{
	struct display_info;
	struct cxform;
	struct matrix;
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
											const cxform& color_transform,
											const matrix& mat,
											float ratio)
		{
		}

		virtual void	remove_display_object(Uint16 depth)
		{
		}

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }
	};


	// SWF movies contain "characters" to represent the various elements.
	struct character : public movie
	{
		virtual ~character() {}
		virtual void	display(const display_info& di) {}	// renderer state contains context

		// Movie interfaces.  By default do nothing.  sprite will override these.
		int	get_width() { return 0; }
		int	get_height() { return 0; }
		virtual void	restart() { assert(0); }
		virtual void	advance(float delta_time) {}
		virtual void	display() {}

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


	// stream is used to encapsulate bit-packed file reads.
	struct stream
	{
		SDL_RWops*	m_input;
		Uint8	m_current_byte;
		Uint8	m_unused_bits;

		array<int>	m_tag_stack;	// position of end of tag


		stream(SDL_RWops* input);
		int	read_uint(int bitcount);
		int	read_sint(int bitcount);
		float	read_fixed();
		void	align();

		Uint8	read_u8();
		Sint8	read_s8();
		Uint16	read_u16();
		Sint16	read_s16();
		char*	read_string();	// reads *and new[]'s* the string -- ownership passes to caller!

		int	get_position();
		void	set_position(int pos);
		int	get_tag_end_position();
		int	open_tag();
		void	close_tag();
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
