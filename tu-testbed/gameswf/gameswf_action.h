// gameswf_action.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#ifndef GAMESWF_ACTION_H
#define GAMESWF_ACTION_H


#include "gameswf_types.h"

#include "base/container.h"


namespace gameswf
{
	struct movie;
	struct as_environment;
	struct as_object_interface;
	struct as_value;


	//
	// with_stack_entry
	//
	// The "with" stack is for Pascal-like with-scoping.

	struct with_stack_entry
	{
		as_object_interface*	m_object;
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


	// Base class for actions.
	struct action_buffer
	{
		array<unsigned char>	m_buffer;
		array<const char*>	m_dictionary;

		void	read(stream* in);
		void	execute(as_environment* env);
		void	execute(
			as_environment* env,
			int start_pc,
			int exec_bytes,
			as_value* retval,
			const array<with_stack_entry>& initial_with_stack);

		bool	is_null()
		{
			return m_buffer.size() < 1 || m_buffer[0] == 0;
		}
	};


	typedef void (*as_c_function_ptr)(
		as_value* result,
		void* this_ptr,
		as_environment* env,
		int nargs,
		int first_arg_bottom_index);

	struct as_as_function;	// forward decl

	// ActionScript value type.
	struct as_value
	{
		enum type
		{
			UNDEFINED,
			STRING,
			NUMBER,
			OBJECT,
			C_FUNCTION,
			AS_FUNCTION,	// ActionScript function.
		};

		type	m_type;
		mutable tu_string	m_string_value;
		union
		{
			mutable	double	m_number_value;	// @@ hm, what about PS2, where double is bad?  should maybe have int&float types.
			as_object_interface*	m_object_value;
			as_c_function_ptr	m_c_function_value;
			as_as_function*	m_as_function_value;
		};

		as_value()
			:
			m_type(UNDEFINED),
			m_number_value(0.0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
		}

		as_value(const char* str)
			:
			m_type(STRING),
			m_number_value(0.0),
			m_string_value(str),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
		}

		as_value(type e)
			:
			m_type(e),
			m_number_value(0.0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
		}

		as_value(bool val)
			:
			m_type(NUMBER),
			m_number_value(0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_number_value = double(val);
		}

		as_value(int val)
			:
			m_type(NUMBER),
			m_number_value(0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_number_value = double(val);
		}

		as_value(float val)
			:
			m_type(NUMBER),
			m_number_value(0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_number_value = double(val);
		}

		as_value(double val)
			:
			m_type(NUMBER),
			m_number_value(0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_number_value = val;
		}

		as_value(as_object_interface* obj)
			:
			m_type(OBJECT),
			m_number_value(0.0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_object_value = obj;
		}

		as_value(as_c_function_ptr func)
			:
			m_type(C_FUNCTION),
			m_number_value(0.0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_c_function_value = func;
		}

		as_value(as_as_function* func)
			:
			m_type(AS_FUNCTION),
			m_number_value(0.0),
			m_object_value(0),
			m_c_function_value(0),
			m_as_function_value(0)
		{
			m_as_function_value = func;
		}

		type	get_type() const { return m_type; }

		const char*	to_string() const;
		const tu_string&	to_tu_string() const;
		double	to_number() const;
		bool	to_bool() const;
		as_object_interface*	to_object() const;
		as_c_function_ptr	to_c_function() const;
		as_as_function*	to_as_function() const;

		void	convert_to_number();
		void	convert_to_string();

		void	set(const tu_string& str) { m_type = STRING; m_string_value = str; }
		void	set(const char* str) { m_type = STRING; m_string_value = str; }
		void	set(double val) { m_type = NUMBER; m_number_value = val; }
		void	set(bool val) { m_type = NUMBER; m_number_value = val ? 1.0 : 0.0; }
		void	set(int val) { set(double(val)); }
		void	set(as_object_interface* obj) { m_type = OBJECT; m_object_value = obj; }
		void	set_undefined() { m_type = UNDEFINED; }

