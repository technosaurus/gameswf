// gameswf_root.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_ROOT_H
#define GAMESWF_ROOT_H


#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include "gameswf_movie.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>

namespace gameswf
{
	struct movie_def_impl;

	//
	// Helper to generate mouse events, given mouse state & history.
	//

	struct mouse_button_state
	{
		weak_ptr<movie>	m_active_entity;	// entity that currently owns the mouse pointer
		weak_ptr<movie>	m_topmost_entity;	// what's underneath the mouse right now

		bool	m_mouse_button_state_last;		// previous state of mouse button
		bool	m_mouse_button_state_current;		// current state of mouse button

		bool	m_mouse_inside_entity_last;	// whether mouse was inside the active_entity last frame

		mouse_button_state()
			:
			m_mouse_button_state_last(0),
			m_mouse_button_state_current(0),
			m_mouse_inside_entity_last(false)
		{
		}
	};

	//
	// movie_root
	//
	// Global, shared root state for a movie and all its characters.
	//
	struct movie_root : public movie_interface
	{
		enum listener_type
		{
			KEYPRESS,
			ADVANCE,
			MOVIE_CLIP_LOADER
		};

		smart_ptr<movie_def_impl>	m_def;
		smart_ptr<movie>	m_movie;
		int			m_viewport_x0, m_viewport_y0, m_viewport_width, m_viewport_height;
		float			m_pixel_scale;

		rgba			m_background_color;
//		float			m_timer;
		int			m_mouse_x, m_mouse_y, m_mouse_buttons;
		void *			m_userdata;
		movie::drag_state	m_drag_state;	// @@ fold this into m_mouse_button_state?
		mouse_button_state m_mouse_button_state;
		bool			m_on_event_load_called;

		hash< smart_ptr<as_object_interface>, int > m_listeners;
		smart_ptr<movie> m_current_active_entity;
		float	m_time_remainder;
		float m_frame_time;

		movie_root(movie_def_impl* def);
		~movie_root();

		void	generate_mouse_button_events(mouse_button_state* ms);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual movie*	to_movie();
		void	set_root_movie(movie* root_movie);

		void	set_display_viewport(int x0, int y0, int w, int h);
		void	notify_mouse_state(int x, int y, int buttons);
		virtual void	get_mouse_state(int* x, int* y, int* buttons);
		movie*	get_root_movie();

		void	stop_drag();
		movie_definition*	get_movie_definition();

		uint32	get_file_bytes() const;
		uint32	get_loaded_bytes() const;

		virtual int	get_current_frame() const;
		float	get_frame_rate() const;

		virtual float	get_pixel_scale() const;

		character*	get_character(int character_id);
		void	set_background_color(const rgba& color);
		void	set_background_alpha(float alpha);
		float	get_background_alpha() const;
		void	advance(float delta_time);

		void	goto_frame(int target_frame_number);
		virtual bool	has_looped() const;

		void	display();

		virtual bool	goto_labeled_frame(const char* label);
		virtual void	set_play_state(play_state s);
		virtual play_state	get_play_state() const;
		virtual void	set_variable(const char* path_to_var, const char* new_value);
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value);
		virtual const char*	get_variable(const char* path_to_var) const;
		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...);
		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, 
			va_list args);

		virtual void	set_visible(bool visible);
		virtual bool	get_visible() const;

		virtual void* get_userdata();
		virtual void set_userdata(void * ud );

		virtual void	attach_display_callback(const char* path_to_object,
			void (*callback)(void* user_ptr), void* user_ptr);

		virtual int	get_movie_version();
		virtual int	get_movie_width();
		virtual int	get_movie_height();
		virtual float	get_movie_fps();

		virtual void	notify_key_event(key::code k, bool down);
		void add_listener(as_object_interface* listener, listener_type lt);
		void remove_listener(as_object_interface* listener);

		void	advance_listeners(float delta_time);

	};
}

#endif
