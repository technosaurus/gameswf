// gameswf_root.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_render.h"
#include "gameswf/gameswf_root.h"
#include "base/tu_random.h"

namespace gameswf
{

	static tu_mutex s_gameswf_engine;
	tu_mutex& gameswf_engine_mutex()
	{
		return s_gameswf_engine;
	}

	movie_root::movie_root(movie_def_impl* def)
			:
			m_def(def),
			m_movie(NULL),
			m_viewport_x0(0),
			m_viewport_y0(0),
			m_viewport_width(1),
			m_viewport_height(1),
			m_pixel_scale(1.0f),
			m_background_color(0, 0, 0, 255),
			m_mouse_x(0),
			m_mouse_y(0),
			m_mouse_buttons(0),
			m_userdata(NULL),
			m_on_event_load_called(false),
			m_current_active_entity(NULL),

			// the first time we needs do advance() and
			// then display() certainly
			m_time_remainder(1.0f),

			m_frame_time(1.0f)
		{
			assert(m_def != NULL);
			set_display_viewport(0, 0, (int) m_def->get_width_pixels(), (int) m_def->get_height_pixels());
			m_frame_time = 1.0f / get_frame_rate();
		}

		movie_root::~movie_root()
		{
			assert(m_def != NULL);
			m_movie = NULL;
			m_def = NULL;
		}

		void	movie_root::generate_mouse_button_events(mouse_button_state* ms)
		{
			smart_ptr<character>	active_entity = ms->m_active_entity;
			smart_ptr<character>	topmost_entity = ms->m_topmost_entity;

			// set cursor as passive/active object
			if (active_entity != NULL)
			{
				if (active_entity->can_handle_mouse_event())
				{
					render::set_cursor(render_handler::ACTIVE_CURSOR);
				}
				else
				{
					render::set_cursor(render_handler::SYSTEM_CURSOR);
				}
			}
			else
			{
				render::set_cursor(render_handler::SYSTEM_CURSOR);
			}

			if (ms->m_mouse_button_state_last == 1)
			{
				// Mouse button was down.

				// Handle trackAsMenu dragOver
				if (active_entity == NULL
					|| active_entity->get_track_as_menu())
				{
					if (topmost_entity != NULL
						&& topmost_entity != active_entity
						&& topmost_entity->get_track_as_menu() == true)
					{
						// Transfer to topmost entity, dragOver
						active_entity = topmost_entity;
						active_entity->on_event(event_id::DRAG_OVER);
						ms->m_mouse_inside_entity_last = true;
					}
				}

				// Handle onDragOut, onDragOver
				if (ms->m_mouse_inside_entity_last == false)
				{
					if (topmost_entity == active_entity)
					{
						// onDragOver
						if (active_entity != NULL)
						{
							active_entity->on_event(event_id::DRAG_OVER);
						}
						ms->m_mouse_inside_entity_last = true;
					}
				}
				else
				{
					// mouse_inside_entity_last == true
					if (topmost_entity != active_entity)
					{
						// onDragOut
						if (active_entity != NULL)
						{
							active_entity->on_event(event_id::DRAG_OUT);
						}
						ms->m_mouse_inside_entity_last = false;
					}
				}

				// Handle onRelease, onReleaseOutside
				if (ms->m_mouse_button_state_current == 0)
				{
					// Mouse button just went up.
					ms->m_mouse_button_state_last = 0;

					if (active_entity != NULL)
					{
						if (ms->m_mouse_inside_entity_last)
						{
							// onRelease
							active_entity->on_event(event_id::RELEASE);
						}
						else
						{
							// onReleaseOutside
							if (active_entity->get_track_as_menu() == false)
							{
								active_entity->on_event(event_id::RELEASE_OUTSIDE);
							}
						}
					}
				}
			}

			if (ms->m_mouse_button_state_last == 0)
			{
				// Mouse button was up.

				// New active entity is whatever is below the mouse right now.
				if (topmost_entity != active_entity)
				{
					// onRollOut
					if (active_entity != NULL)
					{
						active_entity->on_event(event_id::ROLL_OUT);
					}

					active_entity = topmost_entity;

					// onRollOver
					if (active_entity != NULL)
					{
						active_entity->on_event(event_id::ROLL_OVER);
					}

					ms->m_mouse_inside_entity_last = true;
				}

				// mouse button press
				if (ms->m_mouse_button_state_current == 1)
				{
					// onPress

					// set/kill focus
					// It's another entity ?
					if (m_current_active_entity != active_entity)
					{
						// First to clean focus
						if (m_current_active_entity != NULL)
						{
							m_current_active_entity->on_event(event_id::KILLFOCUS);
							m_current_active_entity = NULL;
						}

						// Then to set focus
						if (active_entity != NULL)
						{
							if (active_entity->on_event(event_id::SETFOCUS))
							{
								m_current_active_entity = active_entity;
							}
						}
					}

					if (active_entity != NULL)
					{
						active_entity->on_event(event_id::PRESS);
					}
					ms->m_mouse_inside_entity_last = true;
					ms->m_mouse_button_state_last = 1;
				}
			}

			// Write the (possibly modified) smart_ptr copies back
			// into the state struct.
			ms->m_active_entity = active_entity;
			ms->m_topmost_entity = topmost_entity;
		}

