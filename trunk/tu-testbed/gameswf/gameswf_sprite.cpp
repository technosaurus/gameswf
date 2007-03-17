// gameswf_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "base/tu_file.h"
#include "base/utility.h"
#include "gameswf_action.h"
#include "gameswf_button.h"
#include "gameswf_impl.h"
#include "gameswf_font.h"
#include "gameswf_fontlib.h"
#include "gameswf_log.h"
#include "gameswf_morph2.h"
#include "gameswf_video_impl.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_styles.h"
#include "gameswf_dlist.h"
#include "gameswf_timers.h"
#include "gameswf_root.h"
#include "gameswf_movie_def.h"
#include "gameswf_sprite_def.h"
#include "gameswf_sprite.h"
#include "gameswf_as_sprite.h"
#include "base/image.h"
#include "base/jpeg.h"
#include "base/zlib_adapter.h"
#include "base/tu_random.h"
#include <string.h>	// for memset
#include <typeinfo>
#include <float.h>


#if TU_CONFIG_LINK_TO_ZLIB
#include <zlib.h>
#endif // TU_CONFIG_LINK_TO_ZLIB


namespace gameswf
{

	// For built-in sprite ActionScript methods.
	static as_object*	s_sprite_builtins = NULL;	// shared among all sprites.
	static void	sprite_builtins_init();
	void	sprite_builtins_clear();

	void	execute_actions(as_environment* env, const array<action_buffer*>& action_list);

	as_object* get_sprite_builtins()
	{
		return s_sprite_builtins;
	}

	void	sprite_builtins_clear()
	{
		if (s_sprite_builtins)
		{
			delete s_sprite_builtins;
			s_sprite_builtins = 0;
		}
	}


	static void	sprite_builtins_init()
	{
		if (get_sprite_builtins())
		{
			return;
		}

		s_sprite_builtins = new as_object;
		s_sprite_builtins->set_member("play", &sprite_play);
		s_sprite_builtins->set_member("stop", &sprite_stop);
		s_sprite_builtins->set_member("gotoAndStop", &sprite_goto_and_stop);
		s_sprite_builtins->set_member("gotoAndPlay", &sprite_goto_and_play);
		s_sprite_builtins->set_member("nextFrame", &sprite_next_frame);
		s_sprite_builtins->set_member("prevFrame", &sprite_prev_frame);
		s_sprite_builtins->set_member("getBytesLoaded", &sprite_get_bytes_loaded);
		s_sprite_builtins->set_member("getBytesTotal", &sprite_get_bytes_loaded);
		s_sprite_builtins->set_member("swapDepths", &sprite_swap_depths);
		s_sprite_builtins->set_member("duplicateMovieClip", &sprite_duplicate_movieclip);
		s_sprite_builtins->set_member("getDepth", &sprite_get_depth);
		s_sprite_builtins->set_member("createEmptyMovieClip", &sprite_create_empty_movieclip);
		s_sprite_builtins->set_member("removeMovieClip", &sprite_remove_movieclip);

		// @TODO
		//		s_sprite_builtins->set_member("startDrag", &sprite_start_drag);
		//		s_sprite_builtins->set_member("stopDrag", &sprite_stop_drag);
		//		s_sprite_builtins->set_member("getURL", &sprite_get_url);
	}


