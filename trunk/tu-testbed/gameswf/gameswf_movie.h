// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_MOVIE_H
#define GAMESWF_MOVIE_H

#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>

namespace gameswf
{
	struct movie_root;
	struct swf_event;

	struct movie : public movie_interface
	{
		virtual void set_extern_movie(movie_interface* m) { }
		virtual movie_interface*	get_extern_movie() { return NULL; }

		virtual movie_definition*	get_movie_definition() { return NULL; }
		virtual movie_root*		get_root() { return NULL; }
		virtual movie_interface*	get_root_interface() { return NULL; }
		virtual movie*			get_root_movie() { return NULL; }

		virtual float			get_pixel_scale() const { return 1.0f; }
		virtual character*		get_character(int id) { return NULL; }

		virtual matrix			get_world_matrix() const { return matrix::identity; }
		virtual cxform			get_world_cxform() const { return cxform::identity; }

		//
		// display-list management.
		//

		virtual execute_tag*	find_previous_replace_or_add_tag(int current_frame, int depth, int id)
		{
			return NULL;
		}

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

		virtual void	set_background_color(const rgba& color) {}
		virtual void	set_background_alpha(float alpha) {}
		virtual float	get_background_alpha() const { return 1.0f; }
		virtual void	set_display_viewport(int x0, int y0, int width, int height) {}

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }
		virtual void	do_actions(const array<action_buffer*>& action_list) { assert(0); }

		virtual void	goto_frame(int target_frame_number) { assert(0); }
		virtual bool	goto_labeled_frame(const char* label) { assert(0); return false; }

		virtual void	set_play_state(play_state s) {}
		virtual play_state	get_play_state() const { assert(0); return STOP; }

		virtual void	notify_mouse_state(int x, int y, int buttons)
			// The host app uses this to tell the movie where the
			// user's mouse pointer is.
		{
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
			// Use this to retrieve the last state of the mouse, as set via
			// notify_mouse_state().
		{
			assert(0);
		}

		struct drag_state
		{
			movie*	m_character;
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
		virtual void	get_drag_state(drag_state* st) { assert(0); *st = drag_state(); }
		virtual void	set_drag_state(const drag_state& st) { assert(0); }
		virtual void	stop_drag() { assert(0); }


		// External
		virtual void	set_variable(const char* path_to_var, const char* new_value)
		{
			assert(0);
		}

		// External
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value)
		{
			assert(0);
		}

		// External
		virtual const char*	get_variable(const char* path_to_var) const
		{
			assert(0);
			return "";
		}

		virtual void * get_userdata() { assert(0); return NULL; }
		virtual void set_userdata(void *) { assert(0); }

		// External
		virtual bool	has_looped() const { return true; }


		//
		// Mouse/Button interface.
		//

		virtual movie* get_topmost_mouse_entity(float x, float y) { return NULL; }
		virtual bool	get_track_as_menu() const { return false; }
		virtual void	on_button_event(event_id id) { on_event(id); }


		//
		// ActionScript.
		//


		virtual movie*	get_relative_target(const tu_string& name)
		{
			assert(0);	
			return NULL;
		}

		// ActionScript event handler.	Returns true if a handler was called.
//		virtual bool	on_event(const event_id& id) { return false; }

		int    add_interval_timer(void *timer)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
			return -1;	// ???
		}

		void	clear_interval_timer(int x)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		}

		virtual void	do_something(void *timer)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		}

		// Special event handler; sprites also execute their frame1 actions on this event.
//		virtual void	on_event_load() { on_event(event_id::LOAD); }

#if 0
		// tulrich: @@ is there a good reason these are in the
		// vtable?  I.e. can the caller just call
		// on_event(event_id::SOCK_DATA) instead of
		// on_event_xmlsocket_ondata()?
		virtual void	on_event_xmlsocket_ondata() { on_event(event_id::SOCK_DATA); }
		virtual void	on_event_xmlsocket_onxml() { on_event(event_id::SOCK_XML); }
		virtual void	on_event_interval_timer() { on_event(event_id::TIMER); }
		virtual void	on_event_load_progress() { on_event(event_id::LOAD_PROGRESS); }
