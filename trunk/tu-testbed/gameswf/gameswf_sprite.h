// gameswf_sprite.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_SPRITE_H
#define GAMESWF_SPRITE_H


#include "gameswf/gameswf.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_as_classes/as_mcloader.h"
#include "gameswf/gameswf_dlist.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_sprite_def.h"
#include "gameswf/gameswf_types.h"
#include "gameswf/gameswf_canvas.h"

#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>

namespace gameswf
{

	struct sprite_instance : public character
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_SPRITE };
		virtual bool is(int class_id)
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		smart_ptr<movie_definition_sub>	m_def;
		root*	m_root;

		display_list	m_display_list;
		array<action_buffer*>	m_action_list;
		array<action_buffer*>	m_goto_frame_action_list;

		play_state	m_play_state;
		int		m_current_frame;
		bool		m_update_frame;
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
		smart_ptr<character> m_canvas;

		sprite_instance(movie_definition_sub* def, root* r, character* parent, int id);
		virtual ~sprite_instance();

		virtual character_def* get_character_def() { return m_def.get_ptr();	}
		virtual bool has_keypress_event();
//		character*	get_root_interface() { return m_root; }
		root*	get_root() { return m_root; }

		// used in loadMovieClip()
		void	set_root(root* mroot) { m_root = mroot; }
		uint32	get_file_bytes() const;
		uint32	get_loaded_bytes() const;

		character*	get_root_movie() { return m_root->get_root_movie(); }
		movie_definition*	get_movie_definition() { return m_def.get_ptr(); }

		virtual void get_bound(rect* bound);

		virtual int	get_current_frame() const { return m_current_frame; }
		virtual int	get_frame_count() const { return m_def->get_frame_count(); }
		virtual int get_loading_frame() const { return m_def->get_loading_frame(); }

		character* add_empty_movieclip(const char* name, int depth);

		void	set_play_state(play_state s);
		play_state	get_play_state() const;

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

//		virtual bool	has_looped() const { return m_has_looped; }

		inline int	transition(int a, int b) const
			// Combine the flags to avoid a conditional. It would be faster with a macro.
		{
			return (a << 2) | b;
		}

		virtual bool can_handle_mouse_event();
		virtual character*	get_topmost_mouse_entity(float x, float y);
		void advance(float delta_time);

		void	execute_frame_tags(int frame, bool state_only = false);
		void	execute_frame_tags_reverse(int frame);
		execute_tag*	find_previous_replace_or_add_tag(int frame, int depth, int id);
		void	execute_remove_tags(int frame);
		void	do_actions();
		virtual void do_actions(const array<action_buffer*>& action_list);
		void	goto_frame(const tu_string& target_frame);
		void	goto_frame(int target_frame_number);
		bool	goto_labeled_frame(const char* label);

		void	display();

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
		void	remove_display_object(const tu_string& name);
		void	remove_display_object(character* ch);
		void	clear_display_objects();

		virtual character* replace_me(movie_definition*	md);
		virtual character* replace_me(character_def*	def);

		void	add_action_buffer(action_buffer* a);
		int	get_id_at_depth(int depth);
		int	get_highest_depth();

		//
		// ActionScript support
		//

		virtual void	set_variable(const char* path_to_var, const char* new_value);
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value);
		virtual const char*	get_variable(const char* path_to_var) const;

//		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual character*	get_relative_target(const tu_string& name);
		virtual void	call_frame_actions(const as_value& frame_spec);
		virtual void	set_drag_state(const drag_state& st);
		virtual void	stop_drag();
		virtual void	get_drag_state(drag_state* st);
		character*	clone_display_object(const tu_string& newname, Uint16 depth);
		virtual bool	on_event(const event_id& id);
		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args);
		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr);
		bool	hit_test(character* target);
		sprite_instance* attach_movie(const tu_string& id, const tu_string name, int depth);

		virtual	void enumerate(as_environment* env);

		character* create_text_field(const char* name, int depth, int x, int y, int width, int height);

		virtual character*	find_target(const tu_string& path) const;

		virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
		virtual as_environment*	get_environment() { return &m_as_environment; }
		virtual void dump();

		virtual character_def*	find_exported_resource(const tu_string& symbol);

		// gameSWF extension
		void	sprite_instance::set_fps(float fps);

		// drawing API
		canvas* get_canvas();

	};
}

#endif
