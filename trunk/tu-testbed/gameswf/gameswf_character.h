// gameswf_character.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_CHARACTER_H
#define GAMESWF_CHARACTER_H

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_types.h"
#include "gameswf/gameswf_log.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>

namespace gameswf
{
	struct movie_root;
	struct swf_event;
	struct as_mcloader;

	// Information about how to display a character.
	struct display_info
	{
		character*	m_parent;
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
		Uint16	m_clip_depth;

		display_info()
			:
			m_parent(NULL),
			m_depth(0),
			m_ratio(0.0f),
			m_clip_depth(0)
		{
		}

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			m_depth = di.m_depth;
			m_color_transform.concatenate(di.m_color_transform);
			m_matrix.concatenate(di.m_matrix);
			m_ratio = di.m_ratio;
			m_clip_depth = di.m_clip_depth;
		}
	};

	// character is a live, stateful instance of a character_def.
	// It represents a single active element in a movie.
	// internal interface
	struct character : public movie_interface
	{
		int		m_id;
		weak_ptr<character>		m_parent;
		tu_string	m_name;
		int		m_depth;
		cxform		m_color_transform;
		matrix		m_matrix;
		float		m_ratio;
		Uint16		m_clip_depth;
		bool		m_visible;
		hash<event_id, as_value>	m_event_handlers;
		void		(*m_display_callback)(void*);
		void*		m_display_callback_user_ptr;

		struct drag_state
		{
			character*	m_character;
			bool	m_lock_center;
			bool	m_bound;
			float	m_bound_x0;
			float	m_bound_y0;
			float	m_bound_x1;
			float	m_bound_y1;

			drag_state()
				:
				m_character(0), m_lock_center(0), m_bound(0),
				m_bound_x0(0), m_bound_y0(0), m_bound_x1(1), m_bound_y1(1)
			{
			}
		};

		bool m_is_alive;

		character(character* parent, int id);

		//
		// Mouse/Button interface.
		//
		virtual character* get_topmost_mouse_entity(float x, float y) { return NULL; }

		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		virtual void	notify_mouse_state(int x, int y, int buttons) {}
		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			get_parent()->get_mouse_state(x, y, buttons); 
		}

		// Utility.
		void	do_mouse_drag();

		virtual bool	get_track_as_menu() const { return false; }

		virtual float			get_pixel_scale() const { return 1.0f; }
		virtual character*		get_character(int id) { return NULL; }

		virtual movie_definition*	get_movie_definition()
		{
			character* ch = get_parent();
			if (ch)
			{
				return ch->get_movie_definition();
			}
			return NULL;
		}

		virtual as_object_interface*	find_exported_resource(const tu_string& symbol)
		{
			character* ch = get_parent();
			if (ch)
			{
				return ch->find_exported_resource(symbol);
			}
			return NULL;
		}


		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
		// Override this if you implement call_method.
		{
			assert(0);
			return NULL;
		}

		virtual void	set_play_state(play_state s) {}
		virtual play_state	get_play_state() const { assert(0); return STOP; }

		virtual bool	goto_labeled_frame(const char* label) { assert(0); return false; }

		//
		// display-list management.
		//

		virtual execute_tag*	find_previous_replace_or_add_tag(int current_frame, int depth, int id)
		{
			return NULL;
		}

		virtual void	clear_display_objects() { assert(0); }
		virtual void	remove_display_object(character* ch) { assert(0); }

		// replaces 'this' on md->create_instance()
		virtual character* replace_me(character_def*	def) { assert(0); return NULL; }
		virtual character* replace_me(movie_definition*	md) { assert(0); return NULL; }

		virtual character*	add_display_object(
			Uint16 character_id,
			const char*		 name,
			const array<swf_event*>& event_handlers,
			Uint16			 depth,
			bool			 replace_if_depth_is_occupied,
			const cxform&		 color_transform,
			const matrix&		 mat,
			float			 ratio,
			Uint16			clip_depth)
		{
			return NULL;
		}

		virtual void	move_display_object(
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	replace_display_object(
			Uint16		character_id,
			const char*	name,
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	replace_display_object(
			character*	ch,
			const char*	name,
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	remove_display_object(Uint16 depth, int id)	{}

		virtual character*	get_relative_target(const tu_string& name)
		{
			assert(0);	
			return NULL;
		}

		virtual void	execute_frame_tags(int frame, bool state_only = false) {}
		virtual character* cast_to_character() { return this; }

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }
		virtual void	do_actions(const array<action_buffer*>& action_list) { assert(0); }

		virtual character*	clone_display_object(const tu_string& newname, Uint16 depth)
		{
			assert(0);
			return NULL; 
		}

		virtual void	remove_display_object(const tu_string& name) { assert(0); }

		virtual void	get_drag_state(drag_state* st) { assert(m_parent != 0); m_parent->get_drag_state(st); }
		virtual void	set_drag_state(const drag_state& st) { assert(0); }
		virtual void	stop_drag() { assert(0); }
		virtual movie_interface*	get_root_interface() { return NULL; }
		virtual void	call_frame_actions(const as_value& frame_spec) { assert(0); }

		virtual void	set_background_color(const rgba& color) {}
		virtual void	set_background_alpha(float alpha) {}
		virtual float	get_background_alpha() const { return 1.0f; }
		virtual void	set_display_viewport(int x0, int y0, int width, int height) {}

		// Forward vararg call to version taking va_list.
		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...)
		{
			va_list	args;
			va_start(args, method_arg_fmt);
			const char*	result = call_method_args(method_name, method_arg_fmt, args);
			va_end(args);

			return result;
		}

		//
		// external
		//

		virtual void * get_userdata() { assert(0); return NULL; }
		virtual void set_userdata(void *) { assert(0); }

		virtual void	set_variable(const char* path_to_var, const char* new_value)
		{
			assert(0);
		}

		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value)
		{
			assert(0);
		}

		virtual const char*	get_variable(const char* path_to_var) const
		{
			assert(0);
			return "";
		}

		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
		{
			assert(0);
		}

		// Accessors for basic display info.

		int	get_id() const { return m_id; }
		character*	get_parent() const { return m_parent.get_ptr(); }
		void set_parent(character* parent) { m_parent = parent; }  // for extern movie
		int	get_depth() const { return m_depth; }
		void	set_depth(int d) { m_depth = d; }
		const matrix&	get_matrix() const { return m_matrix; }
		void	set_matrix(const matrix& m)
		{
			m_matrix = m;
		}
		const cxform&	get_cxform() const 
		{ 
			return m_color_transform; 
		}
		void	set_cxform(const cxform& cx)
		{
			m_color_transform = cx;
		}
		void	concatenate_cxform(const cxform& cx) { m_color_transform.concatenate(cx); }
		void	concatenate_matrix(const matrix& m) { m_matrix.concatenate(m); }
		float	get_ratio() const { return m_ratio; }
		void	set_ratio(float f) { m_ratio = f; }
		Uint16	get_clip_depth() const { return m_clip_depth; }
		void	set_clip_depth(Uint16 d) { m_clip_depth = d; }

		void	set_name(const char* name) { m_name = name; }
		const tu_string&	get_name() const { return m_name; }

		// For edit_text support (Flash 5).  More correct way
		// is to do "text_character.text = whatever", via
		// set_member().
		virtual const char*	get_text_name() const { return ""; }
		virtual void	set_text_value(const char* new_text) { assert(0); }

		virtual matrix	get_world_matrix() const
			// Get our concatenated matrix (all our ancestor transforms, times our matrix).	 Maps
			// from our local space into "world" space (i.e. root movie space).
		{
			matrix	m;
			if (m_parent != NULL)
			{
				m = m_parent->get_world_matrix();
			}
			m.concatenate(get_matrix());

			return m;
		}

		virtual cxform	get_world_cxform() const
		// Get our concatenated color transform (all our ancestor transforms,
		// times our cxform).  Maps from our local space into normal color space.
		{
			cxform	m;
			if (m_parent != NULL)
			{
				m = m_parent->get_world_cxform();
			}
			m.concatenate(get_cxform());

			return m;
		}

		// Event handler accessors.

		const hash<event_id, as_value>* get_event_handlers() const 
		{ 
			return &m_event_handlers; 
		} 

		bool	get_event_handler(event_id id, as_value* result)
		{
			return m_event_handlers.get(id, result);
		}
		void	set_event_handler(event_id id, const as_value& method)
		{
			m_event_handlers.set(id, method);
		}

		// Movie interfaces.  By default do nothing.  sprite_instance and some others override these.
		virtual void	display() {}
		virtual float	get_height();
		virtual float	get_width();
		virtual void get_bound(rect* bound);
		virtual character_def* get_character_def() = 0;

		virtual character*	get_root_movie() { return m_parent->get_root_movie(); }

		virtual int	get_current_frame() const { return -1; }
		virtual int	get_frame_count() const { return -1; }
		virtual int get_loading_frame() const { return -1; }

		virtual bool	has_looped() const { assert(0); return false; }
		virtual void	advance(float delta_time) {}	// for buttons and sprites
		virtual void	goto_frame(int target_frame) {}
		virtual bool	get_accept_anim_moves() const { return true; }

		virtual void	set_visible(bool visible) { m_visible = visible; }
		virtual bool	get_visible() const { return m_visible; }

		virtual void	set_display_callback(void (*callback)(void*), void* user_ptr)
		{
			m_display_callback = callback;
			m_display_callback_user_ptr = user_ptr;
		}

		virtual void	do_display_callback()
		{
			if (m_display_callback)
			{
				(*m_display_callback)(m_display_callback_user_ptr);
			}
		}

		virtual bool has_keypress_event()
		{
			return false;
		}

		virtual bool can_handle_mouse_event() { return false; }
	
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);

		virtual character*	find_target(const tu_string& path) const;
		virtual character*	find_target(const as_value& target) const;

		virtual bool	is_visible();

	};

}

#endif
