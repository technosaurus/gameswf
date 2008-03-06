// gameswf_function.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript function.

#ifndef GAMESWF_FUNCTION_H
#define GAMESWF_FUNCTION_H

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_environment.h"
#include "gameswf/gameswf_object.h"
#include "gameswf/gameswf_action.h"

#include "base/container.h"
#include "base/smart_ptr.h"

namespace gameswf
{
	struct as_environment;
	struct as_object;

	struct as_as_function : public as_object_interface
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_AS_FUNCTION };
		virtual bool is(int class_id)
		{
			return m_class_id == class_id;
		}

		action_buffer	m_action_buffer;

		array<with_stack_entry>	m_with_stack;	// initial with-stack on function entry.
		int	m_start_pc;
		int	m_length;
		struct arg_spec
		{
			int	m_register;
			tu_string	m_name;
		};
		array<arg_spec>	m_args;
		bool	m_is_function2;
		uint8	m_local_register_count;
		uint16	m_function2_flags;	// used by function2 to control implicit arg register assignments

		// ActionScript functions have a property namespace!
		// Typically used for class constructors, for "prototype", "constructor",
		// and class properties.
		smart_ptr<as_object>	m_properties;

		// if function has been declared in moviclip then we should use its environment
		// And for this purpose it is necessary to keep target that has created 'this'
		// testcase:
		// _root.myclip.onEnterFrame = _root.myfunc;
		// myfunc should use _root environment
		weak_ptr<as_object_interface>	m_target;

		as_as_function(action_buffer* ab, int start, const array<with_stack_entry>& with_stack);
		~as_as_function();

		void	set_is_function2() { m_is_function2 = true; }
		void	set_local_register_count(uint8 ct) { assert(m_is_function2); m_local_register_count = ct; }
		void	set_function2_flags(uint16 flags) { assert(m_is_function2); m_function2_flags = flags; }

		void	add_arg(int arg_register, const char* name)
		{
			assert(arg_register == 0 || m_is_function2 == true);
			m_args.resize(m_args.size() + 1);
			m_args.back().m_register = arg_register;
			m_args.back().m_name = name;
		}

		void	set_length(int len) { assert(len >= 0); m_length = len; }
		void set_target(as_object_interface* target)
		{
			m_target = target;
		}

		// Dispatch.
		void	operator()(const fn_call& fn);

	};

}

#endif
