// gameswf_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_impl.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_sprite_def.h"
#include "gameswf/gameswf_sprite.h"
#include "gameswf/gameswf_as_sprite.h"
#include "gameswf/gameswf_text.h"
#include "gameswf/gameswf_as_classes/as_string.h"

namespace gameswf
{

	struct as_mcloader;

	const char*	next_slash_or_dot(const char* word);
	void	execute_actions(as_environment* env, const array<action_buffer*>& action_list);

	// this stuff should be high optimized
	// thus I can't use here set_member(...);
	sprite_instance::sprite_instance(movie_definition_sub* def,	root* r, 
		character* parent, int id)
		:
		character(parent, id),
		m_def(def),
		m_root(r),
		m_play_state(PLAY),
		m_current_frame(0),
		m_update_frame(true),
		m_mouse_state(UP),
		m_enabled(true),
		m_on_event_load_called(false)
	{
		assert(m_def != NULL);
		assert(m_root != NULL);

		//m_root->add_ref();	// @@ circular!
		m_as_environment.set_target(this);
		
		// Initialize the flags for init action executed.
		m_init_actions_executed.resize(m_def->get_frame_count());
		memset(&m_init_actions_executed[0], 0,
			sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());

		get_heap()->set(this, false);
	}

	sprite_instance::~sprite_instance()
	{
//		printf("delete sprite 0x%p\n", this);
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

	void sprite_instance::get_bound(rect* bound)
	{
		int i, n = m_display_list.size();
		if (n == 0)
		{
			return;
		}

		bound->m_x_min = FLT_MAX;
		bound->m_x_max = - FLT_MAX;
		bound->m_y_min = FLT_MAX;
		bound->m_y_max = - FLT_MAX;

		matrix m = get_matrix();
		for (i = 0; i < n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch != NULL)
			{
				rect ch_bound;
				ch->get_bound(&ch_bound);

				m.transform(&ch_bound);

				bound->expand_to_rect(ch_bound);
			}
		}
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
		// pauses stream sound
		sound_handler* sound = get_sound_handler();
		if (sound)
		{
			if (m_def->m_ss_id >= 0)
			{
				sound->pause(m_def->m_ss_id, m_play_state == PLAY ? true : false);
			}
		}
		m_play_state = s;
	}

	sprite_instance::play_state	sprite_instance::get_play_state() const
	{
		return m_play_state; 
	}

	// Functions that qualify as mouse event handlers.
	static const tu_stringi FN_NAMES[] =
	{
		"onKeyPress",
		"onRelease",
		"onDragOver",
		"onDragOut",
		"onPress",
		"onReleaseOutside",
		"onRollout",
		"onRollover",
	};