		bool	operator==(const as_value& v) const;
		bool	operator<(const as_value& v) const { return to_number() < v.to_number(); }
		void	operator+=(const as_value& v) { set(this->to_number() + v.to_number()); }
		void	operator-=(const as_value& v) { set(this->to_number() - v.to_number()); }
		void	operator*=(const as_value& v) { set(this->to_number() * v.to_number()); }
		void	operator/=(const as_value& v) { set(this->to_number() / v.to_number()); }	// @@ check for div/0
		void	operator&=(const as_value& v) { set(int(this->to_number()) & int(v.to_number())); }
		void	operator|=(const as_value& v) { set(int(this->to_number()) | int(v.to_number())); }
		void	operator^=(const as_value& v) { set(int(this->to_number()) ^ int(v.to_number())); }
		void	shl(const as_value& v) { set(int(this->to_number()) << int(v.to_number())); }
		void	asr(const as_value& v) { set(int(this->to_number()) >> int(v.to_number())); }
		void	lsr(const as_value& v) { set(double(Uint32(this->to_number()) >> int(v.to_number()))); }

		void	string_concat(const tu_string& str);
	};


	struct as_property_interface
	{
		virtual bool	set_property(int index, const as_value& val) = 0;
	};


	struct as_object_interface
	{
		virtual ~as_object_interface() {}

		virtual bool	set_self_value(const as_value& val) = 0;
		virtual bool	get_self_value(as_value* val) = 0;

		virtual void	set_member(const tu_string& name, const as_value& val) = 0;
		virtual bool	get_member(const tu_string& name, as_value* val) = 0;
//		virtual as_value	call_method(
//			const tu_string& name,
//			as_environment* env,
//			int nargs,
//			int first_arg_bottom_index) = 0;

		virtual movie*	to_movie() = 0;
	};


	// ActionScript "environment", essentially VM state?
	struct as_environment
	{
		array<as_value>	m_stack;
		movie*	m_target;
		string_hash<as_value>	m_variables;

		// For local vars.  Use empty names to separate frames.
		struct frame_slot
		{
			tu_string	m_name;
			as_value	m_value;

			frame_slot() {}
			frame_slot(const tu_string& name, const as_value& val) : m_name(name), m_value(val) {}
		};
		array<frame_slot>	m_local_frames;


		as_environment()
			:
			m_target(0)
		{
		}

		movie*	get_target() { return m_target; }
		void	set_target(movie* target) { m_target = target; }

		// stack access/manipulation
		// @@ TODO do more checking on these
		template<class T>
		void	push(T val) { push_val(as_value(val)); }
		void	push_val(const as_value& val) { m_stack.push_back(val); }
		as_value	pop() { return m_stack.pop_back(); }
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

		bool	get_member(const tu_string& varname, as_value* val) const;
		void	set_member(const tu_string& varname, const as_value& val);

		// Parameter/local stack frame management.
		int	get_local_frame_top() const { return m_local_frames.size(); }
		void	set_local_frame_top(int t) { assert(t <= m_local_frames.size()); m_local_frames.resize(t); }
		void	add_frame_barrier() { m_local_frames.push_back(frame_slot()); }

		// Internal.
		int	find_local(const tu_string& varname) const;
		bool	parse_path(const tu_string& var_path, tu_string* path, tu_string* var) const;
		movie*	find_target(const tu_string& path) const;
		movie*	find_target(const as_value& val) const;
	};


	//
	// as_object
	//
	// A generic bag of attributes.  Base-class for ActionScript
	// script-defined objects.
	struct as_object : public as_object_interface
	{
		string_hash<as_value>	m_members;

		virtual bool	set_self_value(const as_value& val) { return false; }
		virtual bool	get_self_value(as_value* val) { val->set(static_cast<as_object_interface*>(this)); return true; }

		virtual void	set_member(const tu_string& name, const as_value& val)
		{
			m_members.set(name, val);
		}

		virtual bool	get_member(const tu_string& name, as_value* val)
		{
			return m_members.get(name, val);
		}

// 		virtual as_value	call_method(
// 			const tu_string& name,
// 			as_environment* env,
// 			int nargs,
// 			int first_arg_bottom_index);

		virtual movie*	to_movie()
		// This object is not a movie; no conversion.
		{
			return NULL;
		}
	};


	//
	// Some handy helpers
	//

	// Dispatching methods from C++.
	as_value	call_method0(const as_value& method, as_environment* env, as_object_interface* this_ptr);
	as_value	call_method1(
		const as_value& method, as_environment* env, as_object_interface* this_ptr,
		const as_value& arg0);
	as_value	call_method2(
		const as_value& method, as_environment* env, as_object_interface* this_ptr,
		const as_value& arg0, const as_value& arg1);
	as_value	call_method3(
		const as_value& method, as_environment* env, as_object_interface* this_ptr,
		const as_value& arg0, const as_value& arg1, const as_value& arg2);


}	// end namespace gameswf


#endif // GAMESWF_ACTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