		// movie info

		int	movie_root::get_movie_version()
		{
			return m_def->get_version();
		}

		int	movie_root::get_movie_width()
		{
			return (int) m_def->get_width_pixels();
		}
		int	movie_root::get_movie_height()
		{
			return (int) m_def->get_height_pixels();
		}
		float	movie_root::get_movie_fps()
		{
			return m_def->get_frame_rate();
		}

		void	action_init();
		void notify_key_object(key::code k, bool down);

		void	movie_root::notify_key_event(key::code k, bool down)
		{

			// First notify global Key object
			// listeners uses the last keypressed code
			notify_key_object(k, down);

			// Notify keypress listeners.
			for (hash< smart_ptr<as_object_interface>, int >::iterator it = m_listeners.begin();
				it != m_listeners.end(); ++it)
			{
				if (it->second == KEYPRESS)
				{
					// protect from deleting in Action Script
					smart_ptr<as_object_interface> obj = it->first;

					if (down)
					{
						obj->on_event(event_id(event_id::KEY_DOWN));
						obj->on_event(event_id(event_id::KEY_PRESS, (key::code) k));
					}
					else
					{
						obj->on_event(event_id(event_id::KEY_UP));
					}
				}
			}
		}

		void	movie_root::advance_listeners(float delta_time)
		{
			// Notify network listeners.
			for (hash< smart_ptr<as_object_interface>, int >::iterator it = m_listeners.begin();
				it != m_listeners.end(); ++it)
			{
				if (it->second == ADVANCE)
				{
					// protect from deleting in Action Script
					smart_ptr<as_object_interface> obj = it->first;

					obj->advance(delta_time);
				}
			}
		}

		void movie_root::add_listener(as_object_interface* listener, 
			listener_type lt) 
		{
			m_listeners[listener] = lt;

			// sanity check
//			printf("add_keypress_listener=%08X (%d)\n", listener, m_keypress_listeners.size());
			assert(m_listeners.size() < 100);
		} 

		void movie_root::remove_listener(as_object_interface* listener) 
		{
			// Do not delete this "if", it is necessary for prevention
			// of an infinite loop
			if (m_listeners.size() > 0)
			{
				m_listeners.erase(listener);
			}
//			printf("remove_keypress_listener=%08X (%d)\n", listener, m_keypress_listeners.size());
		} 

		// @@ should these delegate to m_movie?	 Probably...
		bool	movie_root::set_member(const tu_stringi& name, const as_value& val) { return false; }
		bool	movie_root::get_member(const tu_stringi& name, as_value* val) { return false; }
		character*	movie_root::cast_to_character() { return m_movie.get_ptr(); } 

		void	movie_root::set_root_movie(character* root_movie)
		{
			m_movie = root_movie;
			assert(m_movie != NULL);
		}

		void	movie_root::set_display_viewport(int x0, int y0, int w, int h)
		{
			m_viewport_x0 = x0;
			m_viewport_y0 = y0;
			m_viewport_width = w;
			m_viewport_height = h;

			// Recompute pixel scale.
			float	scale_x = m_viewport_width / TWIPS_TO_PIXELS(m_def->m_frame_size.width());
			float	scale_y = m_viewport_height / TWIPS_TO_PIXELS(m_def->m_frame_size.height());
			m_pixel_scale = fmax(scale_x, scale_y);
		}


		void	movie_root::notify_mouse_state(int x, int y, int buttons)
			// The host app uses this to tell the movie where the
			// user's mouse pointer is.
		{
			m_mouse_x = x;
			m_mouse_y = y;
			m_mouse_buttons = buttons;
		}

		void	movie_root::get_mouse_state(int* x, int* y, int* buttons)
			// Use this to retrieve the last state of the mouse, as set via
			// notify_mouse_state().  Coordinates are in PIXELS, NOT TWIPS.
		{
			assert(x);
			assert(y);
			assert(buttons);

			*x = m_mouse_x;
			*y = m_mouse_y;
			*buttons = m_mouse_buttons;
		}

		character*	movie_root::get_root_movie() { return m_movie.get_ptr(); }


		void	movie_root::stop_drag()
		{
			m_drag_state.m_character = NULL;
		}

		movie_definition*	movie_root::get_movie_definition() { return m_movie->get_movie_definition(); }

		// 0-based!!
		int	movie_root::get_current_frame() const { return m_movie->get_current_frame(); }
		float	movie_root::get_frame_rate() const { return m_def->get_frame_rate(); }

