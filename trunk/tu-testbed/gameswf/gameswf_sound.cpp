// gameswf_sound.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to handle SWF sound-related tags.


#include "gameswf_stream.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"


namespace gameswf
{
	// Callback interface to host, for handling sounds.  If it's NULL,
	// sound is ignored.
	static sound_handler*	s_sound_handler = 0;


	void	set_sound_handler(sound_handler* s)
	// Called by host, to set a handler for all sounds.
	// Can pass in 0 to disable sound.
	{
		s_sound_handler = s;
	}


	struct sound_sample
	{
		int	m_sound_handler_id;

		sound_sample(int id)
			:
			m_sound_handler_id(id)
		{
		}
	};


	void	define_sound_loader(stream* in, int tag_type, movie* m)
	// Load a DefineSound tag.
	{
		assert(tag_type == 14);

		Uint16	character_id = in->read_u16();

		sound_handler::format_type	format = (sound_handler::format_type) in->read_uint(4);
		int	sample_rate = in->read_uint(2);	// multiples of 5512.5
		bool	sample_16bit = in->read_uint(1) ? true : false;
		bool	stereo = in->read_uint(1) ? true : false;
		int	sample_count = in->read_u32();

		static int	s_sample_rate_table[] = { 5512, 11025, 22050, 44100 };

		IF_VERBOSE_PARSE(log_msg("define sound: ch=%d, format=%d, rate=%d, 16=%d, stereo=%d, ct=%d\n",
					 character_id, int(format), sample_rate, int(sample_16bit), int(stereo), sample_count));

		// If we have a sound_handler, ask it to init this sound.
		if (s_sound_handler)
		{
			// @@ This is pretty awful -- lots of copying, slow reading.
			int	data_bytes = in->get_tag_end_position() - in->get_position();
			unsigned char*	data = new unsigned char[data_bytes];
			for (int i = 0; i < data_bytes; i++) { data[i] = in->read_u8(); }
			
			int	handler_id = s_sound_handler->create_sound(
				data,
				data_bytes,
				sample_count,
				format,
				s_sample_rate_table[sample_rate],
				stereo);
//			sound_sample*	sam = new sound_sample(handler_id);
//			m->add_sound(character_id, sam);

			delete [] data;
		}
	}


	void	start_sound_loader(stream* in, int tag_type, movie* m)
	// Load a StartSound tag.
	{
	}


// @@ currently not implemented
// 	void	sound_stream_loader(stream* in, int tag_type, movie* m)
// 	// Load the various stream-related tags: SoundStreamHead,
// 	// SoundStreamHead2, SoundStreamBlock.
// 	{
// 	}


	//
	// ADPCM
	//


	// Data from Alexis' SWF reference
	static int	s_index_update_table_2bits[2] = { -1,  2 };
	static int	s_index_update_table_3bits[4] = { -1, -1,  2,  4 };
	static int	s_index_update_table_4bits[8] = { -1, -1, -1, -1,  2,  4,  6,  8 };
	static int	s_index_update_table_5bits[16] = { -1, -1, -1, -1, -1, -1, -1, -1, 1,  2,  4,  6,  8, 10, 13, 16 };

	static int*	s_index_update_tables[4] = {
		s_index_update_table_2bits,
		s_index_update_table_3bits,
		s_index_update_table_4bits,
		s_index_update_table_5bits,
	};

