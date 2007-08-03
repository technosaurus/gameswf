// gameswf_environment.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript "environment", essentially VM state?

#ifndef GAMESWF_ENVIRONMENT_H
#define GAMESWF_ENVIRONMENT_H

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_value.h"

namespace gameswf
{

	#define GLOBAL_REGISTER_COUNT 4

	struct character;
	struct sprite_instance;
	struct as_object;

	//
	// with_stack_entry
	//
	// The "with" stack is for Pascal-like with-scoping.

	struct with_stack_entry
	{
		smart_ptr<as_object_interface>	m_object;
		int	m_block_end_pc;
		
		with_stack_entry()
			:
			m_object(NULL),
			m_block_end_pc(0)
		{
		}

		with_stack_entry(as_object_interface* obj, int end)
			:
			m_object(obj),
			m_block_end_pc(end)
		{
		}
	};

	struct as_environment
	{
		array<as_value>	m_stack;
		as_value	m_global_register[GLOBAL_REGISTER_COUNT];
		array<as_value>	m_local_register;	// function2 uses this
		character*	m_target;

		// this is used in 'new' (0x40 opcode) to pass
		// new created object pointer to constructor chain
		smart_ptr<as_object>	m_instance;

		stringi_hash<as_value>	m_variables;

		// For local vars.  Use empty names to separate frames.
		struct frame_slot
		{
			tu_string	m_name;
			as_value	m_value;

			frame_slot() {}
			frame_slot(const tu_string& name, const as_value& val) : m_name(name), m_value(val) {}
		};
		array<frame_slot>	m_local_frames;

		as_environment() :
			m_target(NULL),
			m_instance(NULL)
		{
		}

		~as_environment()
		{
		}

		character*	get_target() { return m_target; }
		
		void set_target(character* target) { m_target = target; }
		void set_target(as_value& target, character* original_target);

		// stack access/manipulation
		// @@ TODO do more checking on these
		template<class T>
		void	push(T val) { push_val(as_value(val)); }
		void	push_val(const as_value& val) { m_stack.push_back(val); }
		as_value	pop() { as_value result = m_stack.back(); m_stack.pop_back(); return result; }
		as_value&	top(int dist) { return m_stack[m_stack.size() - 1 - dist]; }
		as_value&	bottom(int index) { return m_stack[index]; }
		void	drop(int count) { m_stack.resize(m_stack.size() - count); }

		int	get_top_index() const { return m_stack.size() - 1; }

		as_value	get_variable(const tu_string& varname, const array<with_stack_entry>& with_stack) const;
		// no path stuff:
		as_value	get_variable_raw(const tu_string& varname, const array<with_stack_entry>& with_stack) const;

		void	set_variable(const tu_string& path, const as_value& val, const array<with_stack_entry>& with_stack);
		// no path stuff:
		void	set_variable_raw(const tu_string& path, const as_value& val, const array<with_stack_entry>& with_stack);

		void	set_local(const tu_string& varname, const as_value& val);
		void	add_local(const tu_string& varname, const as_value& val);	// when you know it doesn't exist.
		void	declare_local(const tu_string& varname);	// Declare varname; undefined unless it already exists.

		bool	get_member(const tu_stringi& varname, as_value* val) const;
		bool	set_member(const tu_stringi& varname, const as_value& val);

		// Parameter/local stack frame management.
		int	get_local_frame_top() const { return m_local_frames.size(); }
		void	set_local_frame_top(int t) { assert(t <= m_local_frames.size()); m_local_frames.resize(t); }
		void	add_frame_barrier() { m_local_frames.push_back(frame_slot()); }

		// Local registers.
		void	add_local_registers(int register_count)
		{
			m_local_register.resize(m_local_register.size() + register_count);
		}
		void	drop_local_registers(int register_count)
		{
			m_local_register.resize(m_local_register.size() - register_count);
		}

		as_value* get_register(int reg);
		void set_register(int reg, const as_value& val);

		// may be used in outside of class instance
		static bool	parse_path(const tu_string& var_path, tu_string* path, tu_string* var);

		// Internal.
		int	find_local(const tu_string& varname, bool ignore_barrier) const;
		character*	find_target(const tu_string& path) const;
		character*	find_target(const as_value& val) const;

		character* load_file(const char* url, const as_value& target);

		int get_refs(as_object_interface* this_ptr);
		void clear_refs(as_object_interface* this_ptr);

		void dump();

		private:

		as_value*	local_register_ptr(int reg);

	};


	// Parameters/environment for C functions callable from ActionScript.
	struct fn_call
	{
		as_value* result;
		as_object_interface* this_ptr;
		as_environment* env;
		int nargs;
		int first_arg_bottom_index;

		fn_call(as_value* res_in, as_object_interface* this_in, as_environment* env_in, int nargs_in, int first_in)
			:
			result(res_in),
			this_ptr(this_in),
			env(env_in),
			nargs(nargs_in),
			first_arg_bottom_index(first_in)
		{
		}

		as_value& arg(int n) const
		// Access a particular argument.
		{
			assert(n < nargs);
			return env->bottom(first_arg_bottom_index - n);
		}
	};

}

#endif
