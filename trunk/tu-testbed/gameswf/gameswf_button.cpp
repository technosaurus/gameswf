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
	// Dumb little proxy for holding button characters & applying transforms.
	struct button_record_container : public character
	{
		int	m_id;

		button_record_container(movie* parent, const matrix& mat, const cxform& cx, int id)
			:
			character(parent),
			m_id(id)
		{
			set_matrix(mat);
			set_cxform(cx);
		}

		virtual int	get_id() const { return m_id; }
		virtual const char*	get_name() const { return ""; }
	};

	struct button_character_instance : public character
	{
		button_character_definition*	m_def;
		array<character*>	m_record_character;
		array<button_record_container*>	m_container;

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

		button_character_instance(button_character_definition* def, movie* parent)
			:
			character(parent),
			m_def(def),
			m_last_mouse_flags(IDLE),
			m_mouse_flags(IDLE),
			m_mouse_state(UP)
		{
			assert(m_def);

			int r, r_num =  m_def->m_button_records.size();
			m_record_character.resize(r_num);
			m_container.resize(r_num);

			for (r = 0; r < r_num; r++)
			{
				character_def* cdef = m_def->m_button_records[r].m_character;
				const matrix&	mat = m_def->m_button_records[r].m_button_matrix;
				const cxform&	cx = m_def->m_button_records[r].m_button_cxform;
				button_record_container*	container = new button_record_container(
					this, mat, cx, get_id());
				m_container[r] = container;

				character*	ch = cdef->create_character_instance(container);
				m_record_character[r] = ch;
				ch->restart();
			}
		}

		~button_character_instance()
		{
			int r, r_num =  m_record_character.size();
			assert(m_container.size() == r_num);

			for (r = 0; r < r_num; r++)
			{
				delete m_record_character[r];
				delete m_container[r];
			}
		}

		movie_root*	get_movie_root() { return get_parent()->get_movie_root(); }

		virtual int	get_id() const { return m_def->get_id(); }
		virtual const char*	get_name() const { return m_def->get_name(); }

		void	restart()
		{
			m_last_mouse_flags = IDLE;
			m_mouse_flags = IDLE;
			m_mouse_state = UP;
			int r, r_num =  m_record_character.size();
			for (r = 0; r < r_num; r++)
			{
				m_record_character[r]->restart();
			}
		}


		void	advance(float delta_time)
		{
			matrix	mat = get_world_matrix();

			// Get current mouse capture.
			int id = get_parent()->get_mouse_capture();

			// update state if no mouse capture or we have the capture.
			//if (id == -1 || (!m_def->m_menu && id == get_id()))
			if (id == -1 || id == get_id())
			{
				update_state(mat);
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
						m_record_character[i]->advance(delta_time);
					}
				}
				else if (m_mouse_state == DOWN)
				{
					if (rec.m_down)
					{
						m_record_character[i]->advance(delta_time);
					}
				}
				else if (m_mouse_state == OVER)
				{
					if (rec.m_over)
					{
						m_record_character[i]->advance(delta_time);
					}
				}
			}}
		}

		void	display()
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
//					display_info	sub_di = di;
//					sub_di.m_matrix.concatenate(rec.m_button_matrix);
//					sub_di.m_color_transform.concatenate(rec.m_button_cxform);
					// @@ Hm, do we need a proxy container here?
					m_record_character[i]->display();
				}
			}
		}

		inline int	transition(int a, int b) const
		// Combine the flags to avoid a conditional. It would be faster with a macro.
		{
			return (a << 2) | b;
		}

		void	update_state(const matrix& mat)
		// Update button state.
		{

			// Look at the mouse state, and figure out our button state.  We want to
			// know if the mouse is hovering over us, and whether it's clicking on us.
			int	mx, my, mbuttons;
			get_parent()->get_mouse_state(&mx, &my, &mbuttons);

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
				// @@ suspicious!
				point	sub_mouse_position;
				rec.m_button_matrix.transform_by_inverse(&sub_mouse_position, mouse_position);

				if (rec.m_character->point_test_local(sub_mouse_position.m_x, sub_mouse_position.m_y))
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
					get_parent()->set_mouse_capture( get_id() );
				}
				else if (t == transition(OVER_DOWN, OVER_UP))	// Release
				{
					c = button_action::OVER_DOWN_TO_OVER_UP;
					m_mouse_state = OVER;
					get_parent()->set_mouse_capture( -1 );
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
					get_parent()->set_mouse_capture( -1 );
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
						get_parent()->add_action_buffer(&(m_def->m_button_actions[i].m_actions[j]));
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

	bool	button_record::read(stream* in, int tag_type, movie_definition_sub* m)
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
		m_character = m->get_character_def(cid);
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
		IF_VERBOSE_ACTION(log_msg("-- actions in button\n")); // @@ need more info about which actions
		action_buffer	a;
		a.read(in);
		m_actions.push_back(a);
	}


	//
	// button_character_definition
	//

	button_character_definition::button_character_definition()
	// Constructor.
	{
	}


// 	bool	button_character_definition::is_definition() const
// 	// Return true, so that a containing movie knows to create an instance
// 	// for storing in its display list, instead of storing the definition
// 	// directly.
// 	{
// 		return true;
// 	}


	void	button_character_definition::read(stream* in, int tag_type, movie_definition_sub* m)
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


	character*	button_character_definition::create_character_instance(movie* parent)
	// Create a mutable instance of our definition.
	{
		return new button_character_instance(this, parent);
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