	// Data from Jansen.  http://homepages.cwi.nl/~jack/
	// Check out his Dutch retro punk songs, heh heh :)
	const int STEPSIZE_CT = 89;
	static int s_stepsize[STEPSIZE_CT] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};


	// Algo from http://www.circuitcellar.com/pastissues/articles/richey110/text.htm
	// And also Jansen.
	// Here's another reference: http://www.geocities.com/SiliconValley/8682/aud3.txt
	// Original IMA spec doesn't seem to be on the web :(


	template<int n_bits> void do_sample(int& sample, int& stepsize_index, int raw_code)
	// Core of ADPCM -- generate a single sample based on previous
	// sample and compressed input sample.
	//
	// sample and stepsize_index are in/out params.  raw_code is the
	// N-bit compressed input sample.
	{
		assert(raw_code >= 0 && raw_code < (1 << n_bits));

		static const int	HI_BIT = (1 << (n_bits - 1));
		int*	index_update_table = s_index_update_tables[n_bits - 2];

		// Core of ADPCM.

		int	code_mag = raw_code & (HI_BIT - 1);
		bool	code_sign_bit = (raw_code & HI_BIT) ? 1 : 0;
		int	mag = (code_mag << 1) + 1;	// shift in an LSB (they do this so that pos & neg zero are different)

		int	stepsize = s_stepsize[stepsize_index];

		// Compute the new sample.  It's the predicted value
		// (i.e. the previous value), plus a delta.  The delta
		// comes from the code times the stepsize.  going for
		// something like: delta = stepsize * (code * 2 + 1) >> code_bits
		int	delta = (stepsize * mag) >> (n_bits - 1);
		if (code_sign_bit) delta = -delta;

		sample += delta;
		sample = iclamp(sample, -32768, 32767);

		// Update our stepsize index.  Use a lookup table.
		stepsize_index += index_update_table[raw_code];
		stepsize_index = iclamp(stepsize_index, 0, STEPSIZE_CT - 1);
	}


	template<int n_bits>
	int	extract_bits(const unsigned char*& in_data, int& current_bits, int& unused_bits)
	// Get an n_bits integer from the input stream.  current_bits is a
	// temp repository for the next few bits of input.  unused_bits
	// tells us how many bits of current_bits are valid.
	{
		if (unused_bits < n_bits)
		{
			// Get more data.
			current_bits |= (*in_data) << unused_bits;
			in_data++;
			unused_bits += 8;
		}

		int	result = current_bits & ((1 << n_bits) - 1);
		current_bits >>= n_bits;
		unused_bits -= n_bits;

		return result;
	}


	template<int n_bits>
	void do_mono_block(Sint16* out_data, const unsigned char* in_data, int sample, int stepsize_index)
	// Uncompress 4096 mono samples of ADPCM.
	{
		int	sample_count = 4096;
		int	current_bits = 0;
		int	unused_bits = 0;

		while (sample_count--)
		{
			*out_data++ = (Sint16) sample;
			int	raw_code = extract_bits<n_bits>(in_data, current_bits, unused_bits);
			do_sample<n_bits>(sample, stepsize_index, raw_code);	// sample & stepsize_index are in/out params
		}
	}


	template<int n_bits>
	void do_stereo_block(
		Sint16* out_data,
		const unsigned char* in_data,
		int left_sample,
		int left_stepsize_index,
		int right_sample,
		int right_stepsize_index
		)
	// Uncompress 4096 stereo sample pairs of ADPCM.
	{
		int	sample_count = 4096;
		int	current_bits = 0;
		int	unused_bits = 0;

		while (sample_count--)
		{
			*out_data++ = (Sint16) left_sample;
			int	left_raw_code = extract_bits<n_bits>(in_data, current_bits, unused_bits);
			do_sample<n_bits>(left_sample, left_stepsize_index, left_raw_code);

			*out_data++ = (Sint16) right_sample;
			int	right_raw_code = extract_bits<n_bits>(in_data, current_bits, unused_bits);
			do_sample<n_bits>(right_sample, right_stepsize_index, right_raw_code);
		}
	}


	void	adpcm_expand(
		void* out_data_void,
		const void* in_data_void,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo)
	// Utility function: uncompress ADPCM data from in_data[] to
	// out_data[].  The output buffer must have (sample_count*2)
	// bytes for mono, or (sample_count*4) bytes for stereo.
	{
		int	blocks = sample_count >> 12;	// 4096 samples per block!
		int	extra_samples = sample_count & ((1 << 12) - 1);

		Sint16*	out_data = (Sint16*) out_data_void;

		const unsigned char*	in_data = (const unsigned char*) in_data_void;

		// Read header.
		int	n_bits = (*in_data++) & 3 + 2;	// 2 to 5 bits

		while (blocks--)
		{
			// Read initial sample & index values.
			int	sample = (*in_data++);
			sample += (*in_data++) << 8;
			if (sample & 0x08000) { sample |= ~0x07FFF; }	// sign-extend

			int	stepsize_index = (*in_data++);
			stepsize_index = iclamp(stepsize_index, 0, STEPSIZE_CT - 1);

			if (stereo == false)
			{
				switch (n_bits) {
				default: assert(0); break;
				case 2: do_mono_block<2>(out_data, in_data, sample, stepsize_index); break;
				case 3: do_mono_block<3>(out_data, in_data, sample, stepsize_index); break;
				case 4: do_mono_block<4>(out_data, in_data, sample, stepsize_index); break;
				case 5: do_mono_block<5>(out_data, in_data, sample, stepsize_index); break;
				}
				out_data += 4096;
			}
			else
			{
				// Stereo.

				// Got values for left channel; now get initial sample
				// & index for right channel.
				int	right_sample = (*in_data++);
				right_sample += (*in_data++) << 8;
				if (right_sample & 0x08000) { right_sample |= ~0x07FFF; }	// sign-extend
				
				int	right_stepsize_index = (*in_data++);
				right_stepsize_index = iclamp(right_stepsize_index, 0, STEPSIZE_CT - 1);

				switch (n_bits) {
				default: assert(0); break;
				case 2: do_stereo_block<2>(out_data, in_data, sample, stepsize_index, right_sample, right_stepsize_index); break;
				case 3: do_stereo_block<3>(out_data, in_data, sample, stepsize_index, right_sample, right_stepsize_index); break;
				case 4: do_stereo_block<4>(out_data, in_data, sample, stepsize_index, right_sample, right_stepsize_index); break;
				case 5: do_stereo_block<5>(out_data, in_data, sample, stepsize_index, right_sample, right_stepsize_index); break;
				}
				out_data += 4096 * 2;
			}
		}

		if (extra_samples)
		{
			log_error(
				"ADPCM buffer should have a multiple of 4096 samples.  Padding %d samples with 0's\n",
				extra_samples);
			int	extra_bytes = extra_samples * (stereo ? 4 : 2);
			memset(out_data, 0, extra_bytes);
		}
	}


};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
