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


	// Base class for actions.
	struct action_buffer
	{
		array<unsigned char>	m_buffer;

		void	read(stream* in);
		void	execute(as_environment* env);

		bool	is_null()
		{
			return m_buffer.size() < 1 || m_buffer[0] == 0;
		}
	};


	typedef void (*as_function_ptr)(
		as_value* result,
		void* this_ptr,
		as_environment* env,
		int nargs,
		int first_arg_bottom_index);


	// ActionScript value type.
	struct as_value
	{
		enum type
		{
			UNDEFINED,
			STRING,
			NUMBER,
			OBJECT,
			FUNCTION,
		};

		type	m_type;
		mutable	double	m_number_value;	// @@ hm, what about PS2, where double is bad?  should maybe have int&float types.
		mutable tu_string	m_string_value;
		as_object_interface*	m_object_value;
		as_function_ptr	m_function_value;

		as_value()
			:
			m_type(UNDEFINED),
			m_number_value(0.0),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(const char* str)
			:
			m_type(STRING),
			m_number_value(0.0),
			m_string_value(str),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(type e)
			:
			m_type(e),
			m_number_value(0.0),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(bool val)
			:
			m_type(NUMBER),
			m_number_value(double(val)),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(int val)
			:
			m_type(NUMBER),
			m_number_value(val),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(float val)
			:
			m_type(NUMBER),
			m_number_value(val),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(double val)
			:
			m_type(NUMBER),
			m_number_value(val),
			m_object_value(0),
			m_function_value(0)
		{
		}

		as_value(as_object_interface* obj)
			:
			m_type(OBJECT),
			m_number_value(0.0),
			m_object_value(obj),
			m_function_value(0)
		{
		}

		as_value(as_function_ptr func)
			:
			m_type(FUNCTION),
			m_number_value(0.0),
			m_object_value(0),
			m_function_value(func)
		{
		}

		type	get_type() const { return m_type; }

		const char*	to_string() const;
		const tu_string&	to_tu_string() const;
		double	to_number() const;
		bool	to_bool() const;
		as_object_interface*	to_object() const;
		as_function_ptr	to_function() const;

		void	convert_to_number();
		void	convert_to_string();

		void	set(const tu_string& str) { m_type = STRING; m_string_value = str; }
		void	set(const char* str) { m_type = STRING; m_string_value = str; }
		void	set(double val) { m_type = NUMBER; m_number_value = val; }
		void	set(bool val) { m_type = NUMBER; m_number_value = val ? 1.0 : 0.0; }
		void	set(int val) { set(double(val)); }

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


	// For objects that can get/set ActionScript values.
	// E.g. text fields, ...
	struct as_variable_interface
	{
		virtual bool	set_value(const as_value& val) = 0;
	};


	struct as_property_interface
	{
		virtual bool	set_property(int index, const as_value& val) = 0;
	};


	struct as_object_interface
	{
		virtual ~as_object_interface() {}
		virtual void	set_member(const tu_string& name, const as_value& val) = 0;
		virtual as_value	get_member(const tu_string& name) = 0;
		virtual as_value	call_method(
			const tu_string& name,
			as_environment* env,
			int nargs,
			int first_arg_bottom_index) = 0;
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
		void	push(T val) { m_stack.push_back(as_value(val)); }
		as_value	pop() { return m_stack.pop_back(); }
		as_value&	top(int dist) { return m_stack[m_stack.size() - 1 - dist]; }
		as_value&	bottom(int index) { return m_stack[index]; }
		void	drop(int count) { m_stack.resize(m_stack.size() - count); }

		int	get_top_index() const { return m_stack.size() - 1; }

		as_value	get_variable(const tu_string& varname) const;
		as_value	get_variable_raw(const tu_string& varname) const;	// no path stuff

		void	set_variable(const tu_string& path, const as_value& val);
		void	set_local(const tu_string& varname, const as_value& val);

		// Internal.
		int	find_local(const tu_string& varname) const;
		bool	parse_path(const tu_string& var_path, tu_string* path, tu_string* var) const;
		movie*	find_target(const tu_string& path) const;
	};


};	// end namespace gameswf


#endif // GAMESWF_ACTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
