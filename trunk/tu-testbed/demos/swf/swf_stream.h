// swf_stream.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A class to handle reading the elements of swf files.


#ifndef SWF_STREAM_H
#define SWF_STREAM_H


#include "engine/container.h"
#include "SDL.h"


namespace swf
{
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
		Uint32	read_u32();
		Sint32	read_s32();

		// For null-terminated string.
		char*	read_string();	// reads *and new[]'s* the string -- ownership passes to caller!

		// For string that begins with an 8-bit length code.
		char*	read_string_with_length();	// reads *and new[]'s* the string -- ownership passes to caller!

		int	get_position();
		void	set_position(int pos);
		int	get_tag_end_position();
		int	open_tag();
		void	close_tag();
	};


};	// end namespace swf


#endif // SWF_STREAM_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
