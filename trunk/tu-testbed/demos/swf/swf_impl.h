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
	// SWF movies contain "characters" to represent the various elements.
	struct character
	{
		virtual void	execute(float time) {}	// OpenGL state contains viewport, transforms, etc.
	};

	// Control tags include things like setting the background
	// color, etc.
	struct control_tag
	{
		virtual void	execute(float time) {}
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
		int	get_position();
		int	open_tag();
		void	close_tag();
	};


	void	set_background_color_loader(stream* in, int tag_type, movie* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie* m);
	void	define_shape_loader(stream* in, int tag_type, movie* m);
};

#endif // SWF_IMPL_H
