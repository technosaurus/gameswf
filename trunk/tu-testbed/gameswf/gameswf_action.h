// gameswf_action.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#ifndef GAMESWF_ACTION_H
#define GAMESWF_ACTION_H

#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf_environment.h"
#include "gameswf/gameswf_object.h"
#include "gameswf/gameswf_types.h"
#include "base/container.h"
#include "base/smart_ptr.h"

namespace gameswf
{
	struct as_environment;
	struct as_as_function;

	as_object* get_global();

	// Clean up any stray heap stuff we've allocated.
	void	action_clear();

	hash<smart_ptr<as_object_interface>, bool>* get_garbage();
	void clear_garbage();

	// Dispatching methods from C++.
	as_value	call_method(
		const as_value& method,	as_environment* env, as_object_interface* this_ptr,
		int nargs, int first_arg_bottom_index);

	as_value	call_method0(const as_value& method, as_environment* env, as_object_interface* this_ptr);

	const char*	call_method_parsed(
		as_environment* env,
		as_object_interface* this_ptr,
		const char* method_name,
		const char* method_arg_fmt,
		va_list args);

	//
	// event_id
	//
	// For keyDown and stuff like that.

	struct event_id
	{
		// These must match the function names in event_id::get_function_name()
		enum id_code
		{
			INVALID,

			// These are for buttons & sprites.
			PRESS,
			RELEASE,
			RELEASE_OUTSIDE,
			ROLL_OVER,
			ROLL_OUT,
			DRAG_OVER,
			DRAG_OUT,
			KEY_PRESS,

			// These are for sprites only.
			INITIALIZE,
			LOAD,
			UNLOAD,
			ENTER_FRAME,
			MOUSE_DOWN,
			MOUSE_UP,
			MOUSE_MOVE,
			KEY_DOWN,
			KEY_UP,
			DATA,

			CONSTRUCT,
			SETFOCUS,
			KILLFOCUS,			

			// MovieClipLoader events
			ONLOAD_COMPLETE,
			ONLOAD_ERROR,
			ONLOAD_INIT,
			ONLOAD_PROGRESS,
			ONLOAD_START,

			EVENT_COUNT
		};

		// Vitaly: I add m_unused because of problem with hash<event_id, ...>
		// the reason: sizeof(unsigned char+unsigned char+character*)  == 8 
		// and
		//						 sizeof(unsigned char+unsigned char+Uint16+character*)  == 8
		// I think that compiler do aligment
		unsigned char	m_id;
		unsigned char	m_key_code;
		Uint16	m_unused;
		character* m_target;

		event_id() :
			m_id(INVALID),
			m_key_code(key::INVALID),
			m_unused(0),
			m_target(NULL)
		{
		}

		event_id(id_code id, character* target) :
			m_id((unsigned char) id),
			m_key_code(key::INVALID),
			m_unused(0),
			m_target(target)
		{
		}

		event_id(id_code id, key::code c = key::INVALID) :
			m_id((unsigned char) id),
			m_key_code((unsigned char) c),
			m_unused(0),
			m_target(NULL)
		{
			// For the button key events, you must supply a keycode.
			// Otherwise, don't.
			assert((m_key_code == key::INVALID && (m_id != KEY_PRESS))
				|| (m_key_code != key::INVALID && (m_id == KEY_PRESS)));
		}

		bool	operator==(const event_id& id) const
		{
			return m_id == id.m_id &&
				m_key_code == id.m_key_code &&
				m_target == id.m_target;
		}

		// Return the name of a method-handler function corresponding to this event.
		const tu_string&	get_function_name() const;
	};


	// Base class for actions.
	struct action_buffer
	{
		action_buffer();
		void	read(stream* in);
		void	execute(as_environment* env);
		void	execute(
			as_environment* env,
			int start_pc,
			int exec_bytes,
			as_value* retval,
			const array<with_stack_entry>& initial_with_stack,
			bool is_function2);

		as_object* load_as_plugin(const tu_string& classname, const array<as_value>& params);

		bool	is_null()
		{
			return m_buffer.size() < 1 || m_buffer[0] == 0;
		}

		int	get_length() const { return m_buffer.size(); }

	private:
		// Don't put these as values in array<>!  They contain
		// internal pointers and cannot be moved or copied.
		// If you need to keep an array of them, keep pointers
		// to new'd instances.
		action_buffer(const action_buffer& a) { assert(0); }
//		void operator=(const action_buffer& a) { assert(0); }

		void	process_decl_dict(int start_pc, int stop_pc);
		void	enumerate(as_environment* env, as_object_interface* object);

		// data:
		array<unsigned char>	m_buffer;
		array<const char*>	m_dictionary;
		int	m_decl_dict_processed_at;
	};


	struct fn_call;

	struct as_property_interface
	{
		virtual ~as_property_interface() {}
		virtual bool	set_property(int index, const as_value& val) = 0;
	};

// tulrich: I'm not too sure this is useful.  For things like
// xml_as_object, is it sufficient to always store the event handlers
// as ordinary members using their canonical names, instead of this
// special table?  I have a feeling that's what Macromedia does
// (though I'm not sure).
#if 0
	// This class is just as_object_interface, with an event
	// handler table added.
	struct as_object_with_handlers : public as_object_interface
	{
		// ActionScript event handler table.
		hash<event_id, gameswf::as_value>	 m_event_handlers;

		// ActionScript event handler.
		void	set_event_handler(event_id id, const as_value& method)
		{
			// m_event_handlers.push_back(as);
			//m_event_handlers.set(id, method);
		}

		bool	get_event_handler(event_id id, gameswf::as_value* result)
		{
			//return m_event_handlers.get(id, result);
			return false;
		}
	};
#endif // 0

	//
	// Some handy helpers
	//

	// tulrich: don't use this!  To register a class constructor,
	// just assign the classname to the constructor function.  E.g.:
	//
	// my_movie->set_member("MyClass", as_value(MyClassConstructorFunction));
	// 
	//void register_as_object(const char* object_name, as_c_function_ptr handler);

	// Numerical indices for standard member names.	 Can use this
	// to help speed up get/set member calls, by using a switch()
	// instead of nasty string compares.
	enum as_standard_member
	{
		M_INVALID_MEMBER = -1,
		M_X,
		M_Y,
		M_XSCALE,
		M_YSCALE,
		M_CURRENTFRAME,
		M_TOTALFRAMES,
		M_ALPHA,
		M_VISIBLE,
		M_WIDTH,
		M_HEIGHT,
		M_ROTATION,
		M_TARGET,
		M_FRAMESLOADED,
		M_NAME,
		M_DROPTARGET,
		M_URL,
		M_HIGHQUALITY,
		M_FOCUSRECT,
		M_SOUNDBUFTIME,
		M_XMOUSE,
		M_YMOUSE,
		M_PARENT,
//		M_TEXT,
//		M_TEXTWIDTH,
//		M_TEXTCOLOR,
//		M_ONLOAD,

		AS_STANDARD_MEMBER_COUNT
	};
	// Return the standard enum, if the arg names a standard member.
	// Returns M_INVALID_MEMBER if there's no match.
	as_standard_member	get_standard_member(const tu_stringi& name);

}	// end namespace gameswf


#endif // GAMESWF_ACTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