#endif
		// as_object_interface stuff
		virtual bool	set_member(const tu_stringi& name, const as_value& val) { assert(0); return false; }
		virtual bool	get_member(const tu_stringi& name, as_value* val) { assert(0); return false; }

		virtual void	call_frame_actions(const as_value& frame_spec) { assert(0); }

		virtual movie*	to_movie() { return this; }

		virtual character*	clone_display_object(const tu_string& newname, Uint16 depth, as_object* init_object)
		{
			assert(0);
			return NULL; 
		}

		virtual void	remove_display_object(const tu_string& name) { assert(0); }

		// Forward vararg call to version taking va_list.
		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...)
		{
			va_list	args;
			va_start(args, method_arg_fmt);
			const char*	result = call_method_args(method_name, method_arg_fmt, args);
			va_end(args);

			return result;
		}

		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
			// Override this if you implement call_method.
		{
			assert(0);
			return NULL;
		}

		virtual void	execute_frame_tags(int frame, bool state_only = false) {}

		// External.
		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
		{
			assert(0);
		}

		virtual void	set_display_callback(void (*callback)(void*), void* user_ptr)
			// Override me to provide this functionality.
		{
		}

		virtual bool can_handle_mouse_event() { return false; }

	};


	// Information about how to display a character.
	struct display_info
	{
		movie*	m_parent;
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
	struct character : public movie
	{
		int		m_id;
		movie*		m_parent;
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

		character(movie* parent, int id)
			:
		m_id(id),
			m_parent(parent),
			m_depth(-1),
			m_ratio(0.0f),
			m_clip_depth(0),
			m_visible(true),
			m_display_callback(NULL),
			m_display_callback_user_ptr(NULL)

		{
			assert((parent == NULL && m_id == -1)
				|| (parent != NULL && m_id >= 0));
		}

		// Accessors for basic display info.
		int	get_id() const { return m_id; }
		movie*	get_parent() const { return m_parent; }
		void set_parent(movie* parent) { m_parent = parent; }  // for extern movie
		int	get_depth() const { return m_depth; }
		void	set_depth(int d) { m_depth = d; }
		const matrix&	get_matrix() const { return m_matrix; }
		void	set_matrix(const matrix& m)
		{
//			assert(m.is_valid());
			m_matrix = m;
		}
		const cxform&	get_cxform() const { return m_color_transform; }
		void	set_cxform(const cxform& cx) { m_color_transform = cx; }
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
			if (m_parent)
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
			if (m_parent)
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
		virtual float	get_height() { return 0; }
		virtual float	get_width() { return 0; }

		virtual movie*	get_root_movie() { return m_parent->get_root_movie(); }

		virtual int	get_current_frame() const { return -1; }
		virtual int	get_frame_count() const { return -1; }

		virtual bool	has_looped() const { assert(0); return false; }
		virtual void	restart() { /*assert(0);*/ }
		virtual void	advance(float delta_time) {}	// for buttons and sprites
		virtual void	goto_frame(int target_frame) {}
		virtual bool	get_accept_anim_moves() const { return true; }

		virtual void	get_drag_state(drag_state* st) { assert(m_parent); m_parent->get_drag_state(st); }

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

		virtual void	get_mouse_state(int* x, int* y, int* buttons) { get_parent()->get_mouse_state(x, y, buttons); }

		// Utility.
		void	do_mouse_drag();

		virtual bool has_keypress_event()
		{
			return false;
		}


		virtual bool	get_member(const tu_stringi& name, as_value* val)
		// Set *val to the value of the named member and
		// return true, if we have the named member.
		// Otherwise leave *val alone and return false.
		{
			as_standard_member	std_member = get_standard_member(name);
			switch (std_member)
			{
			default:
			case M_INVALID_MEMBER:
				break;
			case M_X:
				//if (name == "_x")
				{
					matrix	m = get_matrix();
					val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
					return true;
				}
			case M_Y:
				//else if (name == "_y")
				{
					matrix	m = get_matrix();
					val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
					return true;
				}
			case M_XSCALE:
				//else if (name == "_xscale")
				{
					matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
					float xscale = m.get_x_scale();
					val->set_double(xscale * 100);		// result in percent
					return true;
				}
			case M_YSCALE:
				//else if (name == "_yscale")
				{
					matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
					float yscale = m.get_y_scale();
					val->set_double(yscale * 100);		// result in percent
					return true;
				}
			case M_CURRENTFRAME:
				//else if (name == "_currentframe")
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
				//else if (name == "_totalframes")
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
			case M_ALPHA:
				//else if (name == "_alpha")
				{
					// Alpha units are in percent.
					val->set_double(get_cxform().m_[3][0] * 100.f);
					return true;
				}
			case M_VISIBLE:
				//else if (name == "_visible")
				{
					val->set_bool(get_visible());
					return true;
				}
			case M_WIDTH:
				//else if (name == "_width")
				{
					matrix	m = get_world_matrix();
					rect	transformed_rect;

					// @@ not sure about this...
					rect	source_rect;
					source_rect.m_x_min = 0;
					source_rect.m_y_min = 0;
					source_rect.m_x_max = (float) get_width();
					source_rect.m_y_max = (float) get_height();

					transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
					val->set_double(TWIPS_TO_PIXELS(transformed_rect.width()));
					return true;
				}
			case M_HEIGHT:
				//else if (name == "_height")
				{
					rect	transformed_rect;

					// @@ not sure about this...
					rect	source_rect;
					source_rect.m_x_min = 0;
					source_rect.m_y_min = 0;
					source_rect.m_x_max = (float) get_width();
					source_rect.m_y_max = (float) get_height();

					transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
					val->set_double(TWIPS_TO_PIXELS(transformed_rect.height()));
					return true;
				}
			case M_ROTATION:
				//else if (name == "_rotation")
				{
					// Verified against Macromedia player using samples/test_rotation.swf
					float	angle = get_matrix().get_rotation();

					// Result is CLOCKWISE DEGREES, [-180,180]
					angle *= 180.0f / float(M_PI);

					val->set_double(angle);
					return true;
				}
			case M_TARGET:
				//else if (name == "_target")
				{
					// Full path to this object; e.g. "/_level0/sprite1/sprite2/ourSprite"
					val->set_string("/_root");
					return true;
				}
			case M_FRAMESLOADED:
				//else if (name == "_framesloaded")
				{
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
			case M_NAME:
				//else if (name == "_name")
				{
					val->set_tu_string(get_name());
					return true;
				}
			case M_DROPTARGET:
				//else if (name == "_droptarget")
				{
					// Absolute path in slash syntax where we were last dropped (?)
					// @@ TODO
					val->set_string("/_root");
					return true;
				}
			case M_URL:
				//else if (name == "_url")
				{
					// our URL.
					val->set_string("gameswf");
					return true;
				}
			case M_HIGHQUALITY:
				//else if (name == "_highquality")
				{
					// Whether we're in high quality mode or not.
					val->set_bool(true);
					return true;
				}
			case M_FOCUSRECT:
				//else if (name == "_focusrect")
				{
					// Is a yellow rectangle visible around a focused movie clip (?)
					val->set_bool(false);
					return true;
				}
			case M_SOUNDBUFTIME:
				//else if (name == "_soundbuftime")
				{
					// Number of seconds before sound starts to stream.
					val->set_double(0.0);
					return true;
				}
			case M_XMOUSE:
				//else if (name == "_xmouse")
				{
					// Local coord of mouse IN PIXELS.
					int	x, y, buttons;
					get_mouse_state(&x, &y, &buttons);

					matrix	m = get_world_matrix();

					point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
					point	b;

					m.transform_by_inverse(&b, a);

					val->set_double(TWIPS_TO_PIXELS(b.m_x));
					return true;
				}
			case M_YMOUSE:
				//else if (name == "_ymouse")
				{
					// Local coord of mouse IN PIXELS.
					int	x, y, buttons;
					get_mouse_state(&x, &y, &buttons);

					matrix	m = get_world_matrix();

					point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
					point	b;

					m.transform_by_inverse(&b, a);

					val->set_double(TWIPS_TO_PIXELS(b.m_y));
					return true;
				}
			case M_PARENT:
				{
					val->set_as_object_interface(static_cast<as_object_interface*>(m_parent));
					return true;
				}
			}	// end switch

			return false;
		}

		virtual bool	set_member(const tu_stringi& name, const as_value& val)
		{
			as_standard_member	std_member = get_standard_member(name);
			switch (std_member)
			{
			default:
			case M_INVALID_MEMBER:
				break;
			case M_X:
				//if (name == "_x")
				{
					matrix	m = get_matrix();
					m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
					set_matrix(m);
					return true;
				}
			case M_Y:
				//else if (name == "_y")
				{
					matrix	m = get_matrix();
					m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
					set_matrix(m);
					return true;
				}
			case M_XSCALE:
				//else if (name == "_xscale")
				{
					matrix	m = get_matrix();

					// Decompose matrix and insert the desired value.
					float	x_scale = (float) val.to_number() / 100.f;	// input is in percent
					float	y_scale = m.get_y_scale();
					float	rotation = m.get_rotation();
					m.set_scale_rotation(x_scale, y_scale, rotation);

					set_matrix(m);
					return true;
				}
			case M_YSCALE:
				//else if (name == "_yscale")
				{
					matrix	m = get_matrix();

					// Decompose matrix and insert the desired value.
					float	x_scale = m.get_x_scale();
					float	y_scale = (float) val.to_number() / 100.f;	// input is in percent
					float	rotation = m.get_rotation();
					m.set_scale_rotation(x_scale, y_scale, rotation);

					set_matrix(m);
					return true;
				}
			case M_ALPHA:
				//else if (name == "_alpha")
				{
					// Set alpha modulate, in percent.
					cxform	cx = get_cxform();
					cx.m_[3][0] = float(val.to_number()) / 100.f;
					set_cxform(cx);
					return true;
				}
			case M_VISIBLE:
				//else if (name == "_visible")
				{
					set_visible(val.to_bool());
					return true;
				}
			case M_WIDTH:
				//else if (name == "_width")
				{
					// @@ tulrich: is parameter in world-coords or local-coords?
					matrix	m = get_matrix();
					m.m_[0][0] = float(PIXELS_TO_TWIPS(val.to_number()));
					float w = get_width();
					if (fabsf(w) > 1e-6f)
					{
						m.m_[0][0] /= w;
					}
					set_matrix(m);
					return true;
				}
			case M_HEIGHT:
				//else if (name == "_height")
				{
					// @@ tulrich: is parameter in world-coords or local-coords?
					matrix	m = get_matrix();
					m.m_[1][1] = float(PIXELS_TO_TWIPS(val.to_number()));
					float h = get_height();
					if (fabsf(h) > 1e-6f)
					{
						m.m_[1][1] /= h;
					}
					set_matrix(m);
					return true;
				}
			case M_ROTATION:
				//else if (name == "_rotation")
				{
					matrix	m = get_matrix();

					// Decompose matrix and insert the desired value.
					float	x_scale = m.get_x_scale();
					float	y_scale = m.get_y_scale();
					float	rotation = (float) val.to_number() * float(M_PI) / 180.f;	// input is in degrees
					m.set_scale_rotation(x_scale, y_scale, rotation);

					set_matrix(m);
					return true;
				}
			case M_HIGHQUALITY:
				//else if (name == "_highquality")
				{
					// @@ global { 0, 1, 2 }
					//				// Whether we're in high quality mode or not.
					//				val->set(true);
					return true;
				}
			case M_FOCUSRECT:
				//else if (name == "_focusrect")
				{
					//				// Is a yellow rectangle visible around a focused movie clip (?)
					//				val->set(false);
					return true;
				}
			case M_SOUNDBUFTIME:
				//else if (name == "_soundbuftime")
				{
					// @@ global
					//				// Number of seconds before sound starts to stream.
					//				val->set(0.0);
					return true;
				}
			}	// end switch

			return false;
		}

	};

}

#endif
