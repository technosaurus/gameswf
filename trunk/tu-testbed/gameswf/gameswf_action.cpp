// gameswf_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_stream.h"


namespace gameswf
{
	//
	// action stuff
	//


	void	action_buffer::read(stream* in)
	{
		// Read action bytes.
		for (;;)
		{
			int	action_id = in->read_u8();
			m_buffer.push_back(action_id);
			if (action_id == 0)
			{
				// end of action buffer.
				return;
			}
			if (action_id & 0x80)
			{
				// Action contains extra data.  Read it.
				int	length = in->read_u16();
				m_buffer.push_back(length & 0x0FF);
				m_buffer.push_back((length >> 8) & 0x0FF);
				IF_DEBUG(log_msg("action: 0x%02X[%d]", action_id, length));
				for (int i = 0; i < length; i++)
				{
					unsigned char	b = in->read_u8();
					m_buffer.push_back(b);
					IF_DEBUG(log_msg(" 0x%02X", b));
				}
				IF_DEBUG(log_msg("\n"));
			}
			else
			{
				IF_DEBUG(log_msg("action: 0x%02X\n", action_id));
			}
		}
	}


	void	action_buffer::execute(movie* m)
	// Interpret the actions in this action buffer, and apply them
	// to the given movie.
	{
		assert(m);

		movie*	original_movie = m;

		// Avoid warnings.
		original_movie = original_movie;

		for (int pc = 0; pc < m_buffer.size(); )
		{
			int	action_id = m_buffer[pc];
			if ((action_id & 0x80) == 0)
			{
				IF_DEBUG(log_msg("aid = 0x%02x\n", action_id));

				// Simple action; no extra data.
				switch (action_id)
				{
				default:
					break;

				case 0x00:	// end of actions.
					return;

				case 0x04:	// next frame.
					m->goto_frame(m->get_current_frame() + 1);
					break;

				case 0x05:	// prev frame.
					m->goto_frame(m->get_current_frame() - 1);
					break;

				case 0x06:	// action play
					m->set_play_state(movie::PLAY);
					break;

				case 0x07:	// action stop
					m->set_play_state(movie::STOP);
					break;

				case 0x08:	// toggle quality
				case 0x09:	// stop sounds
				case 0x0A:	// add
				case 0x0B:	// subtract
				case 0x0C:	// multiply
				case 0x0D:	// divide
				case 0x0E:	// equal
				case 0x0F:	// less than
				case 0x10:	// logical and
				case 0x11:	// logical or
				case 0x12:	// logical not
				case 0x13:	// string equal
				case 0x14:	// string length
				case 0x15:	// substring
				case 0x18:	// int
				case 0x1C:	// eval
				case 0x1D:	// set variable
				case 0x20:	// set target expression
				case 0x21:	// string concat
				case 0x22:	// get property
				case 0x23:	// set property
				case 0x24:	// duplicate clip (sprite?)
				case 0x25:	// remove clip
				case 0x26:	// trace
				case 0x27:	// start drag movie
				case 0x28:	// stop drag movie
				case 0x29:	// string less than
				case 0x30:	// random
				case 0x31:	// mb length
				case 0x32:	// ord
				case 0x33:	// chr
				case 0x34:	// get timer
				case 0x35:	// mb substring
				case 0x37:	// mb chr
					break;
				}
				pc++;	// advance to next action.
			}
			else
			{
				IF_DEBUG(log_msg("aid = 0x%02x ", action_id));

				// Action containing extra data.
				int	length = m_buffer[pc + 1] | (m_buffer[pc + 2] << 8);
				int	next_pc = pc + length + 3;

				IF_DEBUG({for (int i = 0; i < imin(length, 8); i++) { log_msg("0x%02x ", m_buffer[pc + 3 + i]); } log_msg("\n"); });
				
				switch (action_id)
				{
				default:
					break;

				case 0x81:	// goto frame
				{
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					m->goto_frame(frame);
					break;
				}

				case 0x83:	// get url
				{
					// @@ TODO should call back into client app, probably...

					// Print as text.
					IF_DEBUG({
						for (int i = 0; i < length; i++) { log_msg("%c", m_buffer[pc + 3 + i]); } log_msg("\n");
					});

					break;
				}

				case 0x8A:	// wait for frame
				{
					// @@ TODO I think this has to deal with incremental loading
					break;
				}

				case 0x8B:	// set target
				{
					// Change the movie we're working on.
					// @@ TODO
					// char* target_name = &m_buffer[pc + 3];
					// if (target_name[0] == 0) { m = original_movie; }
					// else {
					//	m = m->find_labeled_target(target_name);
					//	if (m == NULL) m = original_movie;
					// }
					break;
				}

				case 0x8C:	// go to labeled frame
				{
					// char*	frame_label = (char*) &m_buffer[pc + 3];
					// m->goto_labeled_frame(frame_label);
					break;
				}

				case 0x8D:	// wait for frame expression (?)
				case 0x96:	// push data
				case 0x99:	// branch always (goto)
				case 0x9A:	// get url 2
				case 0x9D:	// branch if true
				case 0x9E:	// call frame
				case 0x9F:	// goto expression (?)
					break;
				
				}
				pc = next_pc;
			}
		}
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
