// swf_stream.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF stream wrapper class.


#include "swf_impl.h"


namespace swf
{
	
	stream::stream(SDL_RWops* input)
		:
		m_input(input),
		m_current_byte(0),
		m_unused_bits(0)
	{
	}


	int	stream::read_uint(int bitcount)
	// Reads a bit-packed unsigned integer from the stream
	// and returns it.  The given bitcount determines the
	// number of bits to read.
	{
		assert(bitcount <= 32 && bitcount > 0);
			
		Uint32	value = 0;

		int	bits_needed = bitcount;
		while (bits_needed > 0)
		{
			if (m_unused_bits) {
				if (bits_needed >= m_unused_bits) {
					// Consume all the unused bits.
					value |= (m_current_byte << (bits_needed - m_unused_bits));

					bits_needed -= m_unused_bits;

					m_current_byte = 0;
					m_unused_bits = 0;

				} else {
					// Consume some of the unused bits.
					value |= (m_current_byte >> (m_unused_bits - bits_needed));

					// mask off the bits we consumed.
					m_current_byte &= ((1 << (m_unused_bits - bits_needed)) - 1);

					m_unused_bits -= bits_needed;

						// We're done.
					bits_needed = 0;
				}
			} else {
				m_current_byte = ReadByte(m_input);
				m_unused_bits = 8;
			}
		}

		assert(bits_needed == 0);

		return value;
	}


	int	stream::read_sint(int bitcount)
	// Reads a bit-packed little-endian signed integer
	// from the stream.  The given bitcount determines the
	// number of bits to read.
	{
		assert(bitcount <= 32 && bitcount > 0);

		Sint32	value = (Sint32) read_uint(bitcount);

		// Sign extend...
		if (value & (1 << (bitcount - 1))) {
			value |= -1 << bitcount;
		}

//		IF_DEBUG(printf("stream::read_sint(%d) == %d\n", bitcount, value));

		return value;
	}


	float	stream::read_fixed()
	{
		m_unused_bits = 0;
		Sint32	val = SDL_ReadLE32(m_input);
		return (float) val / 65536.0f;
	}

	void	stream::align() { m_unused_bits = 0; m_current_byte = 0; }

	Uint8	stream::read_u8() { align(); return ReadByte(m_input); }
	Sint8	stream::read_s8() { align(); return ReadByte(m_input); }
	Uint16	stream::read_u16()
	{
		align();
//		IF_DEBUG(printf("filepos = %d ", SDL_RWtell(m_input)));
		int	val = SDL_ReadLE16(m_input);
//		IF_DEBUG(printf("val = 0x%X\n", val));
		return val;
	}
	Sint16	stream::read_s16() { align(); return SDL_ReadLE16(m_input); }


	char*	stream::read_string()
	// Reads *and new[]'s* the string from the given file.
	// Ownership passes to the caller; caller must delete[] the
	// string when it is done with it.
	{
		align();

		array<char>	buffer;
		char	c;
		while ((c = read_u8()) != 0)
		{
			buffer.push_back(c);
		}

		if (buffer.size() == 0)
		{
			return NULL;
		}

		char*	retval = new char[buffer.size() + 1];
		strcpy(retval, &buffer[0]);

		return retval;
	}


	int	stream::get_position()
	// Return our current (byte) position in the input stream.
	{
		return SDL_RWtell(m_input);
	}


	int	stream::get_tag_end_position()
	// Return the file position of the end of the current tag.
	{
		assert(m_tag_stack.size() > 0);

		return m_tag_stack.back();
	}


	int	stream::open_tag()
	// Return the tag type.
	{
		align();
		int	tag_header = read_u16();
		int	tag_type = tag_header >> 6;
		int	tag_length = tag_header & 0x3F;
		assert(m_unused_bits == 0);
		if (tag_length == 0x3F) {
			tag_length = SDL_ReadLE32(m_input);
		}

		IF_DEBUG(printf("tag type = %d, tag length = %d\n", tag_type, tag_length));
			
		// Remember where the end of the tag is, so we can
		// fast-forward past it when we're done reading it.
		m_tag_stack.push_back(get_position() + tag_length);

		return tag_type;
	}


	void	stream::close_tag()
	// Seek to the end of the most-recently-opened tag.
	{
		assert(m_tag_stack.size() > 0);
		int	end_pos = m_tag_stack.pop_back();
		SDL_RWseek(m_input, end_pos, SEEK_SET);

		m_unused_bits = 0;
	}

}; // end namespace swf
	
