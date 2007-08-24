// gameswf_function.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript function.

#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_as_classes/as_array.h"

namespace gameswf
{
	as_as_function::as_as_function(action_buffer* ab, as_environment* env,
		int start, const array<with_stack_entry>& with_stack)
		:
		m_action_buffer(ab),
		m_env(env),
		m_with_stack(with_stack),
		m_start_pc(start),
		m_length(0),
		m_is_function2(false),
		m_local_register_count(0),
		m_function2_flags(0)
	{
		assert(m_action_buffer);
		m_properties = new as_object();
		m_properties->set_member("prototype", new as_object());
	}

	as_as_function::~as_as_function()
	{
	}

	void	as_as_function::operator()(const fn_call& fn)
	// Dispatch.
	{
		as_environment*	our_env = m_env;
		if (our_env == NULL)
		{
			our_env = fn.env;
		}
		assert(our_env);

		// Set up local stack frame, for parameters and locals.
		int	local_stack_top = our_env->get_local_frame_top();
		our_env->add_frame_barrier();

		if (m_is_function2 == false)
		{
			// Conventional function.

			// Push the arguments onto the local frame.
			int	args_to_pass = imin(fn.nargs, m_args.size());
			for (int i = 0; i < args_to_pass; i++)
			{
				assert(m_args[i].m_register == 0);
				our_env->add_local(m_args[i].m_name, fn.arg(i));
			}

			if (fn.this_ptr)
			{
				our_env->set_local("this", fn.this_ptr);

				// Put 'super' in a local var.
				our_env->add_local("super", fn.this_ptr->get_proto());
			}
		}
		else
		{
			// function2: most args go in registers; any others get pushed.
			
			// Create local registers.
			our_env->add_local_registers(m_local_register_count);

			// Handle the explicit args.
			int	args_to_pass = imin(fn.nargs, m_args.size());
			for (int i = 0; i < args_to_pass; i++)
			{
				if (m_args[i].m_register == 0)
				{
					// Conventional arg passing: create a local var.
					our_env->add_local(m_args[i].m_name, fn.arg(i));
				}
				else
				{
					// Pass argument into a register.
					int	reg = m_args[i].m_register;
					our_env->set_register(reg, fn.arg(i));
				}
			}

			// Handle the implicit args.
			int	current_reg = 1;

			as_object_interface* this_ptr = fn.this_ptr ? fn.this_ptr : our_env->m_target;
			if (m_function2_flags & 0x01)
			{
				// preload 'this' into a register.
				our_env->set_register(current_reg, this_ptr);
				current_reg++;

			}

			if (m_function2_flags & 0x02)
			{
				// Don't put 'this' into a local var.
			}
			else
			{
				// Put 'this' in a local var.
				our_env->add_local("this", as_value(this_ptr));
			}

			// Init arguments array, if it's going to be needed.
			smart_ptr<as_array>	arg_array;
			if ((m_function2_flags & 0x04) || ! (m_function2_flags & 0x08))
			{
				arg_array = new as_array;

				as_value	index_number;
				for (int i = 0; i < fn.nargs; i++)
				{
					index_number.set_int(i);
					arg_array->set_member(index_number.to_string(), fn.arg(i));
				}
			}

			if (m_function2_flags & 0x04)
			{
				// preload 'arguments' into a register.
				our_env->set_register(current_reg, arg_array.get_ptr());
				current_reg++;
			}

			if (m_function2_flags & 0x08)
			{
				// Don't put 'arguments' in a local var.
			}
			else
			{
				// Put 'arguments' in a local var.
 				our_env->add_local("arguments", as_value(arg_array.get_ptr()));
			}

			if (m_function2_flags & 0x10)
			{
				// Put 'super' in a register.
				our_env->set_register(current_reg, fn.this_ptr->get_proto());
				current_reg++;
			}

			if (m_function2_flags & 0x20)
			{
				// Don't put 'super' in a local var.
			}
			else
			{
				// Put 'super' in a local var.
				our_env->add_local("super", fn.this_ptr->get_proto());
			}

			if (m_function2_flags & 0x40)
			{
				// Put '_root' in a register.
				our_env->set_register(current_reg, our_env->m_target->get_root_movie());
				current_reg++;
			}

			if (m_function2_flags & 0x80)
			{
				// Put '_parent' in a register.
				array<with_stack_entry>	dummy;
				as_value	parent = our_env->get_variable("_parent", dummy);
				our_env->set_register(current_reg, parent);
				current_reg++;
			}

			if (m_function2_flags & 0x100)
			{
				// Put '_global' in a register.
				our_env->set_register(current_reg, get_global());
				current_reg++;
			}
		}

		// Execute the actions.
		m_action_buffer->execute(our_env, m_start_pc, m_length, fn.result, m_with_stack, m_is_function2);

		// Clean up stack frame.
		our_env->set_local_frame_top(local_stack_top);

		if (m_is_function2)
		{
			// Clean up the local registers.
			our_env->drop_local_registers(m_local_register_count);
		}
	}
}
