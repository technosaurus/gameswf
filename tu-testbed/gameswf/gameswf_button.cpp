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
		enum mouse_state
		{
			OUT,		// when the mouse is not over the button, and mouse button is up.
			OUT_DOWN,	// when the mouse is not over the button, and mouse button is down.
			DOWN,		// when the mouse is over us, and mouse button is down.
			OVER		// when the mouse is over us, and mouse button is not pressed.
		};
		mouse_state	m_last_mouse_state, m_mouse_state;

		button_character_instance(button_character_definition* def)
			:
			m_def(def)
		{
			assert(m_def);
			m_id = def->m_id;
			restart();
		}

		bool	is_instance() const { return true; }

		void	restart()
		{
			m_last_mouse_state = OUT;
			m_mouse_state = OUT;
		}


		void	advance(float delta_time, movie* m, const matrix& mat)
		{
			assert(m);

			// Look at the mouse state, and figure out our button state.  We want to
			// know if the mouse is hovering over us, and whether it's clicking on us.
			m_last_mouse_state = m_mouse_state;
			int	mx, my, mbuttons;
			m->get_mouse_state(&mx, &my, &mbuttons);
			m_mouse_state = OUT;

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
					m_mouse_state = OVER;
					break;
				}
			}}

			if (mbuttons)
			{
				// Mouse button is pressed.
				if (m_mouse_state == OVER)
				{
					// Flash button is pressed.
					m_mouse_state = DOWN;
				}
				else
				{
					m_mouse_state = OUT_DOWN;
				}
			}

			// OUT/OUT_DOWN/DOWN/OVER
			// Figure out what button_action::condition these states signify.
			button_action::condition	c = (button_action::condition) 0;
			if (m_mouse_state == OVER)
			{
				if (m_last_mouse_state == OUT)
				{
					c = button_action::IDLE_TO_OVER_UP;
				}
				else if (m_last_mouse_state == DOWN)
				{
					c = button_action::OVER_DOWN_TO_OVER_UP;
				}
			}
			else if (m_mouse_state == OUT)
			{
				if (m_last_mouse_state == OVER)
				{
					c = button_action::OVER_UP_TO_IDLE;
				}
				else if (m_last_mouse_state == OUT_DOWN)
				{
					c = button_action::OUT_DOWN_TO_IDLE;
				}
				else if (m_last_mouse_state == DOWN)
				{
					c = button_action::OVER_DOWN_TO_IDLE;
				}
			}
			else if (m_mouse_state == OUT_DOWN)
			{
				if (m_last_mouse_state == DOWN)
				{
					c = button_action::OVER_DOWN_TO_OUT_DOWN;
				}
			}
			else if (m_mouse_state == DOWN)
			{
				if (m_last_mouse_state == OVER)
				{
					c = button_action::OVER_UP_TO_OVER_DOWN;
				}
				else if (m_last_mouse_state == OUT_DOWN)
				{
					c = button_action::OUT_DOWN_TO_OVER_DOWN;
				}
				else if (m_last_mouse_state == OUT)
				{
					c = button_action::IDLE_TO_OVER_DOWN;
				}
			}

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

			// Advance our relevant characters.
			{for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				button_record&	rec = m_def->m_button_records[i];
				if (rec.m_character == NULL)
				{
					continue;
				}

				matrix	sub_matrix = mat;
				sub_matrix.concatenate(rec.m_button_matrix);
				if (m_mouse_state == OUT
				    || m_mouse_state == OUT_DOWN)
				{
					if (rec.m_up)
					{
						rec.m_character->advance(delta_time, m, sub_matrix);
					}
				}
				else if (m_mouse_state == DOWN)
				{
					if (rec.m_down)
					{
						rec.m_character->advance(delta_time, m, sub_matrix);
					}
				}
				else if (m_mouse_state == OVER)
				{
					if (rec.m_over)
					{
						rec.m_character->advance(delta_time, m, sub_matrix);
					}
				}
			}}
		}

		void	display(const display_info& di)
		{
			for (int i = 0; i < m_def->m_button_records.size(); i++)
			{
				button_record&	rec = m_def->m_button_records[i];
				if (rec.m_character == NULL)
				{
					continue;
				}
				if (((m_mouse_state == OUT || m_mouse_state == OUT_DOWN) && rec.m_up)
				    || (m_mouse_state == DOWN && rec.m_down)
				    || (m_mouse_state == OVER && rec.m_over))
				{
					gameswf::render::push_apply_matrix(rec.m_button_matrix);
					gameswf::render::push_apply_cxform(rec.m_button_cxform);
					rec.m_character->display(di);
					gameswf::render::pop_matrix();
					gameswf::render::pop_cxform();
				}
			}
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
			int	flags = in->read_u8();
			flags = flags;	// inhibit warning

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