	bool sprite_instance::can_handle_mouse_event()
	// Return true if we have any mouse event handlers.
	{

		as_value dummy;
		int i;
		for (i = 0; i < TU_ARRAYSIZE(FN_NAMES); i++)
		{
			if (get_member(FN_NAMES[i], &dummy)) 
			{
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

		for (i = 0; i < TU_ARRAYSIZE(EH_IDS); i++)
		{
			if (get_event_handler(EH_IDS[i], &dummy))
			{
				return true;
			}
		}
		return false;
	}


	character*	sprite_instance::get_topmost_mouse_entity(float x, float y)
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

		character*	top_te = NULL;
		character* te = NULL;
		bool this_has_focus = false;
		int i, n = m_display_list.size();

		// Go backwards, to check higher objects first.
		for (i = n - 1; i >= 0; i--)
		{
			character* ch = m_display_list.get_character(i);

			if (ch != NULL && ch->get_visible())
			{
				te = ch->get_topmost_mouse_entity(p.m_x, p.m_y);
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

		// else character which has event is closest to root
		if (top_te)
		{
			return top_te;
		}

		// else if we have focus then return not NULL
		return te;
	}


	// This code is very tricky and hard to get right.  It should
	// only be changed when verified by an automated test.  Here
	// is the procedure:
	//
	// 1. Identify a bug or desired feature.
	//
	// 2. Create a .swf file that isolates the bug or new feature.
	// The .swf should use trace() statements to log what it is
	// doing and define the correct behavior.
	//
	// 3. Collect the contents of flashlog.txt from the standalone
	// Macromedia flash player.  Create a new test file under
	// tests/ where the first line is the name of the new test
	// .swf, and the rest of the file is the correct trace()
	// output.
	//
	// 4. Verify that gameswf fails the new test (by running
	// ./gameswf_batch_test.py tests/name_of_new_test.txt)
	//
	// 5. Fix gameswf behavior.  Add the new test to
	// passing_tests[] in gameswf_batch_test.py.
	//
	// 6. Verify that all the other passing tests still pass!
	// (Run ./gameswf_batch_test.py and make sure all tests are
	// OK!  If not, then the new behavior is actually a
	// regression.)
	void sprite_instance::advance(float delta_time)
	{
		// child movieclip frame rate is the same the root movieclip frame rate
		// that's why it is not needed to analyze 'm_time_remainder'
		if (m_on_event_load_called == false)
		{
			// clip sprite onload 
			// _root.onLoad() will not be executed since do_actions()
			// for frame(0) has not executed yet.
			// _root.onLoad() will be executed later in root::advance()
			m_def->instanciate_registered_class(this);
			on_event(event_id::LOAD);
		}

		// mouse drag. 
		character::do_mouse_drag();

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
				// Macromedia Flash does not call remove display object tag
				// for 1-st frame therefore we should do it for it :-)
				if (m_current_frame == 0 && m_def->get_frame_count() > 1)
				{
					// affected depths
					const array<execute_tag*>& playlist = m_def->get_playlist(0);
					array<int> affected_depths;
					for (int i = 0; i < playlist.size(); i++)
					{
						int depth = (playlist[i]->get_depth_id_of_replace_or_add_tag()) >> 16;
						if (depth != -1)
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

		if (m_on_event_load_called)
		{
			on_event(event_id::ENTER_FRAME);
		}

		do_actions();

		// Advance everything in the display list.
		m_display_list.advance(delta_time);

		m_on_event_load_called = true;

		// 'this' and its variables is not garbage
		not_garbage();
		get_heap()->set(this, false);

	}

	void	sprite_instance::execute_frame_tags(int frame, bool state_only)
	// Execute the tags associated with the specified frame.
	// frame is 0-based
	{
		// Keep this (particularly m_as_environment) alive during execution!
		smart_ptr<as_object>	this_ptr(this);

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

		// start stream sound
		if (state_only == false)
		{
			sound_handler* sound = get_sound_handler();
			if (sound)
			{
				if (m_def->m_ss_start == frame)
				{
					if (m_def->m_ss_id >= 0)
					{
						sound->stop_sound(m_def->m_ss_id);
						sound->play_sound(m_def->m_ss_id, 0);
					}
				}
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
		smart_ptr<as_object>	this_ptr(this);

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
		smart_ptr<as_object>	this_ptr(this);

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

	void	sprite_instance::goto_frame(const tu_string& target_frame)
	{
		// Flash tries to convert STRING to NUMBER,
		// if the conversion is OK then Flash uses this NUMBER as target_frame.
		// else uses arg as label of target_frame
		// Thanks Francois Guibert
		double number_value;

		// try string as number
		if (string_to_number(&number_value, target_frame.c_str()))
		{
			goto_frame((int) number_value - 1);	// Convert to 0-based
		}
		else
		{
			goto_labeled_frame(target_frame.c_str());
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
		}
		else if (target_frame_number > m_current_frame)
		{
			for (int f = m_current_frame + 1; f < target_frame_number; f++)
			{
				execute_frame_tags(f, true);
			}
			m_action_list.clear();
			execute_frame_tags(target_frame_number, false);
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

		if (is_visible() == false)
		{
			// We're invisible, so don't display!
			return;
		}

		// force advance just loaded (by loadMovie(...)) movie
		if (m_on_event_load_called == false)
		{
			advance(1);
		}

		m_display_list.display();

		do_display_callback();
	}

	character*	sprite_instance::add_display_object(
		Uint16 character_id,
		const char* name,
		const array<swf_event*>& event_handlers,
		int depth,
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

		// child clip only
		ch->on_event(event_id::CONSTRUCT);	// tested, ok

		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get_ptr();
	}


	/*sprite_instance*/
	void	sprite_instance::move_display_object(
		int depth,
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
		int depth,
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

		ch->on_event(event_id::CONSTRUCT);	// isn't tested
	}

	character* sprite_instance::replace_me(character_def*	def)
	{
		assert(def);
		character* parent = get_parent();

		// is 'this' root ?
		if (parent == NULL)
		{
			log_error("character can't replace _root\n");
			return NULL;
		}

		character* ch = def->create_character_instance(parent, 0);

		ch->set_parent(parent);
		parent->replace_display_object(
			ch,
			get_name(),
			get_depth(),
			false,
			get_cxform(),
			false,
			get_matrix(),
			get_ratio(),
			get_clip_depth());

		ch->on_event(event_id::CONSTRUCT);	// isn't tested

		return ch;
	}

	character* sprite_instance::replace_me(movie_definition*	md)
	{
		assert(md);
		character* parent = get_parent();

		// is 'this' root ?
		if (parent == NULL)
		{
			root* new_inst = md->create_instance();
			character* ch = new_inst->get_root_movie();
			set_current_root(new_inst);

//			ch->on_event(event_id::LOAD);
			return ch;
		}

		sprite_instance* sprite = new sprite_instance(cast_to<movie_def_impl>(md), 
			get_root(),	parent,	-1);

		sprite->set_parent(parent);
		sprite->set_root(get_root());

		parent->replace_display_object(
			sprite,
			get_name(),
			get_depth(),
			false,
			get_cxform(),
			false,
			get_matrix(),
			get_ratio(),
			get_clip_depth());

		return sprite;
	}

	void	sprite_instance::replace_display_object(
		character* ch,
		const char* name,
		int depth,
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

	void	sprite_instance::remove_display_object(int depth, int id)
	// Remove the object at the specified depth.
	// If id != -1, then only remove the object at depth with matching id.
	{
		m_display_list.remove_display_object(depth, id);
	}

	void	sprite_instance::clear_display_objects()
	// Remove all display objects
	{
		m_display_list.clear();
	}

	void	sprite_instance::add_action_buffer(action_buffer* a)
	// Add the given action buffer to the list of action
	// buffers to be processed at the end of the next
	// frame advance.
	{
		m_action_list.push_back(a);
	}

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

	int	sprite_instance::get_highest_depth()
	{
		return m_display_list.get_highest_depth();
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

	// useful for catching of the calls
	bool	sprite_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		return character::set_member(name, val);
	}

	bool	sprite_instance::get_member(const tu_stringi& name, as_value* val)
	// Set *val to the value of the named member and
	// return true, if we have the named member.
	// Otherwise leave *val alone and return false.
	{

		// first try built-ins sprite methods
		if (get_builtin(BUILTIN_SPRITE_METHOD, name, val))
		{
			return true;
		}

		// then try built-ins sprite properties
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			case M_CURRENTFRAME:
			{
				int n = get_current_frame();
				if (n >= 0)
				{
					val->set_int(n + 1);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			case M_TOTALFRAMES:
			{
				// number of frames.  Read only.
				int n = get_frame_count();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			case M_FRAMESLOADED:
			{
				int n = get_loading_frame();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			default:
				break;
		}

		// Not a built-in property.  Check items on our display list.
		character*	ch = m_display_list.get_character_by_name_i(name);
		if (ch)
		{
			// Found object.
			val->set_as_object(ch);
			return true;
		}

		// finally try standart character properties & movieclip variables
		return character::get_member(name, val);
	}

	character*	sprite_instance::get_relative_target(const tu_string& name)
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
		if (name.size() == 0)
		{
			return this;
		}

		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			case MDOT:
			case M_THIS:
				return this;

			case MDOT2:	// ".."
			case M_PARENT:
				return get_parent();

			case M_LEVEL0:
			case M_ROOT:
				return m_root->m_movie.get_ptr();

			default:
				break;
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
		if (frame_spec.is_string())
		{
			if (m_def->get_labeled_frame(frame_spec.to_string(), &frame_number) == false)
			{
				// Try converting to integer.
				frame_number = frame_spec.to_int();
			}
		}
		else
		{
			// convert from 1-based to 0-based
			frame_number = frame_spec.to_int() - 1;
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


	character*	sprite_instance::clone_display_object(const tu_string& newname, int depth)
	// Duplicate the object with the specified name and add it with a new name 
	// at a new depth.
	{

		if (get_parent() == NULL)
		{
			log_error("can't clone _root\n");
			return NULL;
		}

		// Create the copy of 'this' event handlers
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

		sprite_instance* parent = cast_to<sprite_instance>(get_parent());
		character* ch = NULL; 
		if (parent != NULL) 
		{ 
			// clone a previous external loaded movie ?
			if (get_id() == -1)	
			{
				ch = new sprite_instance(cast_to<movie_def_impl>(m_def.get_ptr()), 
					get_root(),	parent,	-1);

				ch->set_parent(parent);
				cast_to<sprite_instance>(ch)->set_root(get_root());
				ch->set_name(newname.c_str());

				parent->m_display_list.add_display_object(
					ch, 
					depth,
					true,		// replace_if_depth_is_occupied
					get_cxform(), 
					get_matrix(), 
					get_ratio(), 
					get_clip_depth()); 

				// Attach event handlers (if any).
				//TODO: test it
				for (int i = 0, n = event_handlers.size(); i < n; i++)
				{
					event_handlers[i]->attach_to(ch);
				}
			}
			else if( get_id() == 0 )
			{
				smart_ptr<sprite_instance> sprite;

				sprite = new sprite_instance(m_def.get_ptr(), 
					get_root(),	parent,	0);

				sprite->set_parent(parent);
				sprite->set_root(get_root());
				sprite->set_name(newname.c_str());

				*sprite->get_canvas() = *get_canvas();

				parent->m_display_list.add_display_object(
					sprite.get_ptr(), 
					depth,
					true,		// replace_if_depth_is_occupied
					get_cxform(), 
					get_matrix(), 
					get_ratio(), 
					get_clip_depth()); 
			}
			else
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

	void	sprite_instance::remove_display_object(character* ch)
	// Remove the object with the specified pointer.
	{
		m_display_list.remove_display_object(ch);
	}

	bool	sprite_instance::on_event(const event_id& id)
	// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object>	this_ptr(this);

		bool called = false;

		// First, check for built-in event handler.
		{
			as_value	method;
			if (get_event_handler(id, &method))
			{
				// Dispatch.
				gameswf::call_method(method, &m_as_environment, this, 0, m_as_environment.get_top_index());

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
					int nargs = 0;
					if (id.m_args)
					{
						nargs = id.m_args->size();
						for (int i = nargs - 1; i >=0; i--)
						{
							m_as_environment.push((*id.m_args)[i]);
						}
					}
					gameswf::call_method(method, &m_as_environment, this, nargs, 
						m_as_environment.get_top_index());
					m_as_environment.drop(nargs);

					called = true;
				}
			}
		}

		return called;
	}

	const char*	sprite_instance::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object>	this_ptr(this);

		return call_method_parsed(&m_as_environment, this, method_name, method_arg_fmt, args);
	}

	void	sprite_instance::attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
	{
		assert(m_parent == NULL);	// should only be called on the root movie.

		array<with_stack_entry>	dummy;
		as_value	obj = m_as_environment.get_variable(tu_string(path_to_object), dummy);

		character*	m = cast_to<character>(obj.to_object());
		if (m)
		{
			m->set_display_callback(callback, user_ptr);
		}
	}

	bool	sprite_instance::hit_test(character* ch)
	{
		as_value val;
		rect r;

		matrix	m = get_world_matrix();
		r.m_x_min = m.m_[0][2];
		r.m_y_min = m.m_[1][2];
		r.m_x_max = r.m_x_min + get_width();
		r.m_y_max = r.m_y_min + get_height();

		rect ch_r;

		m = ch->get_world_matrix();
		ch_r.m_x_min = m.m_[0][2];
		ch_r.m_y_min = m.m_[1][2];
		ch_r.m_x_max = ch_r.m_x_min + ch->get_width();
		ch_r.m_y_max = ch_r.m_y_min + ch->get_height();

		if (r.point_test(ch_r.m_x_min, ch_r.m_y_min) ||
			r.point_test(ch_r.m_x_min, ch_r.m_y_max) ||
			r.point_test(ch_r.m_x_max, ch_r.m_y_min) ||
			r.point_test(ch_r.m_x_max, ch_r.m_y_max))
		{
			return true;
		}

		return false;
	}

	uint32	sprite_instance::get_file_bytes() const
	{
		movie_def_impl* root_def = cast_to<movie_def_impl>(m_def.get_ptr());
		if (root_def)
		{
			return root_def->get_file_bytes();
		}
		return 0;
	}

	uint32	sprite_instance::get_loaded_bytes() const
	{
		movie_def_impl* root_def = cast_to<movie_def_impl>(m_def.get_ptr());
		if (root_def)
		{
			return root_def->get_loaded_bytes();
		}
		return 0;
	}

	character* sprite_instance::create_text_field(const char* name, int depth, int x, int y, int width, int height)
	// Creates a new, empty text field as a child of the movie clip
	{
		edit_text_character_def* textdef = new edit_text_character_def(width, height);

		character* textfield = textdef->create_character_instance(this, 0);
		textfield->set_name(name);

		matrix m;
		m.concatenate_translation(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));

		cxform color_transform;
		m_display_list.add_display_object(textfield, depth, true, color_transform, m, 0.0f, 0); 

		textfield->on_event(event_id::CONSTRUCT);	// isn't tested
		return textfield;
	}

	character*	sprite_instance::find_target(const as_value& target)
	// Find the sprite/movie referenced by the given path.
	{
		if (target.is_string())
		{
			const tu_string& path = target.to_tu_string();
			if (path.length() == 0)
			{
				return this;
			}

			assert(path.length() > 0);

			character*	tar = this;
			
			const char*	p = path.c_str();
			tu_string	subpart;

			if (*p == '/')
			{
				// Absolute path.  Start at the root.
				tar = get_root_movie();
				p++;
			}

			for (;;)
			{
				const char*	next_slash = next_slash_or_dot(p);
				subpart = p;
				if (next_slash == p)
				{
					log_error("error: invalid path '%s'\n", path.c_str());
					break;
				}
				else if (next_slash)
				{
					// Cut off the slash and everything after it.
					subpart.resize(int(next_slash - p));
				}

				tar = tar->get_relative_target(subpart);
				//@@   _level0 --> root, .. --> parent, . --> this, other == character

				if (tar == NULL || next_slash == NULL)
				{
					break;
				}

				p = next_slash + 1;
			}
			return tar;
		}
		else
		{
			return cast_to<character>(target.to_object());
		}

	}

	void	sprite_instance::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		m_display_list.clear_refs(visited_objects, this_ptr);
		as_object::clear_refs(visited_objects, this_ptr);

		// clear self-refs from environment
		for (int i = 0, n = m_as_environment.m_local_frames.size(); i < n; i++)
		{
			as_object* obj = m_as_environment.m_local_frames[i].m_value.to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					m_as_environment.m_local_frames[i].m_value.set_undefined();
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
			}
		}

	}

	sprite_instance* sprite_instance::attach_movie(const tu_string& id, 
		const tu_string name, 
		int depth)
	{

		// check the import.
		character_def* res = find_exported_resource(id);
		if (res == NULL)
		{
			IF_VERBOSE_ACTION(log_msg("import error: resource '%s' is not exported\n", id.c_str()));
			return NULL;
		}

		sprite_definition* sdef = cast_to<sprite_definition>(res);
		if (sdef == NULL)
		{
			return NULL;
		}

		sprite_instance* sprite = new sprite_instance(sdef, get_root(), this, -1);
		sprite->set_name(name.c_str());

		m_display_list.add_display_object(
			sprite,
			depth,
			true,
			m_color_transform,
			m_matrix,
			0.0f,
			0); 

		sprite->advance(1);	// force advance
		return sprite;
	}

	void	sprite_instance::dump()
	{
		printf("*** dump of sprite 0x%p ***\n", this);
		as_object::dump();
		printf("*** dump of sprite displaylist ***\n");
		m_display_list.dump();
		printf("*** end of sprite dump ***\n");
	}

	character_def*	sprite_instance::find_exported_resource(const tu_string& symbol)
	{
		movie_definition_sub*	def = cast_to<movie_def_impl>(get_movie_definition());
		if (def)
		{
			character_def* res = def->get_exported_resource(symbol);
			if (res)
			{
				return res;
			}
		}

		// try parent 
		character* parent = get_parent();
		if (parent)
		{
			return parent->find_exported_resource(symbol);
		}

		IF_VERBOSE_ACTION(log_msg("can't find exported resource '%s'\n", symbol.c_str()));
		return NULL;
	}

	void	sprite_instance::set_fps(float fps)
	{
		get_root()->set_frame_rate(fps);
	}

	canvas* sprite_instance::get_canvas()
	{
		if (m_canvas == NULL)
		{
			canvas* canvas_def = new canvas();
			m_canvas = canvas_def->create_character_instance(this, -1);

			m_display_list.add_display_object(
				m_canvas.get_ptr(),
				get_highest_depth(),
				true,
				m_color_transform,
				m_matrix,
				0.0f, 0); 
		}
		return cast_to<canvas>(m_canvas->get_character_def());
	}

	bool sprite_instance::is_instance_of(const as_function& constructor) const
	{
		const as_c_function * function = cast_to<as_c_function>(&constructor);
		if( function && function->m_func == as_global_movieclip_ctor )
		{
			return true;
		}

		return as_object::is_instance_of(constructor);
	}

	void sprite_instance::enumerate(as_environment* env)
	{
		assert(env);

		// enumerate variables
		character::enumerate(env);

		// enumerate characters
		for (int i = 0, n = m_display_list.size(); i<n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch)
			{
				const tu_string& name = ch->get_name();
				if (name.size() > 0)
				{
					env->push(name);
				}
			}
		}
	}
}
