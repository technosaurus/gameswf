// gameswf_button.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF buttons.  Mouse-sensitive update/display, actions, etc.


#include "gameswf_action.h"
#include "gameswf_button.h"
#include "gameswf_render.h"
#include "gameswf_stream.h"


namespace gameswf
{
	
	struct button_character_instance : public character
	{
		button_character_definition*	m_def;
		array<character *>	m_record_character;

		enum mouse_flags
		{
			IDLE = 0,
			FLAG_OVER = 1,
			FLAG_DOWN = 2,
			OVER_DOWN = FLAG_OVER|FLAG_DOWN,

			// aliases
			OVER_UP = FLAG_OVER,
			OUT_DOWN = FLAG_DOWN
		};
		int	m_last_mouse_flags, m_mouse_flags;

		enum mouse_state
		{
			UP = 0,
			DOWN,
			OVER
		};
		mouse_state m_mouse_state;

		button_character_instance(button_character_definition* def)
			:
			m_def(def),
			m_last_mouse_flags(IDLE),
			m_mouse_flags(IDLE),
			m_mouse_state(UP)
		{
			assert(m_def);
			set_id(def->get_id());

			int r, r_num =  m_def->m_button_records.size();
			m_record_character.resize(r_num);

			for (r = 0; r < r_num; r++)
			{
				character * ch = m_def->m_button_records[r].m_character;
				if (ch->is_definition())
				{
					m_record_character[r] = ch->create_instance();
					m_record_character[r]->restart();
				}
				else 
				{
					m_record_character[r] = ch;
				}
			}
		}

		~button_character_instance()
		{
			int r, r_num =  m_record_character.size();
			for (r = 0; r < r_num; r++)
			{
				if (m_record_character[r]->is_instance())
				{
					delete m_record_character[r];
				}
			}
		}

		bool	is_instance() const { return true; }

		void	restart()
		{
			m_last_mouse_flags = IDLE;
			m_mouse_flags = IDLE;
			m_mouse_state = UP;
			int r, r_num =  m_record_character.size();
			for (r = 0; r < r_num; r++)
			{
				if (m_record_character[r]->is_instance())
				{
					m_record_character[r]->restart();
				}
			}
		}


		void	advance(float delta_time, movie* m, const matrix& mat)
		{
			assert(m);

			// Get current mouse capture.
			int id = m->get_mouse_capture();

			// update state if no mouse capture or we have the capture.
			//if (id == -1 || (!m_def->m_menu && id == get_id()))
			if (id == -1 || id == get_id())
			{
				update_state(m, mat);
			}


			// Advance our relevant characters.
			{for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				button_record&	rec = m_def->m_button_records[i];
				if (m_record_character[i] == NULL)
				{
					continue;
				}

				matrix	sub_matrix = mat;
				sub_matrix.concatenate(rec.m_button_matrix);
				if (m_mouse_state == UP)
				{
					if (rec.m_up)
					{
						m_record_character[i]->advance(delta_time, m, sub_matrix);
					}
				}
				else if (m_mouse_state == DOWN)
				{
					if (rec.m_down)
					{
						m_record_character[i]->advance(delta_time, m, sub_matrix);
					}
				}
				else if (m_mouse_state == OVER)
				{
					if (rec.m_over)
					{
						m_record_character[i]->advance(delta_time, m, sub_matrix);
					}
				}
			}}
		}

