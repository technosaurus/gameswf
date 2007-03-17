// gameswf_sprite.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_SPRITE_H
#define GAMESWF_SPRITE_H


#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include "gameswf_movie.h"
#include "gameswf_movie_def.h"
#include "gameswf_dlist.h"
#include "gameswf_root.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>
//#include "gameswf_timers.h"
#include "gameswf_sprite_def.h"

namespace gameswf
{

	inline as_object* get_sprite_builtins();
	void	sprite_builtins_clear();
	void	sprite_builtins_init();

	struct sprite_instance : public character
	{
		smart_ptr<movie_definition_sub>	m_def;
		movie_root*	m_root;

		display_list	m_display_list;
		array<action_buffer*>	m_action_list;
		array<action_buffer*>	m_goto_frame_action_list;

		play_state	m_play_state;
		int		m_current_frame;
//		float		m_time_remainder;
		bool		m_update_frame;
//		bool		m_has_looped;
		bool		m_accept_anim_moves;	// once we've been moved by ActionScript, don't accept moves from anim tags.
		array<bool>	m_init_actions_executed;	// a bit-array class would be ideal for this

		as_environment	m_as_environment;

		enum mouse_state
		{
			UP = 0,
			DOWN,
			OVER
		};
		mouse_state m_mouse_state;
		bool m_enabled;
		bool m_on_event_load_called;

		sprite_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id);

		// sprite instance of add_interval_handler()
		virtual int    add_interval_timer(void *timer)
		{
			// log_msg("FIXME: %s:\n", __FUNCTION__);
			return m_root->add_interval_timer(timer);
		}

		virtual void	clear_interval_timer(int x)
		{
			// log_msg("FIXME: %s:\n", __FUNCTION__);
			m_root->clear_interval_timer(x);
		}

		virtual bool has_keypress_event()
		{
			for (hash<event_id, as_value>::iterator it = m_event_handlers.begin();
				it != m_event_handlers.end(); ++it)
			{
				if (it->first.m_id == event_id::KEY_PRESS)
				{
					return true;
				}
			}
			return false;
		}


		/* sprite_instance */
		virtual void	do_something(void *timer)
		{
			as_value	val;
			as_object      *obj, *this_ptr;
			as_environment *as_env;

			//printf("FIXME: %s:\n", __FUNCTION__);
			Timer *ptr = (Timer *)timer;
			//log_msg("INTERVAL ID is %d\n", ptr->getIntervalID());

			const as_value&	timer_method = ptr->getASFunction();
			as_env = ptr->getASEnvironment();
			this_ptr = ptr->getASObject();
			obj = ptr->getObject();
			//m_as_environment.push(obj);

			as_c_function_ptr	cfunc = timer_method.to_c_function();
			if (cfunc) {
				// It's a C function. Call it.
				//log_msg("Calling C function for interval timer\n");
				//(*cfunc)(&val, obj, as_env, 0, 0);
				(*cfunc)(fn_call(&val, obj, &m_as_environment, 0, 0));

			} else if (as_as_function* as_func = timer_method.to_as_function()) {
				// It's an ActionScript function. Call it.
				as_value method;
				//log_msg("Calling ActionScript function for interval timer\n");
				(*as_func)(fn_call(&val, (as_object_interface *)this_ptr, as_env, 0, 0));
				//(*as_func)(&val, (as_object_interface *)this_ptr, &m_as_environment, 1, 1);
			} else {
				log_error("error in call_method(): method is not a function\n");
			}    
		}	

		virtual ~sprite_instance()
		{
			m_display_list.clear();
			//m_root->drop_ref();
		}

		movie_interface*	get_root_interface() { return m_root; }
		movie_root*	get_root() { return m_root; }
		movie*	get_root_movie() { return m_root->get_root_movie(); }

		movie_definition*	get_movie_definition() { return m_def.get_ptr(); }

		virtual float	get_width()
		{
			float	w = 0;
			int i, n = m_display_list.get_character_count();
			character* ch;
			for (i = 0; i < n; i++)
			{
				ch = m_display_list.get_character(i);
				if (ch != NULL)
				{
					float ch_w = ch->get_width();
					if (ch_w > w)
					{
						w = ch_w;
					}
				}
			}

			return w;
		}



		virtual float	get_height()
		{
			float	h = 0; 
			int i, n = m_display_list.get_character_count();
			character* ch;
			for (i=0; i < n; i++)
			{
				ch = m_display_list.get_character(i);
				if (ch != NULL)
				{
					float	ch_h = ch->get_height();
					if (ch_h > h)
					{
						h = ch_h;
					}
				}
			}
			return h;
		}