	sprite_instance::sprite_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id)
		:
	character(parent, id),
		m_def(def),
		m_root(r),
		m_play_state(PLAY),
		m_current_frame(0),
		//			m_time_remainder(0),
		m_update_frame(true),
		//			m_has_looped(false),
		m_accept_anim_moves(true),
		m_mouse_state(UP),
		m_enabled(true),
		m_on_event_load_called(false)
	{
		assert(m_def != NULL);
		assert(m_root != NULL);

		//m_root->add_ref();	// @@ circular!
		m_as_environment.set_target(this);

		sprite_builtins_init();

		// Initialize the flags for init action executed.
		m_init_actions_executed.resize(m_def->get_frame_count());
		memset(&m_init_actions_executed[0], 0,
			sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());
	}

	sprite_instance::~sprite_instance()
	{
		m_display_list.clear();
		//m_root->drop_ref();
	}

	// sprite instance of add_interval_handler()
	int sprite_instance::add_interval_timer(void *timer)
	{
		// log_msg("FIXME: %s:\n", __FUNCTION__);
		return m_root->add_interval_timer(timer);
	}

	void	sprite_instance::clear_interval_timer(int x)
	{
		// log_msg("FIXME: %s:\n", __FUNCTION__);
		m_root->clear_interval_timer(x);
	}


	bool sprite_instance::has_keypress_event()
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

	void	sprite_instance::do_something(void *timer)
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

	float	sprite_instance::get_width()
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

	float	sprite_instance::get_height()
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

	character* sprite_instance::add_empty_movieclip(const char* name, int depth)
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

	void	sprite_instance::set_play_state(play_state s)
		// Stop or play the sprite.
	{
		//			if (m_play_state != s)
		//			{
		//				m_time_remainder = 0;
		//			}

		m_play_state = s;
	}

	sprite_instance::play_state	sprite_instance::get_play_state() const
	{
		return m_play_state; 
	}

	bool sprite_instance::can_handle_mouse_event()
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


	movie*	sprite_instance::get_topmost_mouse_entity(float x, float y)
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


	// Very-very "thin" thing. 100 times think before updating
	void sprite_instance::advance(float delta_time) 
	{ 
		// Vitaly: 
		// child movieclip frame rate is the same the root movieclip frame rate 
		// that's why it is not needed to analyze 'm_time_remainder' 
		if (m_on_event_load_called == false) 
		{ 
			// clip sprite onload 
			// Vitaly:
			// _root.onLoad() will not be executed since do_actions()
			// for frame(0) has not executed yet.
			// _root.onLoad() will be executed later in movie_root::advance()
			on_event(event_id::LOAD);
		} 

		// mouse drag. 
		character::do_mouse_drag(); 

		if (m_on_event_load_called) 
		{ 
			on_event(event_id::ENTER_FRAME); 
		} 

		// execute actions from gotoAndPlay(n) or gotoAndStop(n) frame
		if (m_goto_frame_action_list.size() > 0)
		{
			execute_actions(&m_as_environment, m_goto_frame_action_list);
			m_goto_frame_action_list.clear();
		}

		// Update current and next frames. 
		if (m_play_state == PLAY) 
		{ 
			int prev_frame = m_current_frame;
			if (m_on_event_load_called) 
			{ 
				m_current_frame++;
				if (m_current_frame >= m_def->get_frame_count())
				{
					m_current_frame = 0;
				}
			} 

			// Execute the current frame's tags. 
			// execute_frame_tags(0) already executed in dlist.cpp 
			if (m_current_frame != prev_frame) 
			{ 
				//Vitaly:
				// Macromedia Flash does not call remove display object tag
				// for 1-st frame therefore we should do it for it :-)
				if (m_current_frame == 0 && m_def->get_frame_count() > 1)
				{
					// affected depths
					const array<execute_tag*>& playlist = m_def->get_playlist(0);
					array<Uint16> affected_depths;
					for (int i = 0; i < playlist.size(); i++)
					{
						Uint16 depth = (playlist[i]->get_depth_id_of_replace_or_add_tag()) >> 16;
						if (depth != static_cast<uint16>(-1))
						{
							affected_depths.push_back(depth);
						}
					}

					if (affected_depths.size() > 0)
					{
						m_display_list.clear_unaffected(affected_depths);					
					}
					else
					{
						m_display_list.clear();
					}

				}
				execute_frame_tags(m_current_frame); 
			} 
		} 

		do_actions(); 

		// Advance everything in the display list. 
		m_display_list.advance(delta_time); 

		m_on_event_load_called = true; 
	} 


	void	sprite_instance::execute_frame_tags(int frame, bool state_only)
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


	void	sprite_instance::execute_frame_tags_reverse(int frame)
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


	execute_tag*	sprite_instance::find_previous_replace_or_add_tag(int frame, int depth, int id)
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


	void	sprite_instance::execute_remove_tags(int frame)
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

	void	sprite_instance::do_actions()
		// Take care of this frame's actions.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object_interface>	this_ptr(this);

		execute_actions(&m_as_environment, m_action_list);
		m_action_list.resize(0);
	}

	void sprite_instance::do_actions(const array<action_buffer*>& action_list)
	{
		for (int i = 0; i < action_list.size(); i++)
		{
			action_list[i]->execute(&m_as_environment);
		}
	}

	void	sprite_instance::goto_frame(int target_frame_number)
		// Set the sprite state at the specified frame number.
		// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
	{
		// Macromedia Flash ignores goto_frame(bad_frame)
		if (target_frame_number > m_def->get_frame_count() - 1 ||
			target_frame_number < 0 ||
			target_frame_number == m_current_frame)	// to prevent infinitive recursion
		{
			m_play_state = STOP;
			return;
		}

		if (target_frame_number < m_current_frame)
		{
			for (int f = m_current_frame; f > target_frame_number; f--)
			{
				execute_frame_tags_reverse(f);
			}
			m_action_list.clear();
			execute_frame_tags(target_frame_number, false);
			//				m_display_list.update();
		}
		else if (target_frame_number > m_current_frame)
		{
			for (int f = m_current_frame + 1; f < target_frame_number; f++)
			{
				execute_frame_tags(f, true);
			}
			m_action_list.clear();
			execute_frame_tags(target_frame_number, false);
			//				m_display_list.update();
		}

		m_current_frame = target_frame_number;	    

		// goto_frame stops by default.
		m_play_state = STOP;

		// actions from gotoFrame() will be executed in advance()
		// Macromedia Flash does goto_frame then run actions from this frame.
		// We do too.
		m_goto_frame_action_list = m_action_list; 
		m_action_list.clear();

	}


	bool	sprite_instance::goto_labeled_frame(const char* label)
		// Look up the labeled frame, and jump to it.
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

	void	sprite_instance::display()
	{
		if (get_visible() == false)
		{
			// We're invisible, so don't display!
			return;
		}

		m_display_list.display();

		do_display_callback();
	}

	/*sprite_instance*/
	character*	sprite_instance::add_display_object(
		Uint16 character_id,
		const char* name,
		const array<swf_event*>& event_handlers,
		Uint16 depth,
		bool replace_if_depth_is_occupied,
		const cxform& color_transform,
		const matrix& matrix,
		float ratio,
		Uint16 clip_depth)
		// Add an object to the display list.
	{
		assert(m_def != NULL);

		character_def*	cdef = m_def->get_character_def(character_id);
		if (cdef == NULL)
		{
			log_error("sprite::add_display_object(): unknown cid = %d\n", character_id);
			return NULL;
		}

		// If we already have this object on this
		// plane, then move it instead of replacing
		// it.
		character*	existing_char = m_display_list.get_character_at_depth(depth);
		if (existing_char
			&& existing_char->get_id() == character_id
			&& ((name == NULL && existing_char->get_name().length() == 0)
			|| (name && existing_char->get_name() == name)))
		{
			//				IF_VERBOSE_DEBUG(log_msg("add changed to move on depth %d\n", depth));//xxxxxx

			// compare events 
			hash<event_id, as_value>* existing_events =
				(hash<event_id, as_value>*) existing_char->get_event_handlers(); 
			int n = event_handlers.size(); 
			if (existing_events->size() == n) 
			{ 
				bool same_events = true; 
				for (int i = 0; i < n; i++) 
				{ 
					as_value result; 
					if (existing_events->get(event_handlers[i]->m_event, &result)) 
					{ 
						// compare actionscipt in event 
						if (event_handlers[i]->m_method == result) 
						{ 
							continue; 
						} 
					} 
					same_events = false; 
					break; 
				} 

				if (same_events) 
				{ 
					move_display_object(depth, true, color_transform, true, matrix, ratio, clip_depth);
					return NULL;
				}
			}
		}

		assert(cdef);
		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
		assert(ch != NULL);
		if (name != NULL && name[0] != 0)
		{
			ch->set_name(name);
		}

		// Attach event handlers (if any).
		{
			for (int i = 0, n = event_handlers.size(); i < n; i++)
			{
				event_handlers[i]->attach_to(ch.get_ptr());
			}
		}

		m_display_list.add_display_object(
			ch.get_ptr(),
			depth,
			replace_if_depth_is_occupied,
			color_transform,
			matrix,
			ratio,
			clip_depth);

		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get_ptr();
	}


	/*sprite_instance*/
	void	sprite_instance::move_display_object(
		Uint16 depth,
		bool use_cxform,
		const cxform& color_xform,
		bool use_matrix,
		const matrix& mat,
		float ratio,
		Uint16 clip_depth)
		// Updates the transform properties of the object at
		// the specified depth.
	{
		m_display_list.move_display_object(depth, use_cxform, color_xform, use_matrix, mat, ratio, clip_depth);
	}


	/*sprite_instance*/
	void	sprite_instance::replace_display_object(
		Uint16 character_id,
		const char* name,
		Uint16 depth,
		bool use_cxform,
		const cxform& color_transform,
		bool use_matrix,
		const matrix& mat,
		float ratio,
		Uint16 clip_depth)
	{
		assert(m_def != NULL);

		character_def*	cdef = m_def->get_character_def(character_id);
		if (cdef == NULL)
		{
			log_error("sprite::replace_display_object(): unknown cid = %d\n", character_id);
			return;
		}
		assert(cdef);

		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
		assert(ch != NULL);

		if (name != NULL && name[0] != 0)
		{
			ch->set_name(name);
		}

		m_display_list.replace_display_object(
			ch.get_ptr(),
			depth,
			use_cxform,
			color_transform,
			use_matrix,
			mat,
			ratio,
			clip_depth);
	}


	/*sprite_instance*/
	void	sprite_instance::replace_display_object(
		character* ch,
		const char* name,
		Uint16 depth,
		bool use_cxform,
		const cxform& color_transform,
		bool use_matrix,
		const matrix& mat,
		float ratio,
		Uint16 clip_depth)
	{

		assert(ch != NULL);

		if (name != NULL && name[0] != 0)
		{
			ch->set_name(name);
		}

		m_display_list.replace_display_object(
			ch,
			depth,
			use_cxform,
			color_transform,
			use_matrix,
			mat,
			ratio,
			clip_depth);
	}


	/*sprite_instance*/
	void	sprite_instance::remove_display_object(Uint16 depth, int id)
		// Remove the object at the specified depth.
		// If id != -1, then only remove the object at depth with matching id.
	{
		m_display_list.remove_display_object(depth, id);
	}


	/*sprite_instance*/
	void	sprite_instance::add_action_buffer(action_buffer* a)
		// Add the given action buffer to the list of action
		// buffers to be processed at the end of the next
		// frame advance.
	{
		m_action_list.push_back(a);
	}


	/*sprite_instance*/
	int	sprite_instance::get_id_at_depth(int depth)
		// For debugging -- return the id of the character at the specified depth.
		// Return -1 if nobody's home.
	{
		int	index = m_display_list.get_display_index(depth);
		if (index == -1)
		{
			return -1;
		}

		character*	ch = m_display_list.get_display_object(index).m_character.get_ptr();

		return ch->get_id();
	}


	//
	// ActionScript support
	//

	void	sprite_instance::set_variable(const char* path_to_var, const char* new_value)
	{
		assert(m_parent == NULL);	// should only be called on the root movie.

		if (path_to_var == NULL)
		{
			log_error("error: NULL path_to_var passed to set_variable()\n");
			return;
		}
		if (new_value == NULL)
		{
			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
			return;
		}

		array<with_stack_entry>	empty_with_stack;
		tu_string	path(path_to_var);
		as_value	val(new_value);

		m_as_environment.set_variable(path, val, empty_with_stack);
	}

	void	sprite_instance::set_variable(const char* path_to_var, const wchar_t* new_value)
	{
		if (path_to_var == NULL)
		{
			log_error("error: NULL path_to_var passed to set_variable()\n");
			return;
		}
		if (new_value == NULL)
		{
			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
			return;
		}

		assert(m_parent == NULL);	// should only be called on the root movie.

		array<with_stack_entry>	empty_with_stack;
		tu_string	path(path_to_var);
		as_value	val(new_value);

		m_as_environment.set_variable(path, val, empty_with_stack);
	}

	const char*	sprite_instance::get_variable(const char* path_to_var) const
	{
		assert(m_parent == NULL);	// should only be called on the root movie.

		array<with_stack_entry>	empty_with_stack;
		tu_string	path(path_to_var);

		// NOTE: this is static so that the string
		// value won't go away after we return!!!
		// It'll go away during the next call to this
		// function though!!!  NOT THREAD SAFE!
		static as_value	val;

		val = m_as_environment.get_variable(path, empty_with_stack);

		return val.to_string();	// ack!
	}

	bool	sprite_instance::set_member(const tu_stringi& name, const as_value& val)
		// Set the named member to the value.  Return true if we have
		// that member; false otherwise.
	{
		// first try standart properties
		if (character::set_member(name, val))
		{
			m_accept_anim_moves = false;
			return true;
		}

		// Not a built-in property.  See if we have a
		// matching edit_text character in our display
		// list.
		bool	text_val = val.get_type() == as_value::STRING
			|| val.get_type() == as_value::NUMBER;
		if (text_val)
		{
			bool	success = false;
			for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
			{
				character*	ch = m_display_list.get_character(i);
				// CASE INSENSITIVE compare.  In ActionScript 2.0, this
				// changes to CASE SENSITIVE!!!
				if (name == ch->get_text_name())
				{
					const char* text = val.to_string();
					ch->set_text_value(text);
					success = true;
				}
			}
			if (success) return true;
		}

		// If that didn't work, set a variable within this environment.
		return m_as_environment.set_member(name, val);
	}

	bool	sprite_instance::get_member(const tu_stringi& name, as_value* val)
		// Set *val to the value of the named member and
		// return true, if we have the named member.
		// Otherwise leave *val alone and return false.
	{
		// first try standart properties
		if (character::get_member(name, val))
		{
			return true;
		}

		// Try variables.
		if (m_as_environment.get_member(name, val))
		{
			return true;
		}

		// Not a built-in property.  Check items on our
		// display list.
		character*	ch = m_display_list.get_character_by_name_i(name);
		if (ch)
		{
			// Found object.
			val->set_as_object_interface(static_cast<as_object_interface*>(ch));
			return true;
		}

		// Try static builtin functions.
		assert(s_sprite_builtins);
		if (s_sprite_builtins->get_member(name, val))
		{
			return true;
		}

		return false;
	}

	movie*	sprite_instance::get_relative_target(const tu_string& name)
		// Find the movie which is one degree removed from us,
		// given the relative pathname.
		//
		// If the pathname is "..", then return our parent.
		// If the pathname is ".", then return ourself.	 If
		// the pathname is "_level0" or "_root", then return
		// the root movie.
		//
		// Otherwise, the name should refer to one our our
		// named characters, so we return it.
		//
		// NOTE: In ActionScript 2.0, top level names (like
		// "_root" and "_level0") are CASE SENSITIVE.
		// Character names in a display list are CASE
		// SENSITIVE. Member names are CASE INSENSITIVE.  Gah.
		//
		// In ActionScript 1.0, everything seems to be CASE
		// INSENSITIVE.
	{
		if (name == "." || name == "this")
		{
			return this;
		}
		else if (name == ".." || name == "_parent")
		{
			return get_parent();
		}
		else if (name == "_level0" || name == "_root")
		{
			return m_root->m_movie.get_ptr();
		}

		// See if we have a match on the display list.
		return m_display_list.get_character_by_name(name);
	}

	void	sprite_instance::call_frame_actions(const as_value& frame_spec)
		// Execute the actions for the specified frame.	 The
		// frame_spec could be an integer or a string.
	{
		int	frame_number = -1;

		// Figure out what frame to call.
		if (frame_spec.get_type() == as_value::STRING)
		{
			if (m_def->get_labeled_frame(frame_spec.to_string(), &frame_number) == false)
			{
				// Try converting to integer.
				frame_number = (int) frame_spec.to_number();
			}
		}
		else
		{
			// convert from 1-based to 0-based
			frame_number = (int) frame_spec.to_number() - 1;
		}

		if (frame_number < 0 || frame_number >= m_def->get_frame_count())
		{
			// No dice.
			log_error("error: call_frame('%s') -- unknown frame\n", frame_spec.to_string());
			return;
		}

		int	top_action = m_action_list.size();

		// Execute the actions.
		const array<execute_tag*>&	playlist = m_def->get_playlist(frame_number);
		for (int i = 0; i < playlist.size(); i++)
		{
			execute_tag*	e = playlist[i];
			if (e->is_action_tag())
			{
				e->execute(this);
			}
		}

		// Execute any new actions triggered by the tag,
		// leaving existing actions to be executed.
		while (m_action_list.size() > top_action)
		{
			m_action_list[top_action]->execute(&m_as_environment);
			m_action_list.remove(top_action);
		}
		assert(m_action_list.size() == top_action);
	}


	/* sprite_instance */
	void	sprite_instance::set_drag_state(const drag_state& st)
	{
		m_root->m_drag_state = st;
	}

	void	sprite_instance::stop_drag()
	{
		assert(m_parent == NULL);	// we must be the root movie!!!

		m_root->stop_drag();
	}

	void	sprite_instance::get_drag_state(drag_state* st)
	{
		*st = m_root->m_drag_state;
	}


	character*	sprite_instance::clone_display_object(const tu_string& newname, Uint16 depth, as_object* init_object)
		// Duplicate the object with the specified name and add it with a new name 
		// at a new depth.
	{

		// Create the copy event handlers from sprite 
		// We should not copy 'm_action_buffer' since the 'm_method' already contains it 
		array<swf_event*> event_handlers; 
		const hash<event_id, as_value>* sprite_events = get_event_handlers(); 
		for (hash<event_id, as_value>::const_iterator it = sprite_events->begin();
			it != sprite_events->end(); ++it ) 
		{ 
			swf_event* e = new swf_event; 
			e->m_event = it->first; 
			e->m_method = it->second; 
			event_handlers.push_back(e); 
		} 

		character* parent = (character*) get_parent(); 
		character* ch = NULL; 
		if (parent != NULL) 
		{ 
			ch = parent->add_display_object( 
				get_id(), 
				newname.c_str(), 
				event_handlers, 
				depth, 
				true, // replace if depth is occupied (to drop) 
				get_cxform(), 
				get_matrix(), 
				get_ratio(), 
				get_clip_depth()); 

			// Copy members from initObject 
			if (init_object)
			{
				for (stringi_hash<as_member>::const_iterator it = init_object->m_members.begin(); 
					it != init_object->m_members.end(); ++it ) 
				{ 
					ch->set_member(it->first, it->second.get_member_value()); 
				} 
			}

		} 
		return ch;
	}

	void	sprite_instance::remove_display_object(const tu_string& name)
		// Remove the object with the specified name.
	{
		character* ch = m_display_list.get_character_by_name(name);
		if (ch)
		{
			// @@ TODO: should only remove movies that were created via clone_display_object --
			// apparently original movies, placed by anim events, are immune to this.
			remove_display_object(ch->get_depth(), ch->get_id());
		}
	}

	bool	sprite_instance::on_event(const event_id& id)
		// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object_interface>	this_ptr(this);

		bool called = false;

		// First, check for built-in event handler.
		{
			as_value	method;
			if (get_event_handler(id, &method))
			{
				// Dispatch.
				call_method0(method, &m_as_environment, this);

				called = true;
				// Fall through and call the function also, if it's defined!
				// (@@ Seems to be the behavior for mouse events; not tested & verified for
				// every event type.)
			}
		}

		// Check for member function.
		{
			// In ActionScript 2.0, event method names are CASE SENSITIVE.
			// In ActionScript 1.0, event method names are CASE INSENSITIVE.
			const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
			if (method_name.length() > 0)
			{
				as_value	method;
				if (get_member(method_name, &method))
				{
					call_method0(method, &m_as_environment, this);
					called = true;
				}
			}
		}

		return called;
	}


	/*sprite_instance*/
	//		virtual void	on_event_load()
	// Do the events that (appear to) happen as the movie
	// loads.  frame1 tags and actions are executed (even
	// before advance() is called).	 Then the onLoad event
	// is triggered.
	//		{
	//			execute_frame_tags(0);
	//			do_actions();
	//			on_event(event_id::LOAD);
	//		}

	// Do the events that happen when there is XML data waiting
	// on the XML socket connection.
	void	sprite_instance::on_event_xmlsocket_onxml()
	{
		log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		on_event(event_id::SOCK_XML);
	}

	// Do the events that (appear to) happen on a specified interval.
	void	sprite_instance::on_event_interval_timer()
	{
		log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		on_event(event_id::TIMER);
	}

	// Do the events that happen as a MovieClip (swf 7 only) loads.
	void	sprite_instance::on_event_load_progress()
	{
		log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		on_event(event_id::LOAD_PROGRESS);
	}

	/*sprite_instance*/
	const char*	sprite_instance::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object_interface>	this_ptr(this);

		return call_method_parsed(&m_as_environment, this, method_name, method_arg_fmt, args);
	}

	void	sprite_instance::attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
	{
		assert(m_parent == NULL);	// should only be called on the root movie.

		array<with_stack_entry>	dummy;
		as_value	obj = m_as_environment.get_variable(tu_string(path_to_object), dummy);
		as_object_interface*	as_obj = obj.to_object();
		if (as_obj)
		{
			movie*	m = as_obj->to_movie();
			if (m)
			{
				m->set_display_callback(callback, user_ptr);
			}
		}
	}
}