		void	display(const display_info& di)
		{
			for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				button_record&	rec = m_def->m_button_records[i];
				if (m_record_character[i] == NULL)
				{
					continue;
				}
				if ((m_mouse_state == UP && rec.m_up)
				    || (m_mouse_state == DOWN && rec.m_down)
				    || (m_mouse_state == OVER && rec.m_over))
				{
					display_info	sub_di = di;
					sub_di.m_matrix.concatenate(rec.m_button_matrix);
					sub_di.m_color_transform.concatenate(rec.m_button_cxform);
					m_record_character[i]->display(sub_di);
				}
			}
		}

		inline int	transition(int a, int b) const
		// Combine the flags to avoid a conditional. It would be faster with a macro.
		{
			return (a << 2) | b;
		}

		void	update_state(movie* m, const matrix& mat) 
		// Update button state.
		{

			// Look at the mouse state, and figure out our button state.  We want to
			// know if the mouse is hovering over us, and whether it's clicking on us.
			int	mx, my, mbuttons;
			m->get_mouse_state(&mx, &my, &mbuttons);

			m_last_mouse_flags = m_mouse_flags;
			m_mouse_flags = 0;

			// Find the mouse position in button-space.
			point	mouse_position;
			mat.transform_by_inverse(&mouse_position, point(PIXELS_TO_TWIPS(mx), PIXELS_TO_TWIPS(my)));

			{for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				button_record&	rec = m_def->m_button_records[i];
				if (rec.m_character == NULL
				    || rec.m_hit_test == false)
				{
					continue;
				}

				// Find the mouse position in character-space.
				point	sub_mouse_position;
				rec.m_button_matrix.transform_by_inverse(&sub_mouse_position, mouse_position);

				if (rec.m_character->point_test(sub_mouse_position.m_x, sub_mouse_position.m_y))
				{
					// The mouse is inside the shape.
					m_mouse_flags |= FLAG_OVER;
					break;
				}
			}}

			if (mbuttons)
			{
				// Mouse button is pressed.
				m_mouse_flags |= FLAG_DOWN;
			}


			if (m_mouse_flags == m_last_mouse_flags)
			{
				// No state change
				return;
			}


			// Figure out what button_action::condition these states signify.
			button_action::condition	c = (button_action::condition) 0;
			
			int t = transition(m_last_mouse_flags, m_mouse_flags);

			// Common transitions.
			if (t == transition(IDLE, OVER_UP))	// Roll Over
			{
				c = button_action::IDLE_TO_OVER_UP;
				m_mouse_state = OVER;
			}
			else if (t == transition(OVER_UP, IDLE))	// Roll Out
			{
				c = button_action::OVER_UP_TO_IDLE;
				m_mouse_state = UP;
			}
			else if (m_def->m_menu)
			{
				// Menu button transitions.

				if (t == transition(OVER_UP, OVER_DOWN))	// Press
				{
					c = button_action::OVER_UP_TO_OVER_DOWN;
					m_mouse_state = DOWN;
				}
				else if (t == transition(OVER_DOWN, OVER_UP))	// Release
				{
					c = button_action::OVER_DOWN_TO_OVER_UP;
					m_mouse_state = OVER;
				}
				else if (t == transition(IDLE, OVER_DOWN))	// Drag Over
				{
					c = button_action::IDLE_TO_OVER_DOWN;
					m_mouse_state = DOWN;
				}
				else if (t == transition(OVER_DOWN, IDLE))	// Drag Out
				{
					c = button_action::OVER_DOWN_TO_IDLE;
					m_mouse_state = UP;
				}
			}
			else
			{
				// Push button transitions.

				if (t == transition(OVER_UP, OVER_DOWN))	// Press
				{
					c = button_action::OVER_UP_TO_OVER_DOWN;
					m_mouse_state = DOWN;
					m->set_mouse_capture( get_id() );
				}
				else if (t == transition(OVER_DOWN, OVER_UP))	// Release
				{
					c = button_action::OVER_DOWN_TO_OVER_UP;
					m_mouse_state = OVER;
					m->set_mouse_capture( -1 );
				}
				else if (t == transition(OUT_DOWN, OVER_DOWN))	// Drag Over
				{
					c = button_action::OUT_DOWN_TO_OVER_DOWN;
					m_mouse_state = DOWN;
				}
				else if (t == transition(OVER_DOWN, OUT_DOWN))	// Drag Out
				{
					c = button_action::OVER_DOWN_TO_OUT_DOWN;
					m_mouse_state = UP;
				}
				else if (t == transition(OUT_DOWN, IDLE))	// Release Outside
				{
					c = button_action::OUT_DOWN_TO_IDLE;
					m_mouse_state = UP;
					m->set_mouse_capture( -1 );
				}
			}

			// restart the characters of the new state.
			restart_characters();

			// Add appropriate actions to the movie's execute list...
			{for (int i = 0; i < m_def->m_button_actions.size(); i++)
			{
				if (m_def->m_button_actions[i].m_conditions & c)
				{
					// Matching action.
					for (int j = 0; j < m_def->m_button_actions[i].m_actions.size(); j++)
					{
						m->add_action_buffer(&(m_def->m_button_actions[i].m_actions[j]));
					}
				}
			}}
		}

		void restart_characters()
		{
			// Advance our relevant characters.
			{for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				//button_record&	rec = m_def->m_button_records[i];
				if (m_record_character[i] != NULL)
				{
					m_record_character[i]->restart();
				}

				/*if (m_mouse_state == UP)
				{
					if (rec.m_up)
					{
						m_record_character[i]->restart();
					}
				}
				else if (m_mouse_state == DOWN)
				{
					if (rec.m_down)
					{
						m_record_character[i]->restart();
					}
				}
				else if (m_mouse_state == OVER)
				{
					if (rec.m_over)
					{
						m_record_character[i]->restart();
					}
				}*/
			}}
		}

	};


	//
	// button_record
	//

	bool	button_record::read(stream* in, int tag_type, movie* m)
	// Return true if we read a record; false if this is a null record.
	{
		int	flags = in->read_u8();
		if (flags == 0)
		{
			return false;
		}
		m_hit_test = flags & 8 ? true : false;
		m_down = flags & 4 ? true : false;
		m_over = flags & 2 ? true : false;
		m_up = flags & 1 ? true : false;

		int	cid = in->read_u16();
		m_character = m->get_character(cid);
		m_button_layer = in->read_u16(); 
		m_button_matrix.read(in);

		if (tag_type == 34)
		{
			m_button_cxform.read_rgba(in);
		}

		return true;
	}


	//
	// button_action
	//

	void	button_action::read(stream* in, int tag_type)
	{
		// Read condition flags.
		if (tag_type == 7)
		{
			m_conditions = OVER_DOWN_TO_OVER_UP;
		}
		else
		{
			assert(tag_type == 34);
			m_conditions = in->read_u16();
		}

		// Read actions.
//			for (;;)
		{
			action_buffer	a;
			a.read(in);
			if (a.is_null())
			{
				// End marker; no more action records.
//					break;
			}
			m_actions.push_back(a);
		}
	}


	//
	// button_character_definition
	//

	button_character_definition::button_character_definition()
	// Constructor.
	{
	}


	bool	button_character_definition::is_definition() const
	// Return true, so that a containing movie knows to create an instance
	// for storing in its display list, instead of storing the definition
	// directly.
	{
		return true;
	}


	void	button_character_definition::read(stream* in, int tag_type, movie* m)
	// Initialize from the given stream.
	{
		assert(tag_type == 7 || tag_type == 34);

		if (tag_type == 7)
		{
			// Old button tag.
				
			// Read button character records.
			for (;;)
			{
				button_record	r;
				if (r.read(in, tag_type, m) == false)
				{
					// Null record; marks the end of button records.
					break;
				}
				m_button_records.push_back(r);
			}

			// Read actions.
			m_button_actions.resize(m_button_actions.size() + 1);
			m_button_actions.back().read(in, tag_type);
		}
		else if (tag_type == 34)
		{
			// Read the menu flag.
			m_menu = in->read_u8() != 0;

			int	button_2_action_offset = in->read_u16();
			int	next_action_pos = in->get_position() + button_2_action_offset - 2;

			// Read button records.
			for (;;)
			{
				button_record	r;
				if (r.read(in, tag_type, m) == false)
				{
					// Null record; marks the end of button records.
					break;
				}
				m_button_records.push_back(r);
			}

			if (button_2_action_offset > 0)
			{
				in->set_position(next_action_pos);

				// Read Button2ActionConditions
				for (;;)
				{
					int	next_action_offset = in->read_u16();
					next_action_pos = in->get_position() + next_action_offset - 2;

					m_button_actions.resize(m_button_actions.size() + 1);
					m_button_actions.back().read(in, tag_type);

					if (next_action_offset == 0
					    || in->get_position() >= in->get_tag_end_position())
					{
						// done.
						break;
					}

					// seek to next action.
					in->set_position(next_action_pos);
				}
			}
		}
	}


	character*	button_character_definition::create_instance()
	// Create a mutable instance of our definition.  The instance is
	// designed to be put on the display list, possibly active at the same
	// time as other instances of the same definition.
	{
		return new button_character_instance(this);
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