		virtual int	get_current_frame() const { return m_current_frame; }
		virtual int	get_frame_count() const { return m_def->get_frame_count(); }


		character* add_empty_movieclip(const char* name, int depth)
		{
			cxform color_transform;
			matrix matrix;

			// empty_sprite_def will be deleted during deliting sprite
			sprite_definition* empty_sprite_def = new sprite_definition(NULL);

			sprite_instance* sprite =	new sprite_instance(empty_sprite_def, m_root, this, 0);
			sprite->set_name(name);

			m_display_list.add_display_object(
				sprite,
				depth,
				true,
				color_transform,
				matrix,
				0.0f,
				0); 

			return sprite;
		}

		void	set_play_state(play_state s)
			// Stop or play the sprite.
		{
//			if (m_play_state != s)
//			{
//				m_time_remainder = 0;
//			}

			m_play_state = s;
		}
		play_state	get_play_state() const { return m_play_state; }


		character*	get_character(int character_id)
		{
			//			return m_def->get_character_def(character_id);
			// @@ TODO -- look through our dlist for a match
			return NULL;
		}

		float	get_background_alpha() const
		{
			// @@ this doesn't seem right...
			return m_root->get_background_alpha();
		}

		float	get_pixel_scale() const { return m_root->get_pixel_scale(); }

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			m_root->get_mouse_state(x, y, buttons);
		}

		void	set_background_color(const rgba& color)
		{
			m_root->set_background_color(color);
		}

		void	restart()
		{
			m_current_frame = 0;
//			m_time_remainder = 0;
			m_update_frame = true;
//			m_has_looped = false;
			m_play_state = PLAY;

			m_display_list.clear();
			execute_frame_tags(m_current_frame);
//			m_display_list.update();
		}


