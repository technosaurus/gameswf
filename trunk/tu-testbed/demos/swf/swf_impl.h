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

	// SWF movies contain "characters" to represent the various elements.
	struct character
	{
		virtual ~character() {}
		virtual void	display(const display_info& di) {}	// renderer state contains context
	};


	struct movie : public movie_interface
	{
		virtual void	add_character(int id, character* ch) = 0;
		virtual void	add_execute_tag(execute_tag* c) = 0;

		virtual void	add_display_object(Uint16 character_id,
						   Uint16 depth,
						   const cxform& color_transform,
						   const matrix& mat,
						   float ratio)
		{
		}
	};


	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
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
		int	get_tag_end_position();
		int	open_tag();
		void	close_tag();
	};


	struct movie_container;

	void	set_background_color_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m);
	void	define_shape_loader(stream* in, int tag_type, movie* m);
	void	place_object_2_loader(stream* in, int tag_type, movie* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie* m);
	void	sprite_loader(stream* in, int tag_type, movie* m);
	void	end_loader(stream* in, int tag_type, movie* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie* m);
};


#endif // SWF_IMPL_H