		float	movie_root::get_pixel_scale() const
			// Return the size of a logical movie pixel as
			// displayed on-screen, with the current device
			// coordinates.
		{
			return m_pixel_scale;
		}

		// @@ Is this one necessary?
		character*	movie_root::get_character(int character_id)
		{
			return m_movie->get_character(character_id);
		}

		void	movie_root::set_background_color(const rgba& color)
		{
			m_background_color = color;
		}

		void	movie_root::set_background_alpha(float alpha)
		{
			m_background_color.m_a = iclamp(frnd(alpha * 255.0f), 0, 255);
		}

		float	movie_root::get_background_alpha() const
		{
			return m_background_color.m_a / 255.0f;
		}

		void	movie_root::advance(float delta_time)
		{
			// Lock gameswf engine. Video is running in separate thread and
			// it calls gameswf functions from separate thread to set
			// status of netstream object
			gameswf_engine_mutex().lock();

			action_init();	// @@ put this in some global init somewhere else...

			// Handle the mouse.
			m_mouse_button_state.m_topmost_entity =
				m_movie->get_topmost_mouse_entity(PIXELS_TO_TWIPS(m_mouse_x), PIXELS_TO_TWIPS(m_mouse_y));

			m_mouse_button_state.m_mouse_button_state_current = (m_mouse_buttons & 1);
			generate_mouse_button_events(&m_mouse_button_state);

			// advance Action script objects
			advance_listeners(delta_time);

			m_time_remainder += delta_time;
      if (m_time_remainder >= m_frame_time)
			{
				// this should be called infinitely to not repeat
				// the game situation after restart
				tu_random::next_random();

				m_movie->advance(delta_time); 
				if (m_on_event_load_called == false)
				{
					// Must do loading events.  For child sprites this is
					// done by the dlist, but root movies don't get added
					// to a dlist, so we do it here.
					m_on_event_load_called = true;
					m_movie->on_event(event_id::LOAD);
				}

				m_time_remainder = fmod(m_time_remainder - m_frame_time, m_frame_time);
			}

			gameswf_engine_mutex().unlock();
		}

		// 0-based!!
		void	movie_root::goto_frame(int target_frame_number) { m_movie->goto_frame(target_frame_number); }

		bool	movie_root::has_looped() const { return m_movie->has_looped(); }

		void	movie_root::display()
		{
			if (m_movie->get_visible() == false)
			{
				// Don't display.
				return;
			}

			gameswf::render::begin_display(
				m_background_color,
				m_viewport_x0, m_viewport_y0,
				m_viewport_width, m_viewport_height,
				m_def->m_frame_size.m_x_min, m_def->m_frame_size.m_x_max,
				m_def->m_frame_size.m_y_min, m_def->m_frame_size.m_y_max);

			m_movie->display();

			gameswf::render::end_display();
		}

		bool	movie_root::goto_labeled_frame(const char* label)
		{
			int	target_frame = -1;
			if (m_def->get_labeled_frame(label, &target_frame))
			{
				goto_frame(target_frame);
				return true;
			}
			else
			{
				IF_VERBOSE_ACTION(
					log_error("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label));
				return false;
			}
		}

		void	movie_root::set_play_state(play_state s) { m_movie->set_play_state(s); }
		movie_root::play_state	movie_root::get_play_state() const { return m_movie->get_play_state(); }

		/* movie_root */
		void	movie_root::set_variable(const char* path_to_var, const char* new_value)
		{
			m_movie->set_variable(path_to_var, new_value);
		}

		/* movie_root */
		void	movie_root::set_variable(const char* path_to_var, const wchar_t* new_value)
		{
			m_movie->set_variable(path_to_var, new_value);
		}

		const char*	movie_root::get_variable(const char* path_to_var) const
		{
			return m_movie->get_variable(path_to_var);
		}

		/*movie_root*/
		// For ActionScript interfacing convenience.
		const char*	movie_root::call_method(const char* method_name, const char* method_arg_fmt, ...)
		{
			assert(m_movie != NULL);

			va_list	args;
			va_start(args, method_arg_fmt);
			const char*	result = m_movie->call_method_args(method_name, method_arg_fmt, args);
			va_end(args);

			return result;
		}

		/*movie_root*/
		const char*	movie_root::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
		{
			assert(m_movie != NULL);
			return m_movie->call_method_args(method_name, method_arg_fmt, args);
		}

		void	movie_root::set_visible(bool visible) { m_movie->set_visible(visible); }
		bool	movie_root::get_visible() const { return m_movie->get_visible(); }

		void* movie_root::get_userdata() { return m_userdata; }
		void movie_root::set_userdata(void * ud ) { m_userdata = ud;  }

		void	movie_root::attach_display_callback(const char* path_to_object, void (*callback)(void* user_ptr), void* user_ptr)
		{
			m_movie->attach_display_callback(path_to_object, callback, user_ptr);
		}
}
