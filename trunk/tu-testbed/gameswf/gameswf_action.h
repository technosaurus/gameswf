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


	// ActionScript value type.
	struct as_value
	{
		enum type
		{
			UNDEFINED,
			STRING,
			NUMBER,
//			OBJECT_PTR	// @@ ?
		};

		type	m_type;
		double	m_number_value;
		mutable tu_string	m_string_value;

		as_value()
			:
			m_type(UNDEFINED),
			m_number_value(0.0)
		{
		}

		as_value(const char* str)
			:
			m_type(STRING),
			m_number_value(0.0),
			m_string_value(str)
		{
		}

		as_value(type e)
			:
			m_type(e),
			m_number_value(0.0)
		{
		}

		as_value(bool val)
			:
			m_type(NUMBER),
			m_number_value(double(val))
		{
		}

		as_value(int val)
			:
			m_type(NUMBER),
			m_number_value(val)
		{
		}

		as_value(float val)
			:
			m_type(NUMBER),
			m_number_value(val)
		{
		}

		as_value(double val)
			:
			m_type(NUMBER),
			m_number_value(val)
		{
		}

		bool	is_string() const { return m_type == STRING; }
		const char*	to_string() const;
		const tu_string&	to_tu_string() const;
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


	// ActionScript "environment", essentially VM state?
	struct as_environment
	{
		array<as_value>	m_stack;
		movie*	m_target;
		string_hash<as_value>	m_local_variables;

		as_environment()
			:
			m_target(0)
		{
		}

		movie*	get_target() { return m_target; }
		void	set_target(movie* target) { m_target = target; }

		template<class T>
		void	push(T val) { m_stack.push_back(as_value(val)); }

		as_value	pop() { return m_stack.pop_back(); }

		as_value	get_variable(const tu_string& varname) const;

		void	set_variable(const char* path, const as_value& val);
		void	set_local(const tu_string& varname, const as_value& val);
	};


};	// end namespace gameswf


#endif // GAMESWF_ACTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