//		virtual bool	has_looped() const { return m_has_looped; }

		virtual bool	get_accept_anim_moves() const { return m_accept_anim_moves; }

		inline int	transition(int a, int b) const
			// Combine the flags to avoid a conditional. It would be faster with a macro.
		{
			return (a << 2) | b;
		}


		virtual bool can_handle_mouse_event()
			// Return true if we have any mouse event handlers.
		{
			// We should cache this!
			as_value dummy;

			// Functions that qualify as mouse event handlers.
			const char* FN_NAMES[] = {
				"onKeyPress",
				"onRelease",
				"onDragOver",
				"onDragOut",
				"onPress",
				"onReleaseOutside",
				"onRollout",
				"onRollover",
			};
			for (int i = 0; i < ARRAYSIZE(FN_NAMES); i++) {
				if (get_member(FN_NAMES[i], &dummy)) {
					return true;
				}
			}

			// Event handlers that qualify as mouse event handlers.
			const event_id::id_code EH_IDS[] = {
				event_id::PRESS,
				event_id::RELEASE,
				event_id::RELEASE_OUTSIDE,
				event_id::ROLL_OVER,
				event_id::ROLL_OUT,
				event_id::DRAG_OVER,
				event_id::DRAG_OUT,
			};
			{for (int i = 0; i < ARRAYSIZE(EH_IDS); i++) {
				if (get_event_handler(EH_IDS[i], &dummy)) {
					return true;
				}
			}}

			return false;
		}


		/* sprite_instance */
		virtual movie*	get_topmost_mouse_entity(float x, float y)
			// Return the topmost entity that the given point
			// covers that can receive mouse events.  NULL if
			// none.  Coords are in parent's frame.
		{
			if (get_visible() == false) {
				return NULL;
			}

			matrix	m = get_matrix();
			point	p;
			m.transform_by_inverse(&p, point(x, y));

			movie*	top_te = NULL;
			bool this_has_focus = false;
			int i, n = m_display_list.get_character_count();

			// Go backwards, to check higher objects first.
			for (i = n - 1; i >= 0; i--)
			{
				character* ch = m_display_list.get_character(i);

				if (ch != NULL && ch->get_visible())
				{
					movie* te = ch->get_topmost_mouse_entity(p.m_x, p.m_y);
					if (te)
					{
						this_has_focus = true;
						// The containing entity that 1) is closest to root and 2) can
						// handle mouse events takes precedence.
						if (te->can_handle_mouse_event())
						{
							top_te = te;
							break;
						}
					}
				}
			}

			//  THIS is closest to root
			if (this_has_focus && can_handle_mouse_event())
			{
				return this;
			}

			if (top_te)
			{
				return top_te;
			}

			return NULL;
		}
 
		void advance(float delta_time);

		/*sprite_instance*/
		void	execute_frame_tags(int frame, bool state_only = false)
			// Execute the tags associated with the specified frame.
			// frame is 0-based
		{
			// Keep this (particularly m_as_environment) alive during execution!
			smart_ptr<as_object_interface>	this_ptr(this);

			assert(frame >= 0);
			assert(frame < m_def->get_frame_count());

			m_def->wait_frame(frame);

			// Execute this frame's init actions, if necessary.
			if (m_init_actions_executed[frame] == false)
			{
				const array<execute_tag*>*	init_actions = m_def->get_init_actions(frame);
				if (init_actions && init_actions->size() > 0)
				{
					// Need to execute these actions.
					for (int i= 0; i < init_actions->size(); i++)
					{
						execute_tag*	e = (*init_actions)[i];
						e->execute(this);
					}

					// Mark this frame done, so we never execute these init actions
					// again.
					m_init_actions_executed[frame] = true;
				}
			}

			const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (state_only)
				{
					e->execute_state(this);
				}
				else
				{
					e->execute(this);
				}
			}
		}


		/*sprite_instance*/
		void	execute_frame_tags_reverse(int frame)
			// Execute the tags associated with the specified frame, IN REVERSE.
			// I.e. if it's an "add" tag, then we do a "remove" instead.
			// Only relevant to the display-list manipulation tags: add, move, remove, replace.
			//
			// frame is 0-based
		{
			// Keep this (particularly m_as_environment) alive during execution!
			smart_ptr<as_object_interface>	this_ptr(this);

			assert(frame >= 0);
			assert(frame < m_def->get_frame_count());

			const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				e->execute_state_reverse(this, frame);
			}
		}


		/*sprite_instance*/
		execute_tag*	find_previous_replace_or_add_tag(int frame, int depth, int id)
		{
			uint32	depth_id = ((depth & 0x0FFFF) << 16) | (id & 0x0FFFF);

			for (int f = frame - 1; f >= 0; f--)
			{
				const array<execute_tag*>&	playlist = m_def->get_playlist(f);
				for (int i = playlist.size() - 1; i >= 0; i--)
				{
					execute_tag*	e = playlist[i];
					if (e->get_depth_id_of_replace_or_add_tag() == depth_id)
					{
						return e;
					}
				}
			}

			return NULL;
		}


		/*sprite_instance*/
		void	execute_remove_tags(int frame)
			// Execute any remove-object tags associated with the specified frame.
			// frame is 0-based
		{
			assert(frame >= 0);
			assert(frame < m_def->get_frame_count());

			const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
			for (int i = 0; i < playlist.size(); i++)
			{
				execute_tag*	e = playlist[i];
				if (e->is_remove_tag())
				{
					e->execute_state(this);
				}
			}
		}

		void	do_actions();
		virtual void do_actions(const array<action_buffer*>& action_list);
		void	goto_frame(int target_frame_number);
		bool	goto_labeled_frame(const char* label);

		void	display();

		/*sprite_instance*/
		character*	add_display_object(
			Uint16 character_id,
			const char* name,
			const array<swf_event*>& event_handlers,
			Uint16 depth,
			bool replace_if_depth_is_occupied,
			const cxform& color_transform,
			const matrix& matrix,
			float ratio,
			Uint16 clip_depth);

		void	move_display_object(
			Uint16 depth,
			bool use_cxform,
			const cxform& color_xform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth);


		void	replace_display_object(
			Uint16 character_id,
			const char* name,
			Uint16 depth,
			bool use_cxform,
			const cxform& color_transform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth);

		void	replace_display_object(
			character* ch,
			const char* name,
			Uint16 depth,
			bool use_cxform,
			const cxform& color_transform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth);

		void	remove_display_object(Uint16 depth, int id);
		void	add_action_buffer(action_buffer* a);
		int	get_id_at_depth(int depth);

		//
		// ActionScript support
		//


		/* sprite_instance */
		virtual void	set_variable(const char* path_to_var, const char* new_value);
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value);
		virtual const char*	get_variable(const char* path_to_var) const;

		/* sprite_instance */
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual movie*	get_relative_target(const tu_string& name);
		virtual void	call_frame_actions(const as_value& frame_spec);
		virtual void	set_drag_state(const drag_state& st);
		virtual void	stop_drag();
		virtual void	get_drag_state(drag_state* st);
		character*	clone_display_object(const tu_string& newname, Uint16 depth, as_object* init_object);
		void	remove_display_object(const tu_string& name);
		virtual bool	on_event(const event_id& id);
		virtual void	on_event_xmlsocket_onxml();
		virtual void	on_event_interval_timer();
		virtual void	on_event_load_progress();
		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args);
		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr);
	};
}

#endif
