// gameswf_plugin.h	--Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Based on gameswf_value.*

#ifndef GAMESWF_PLUGIN_H
#define GAMESWF_PLUGIN_H

#include "base/container.h"

#ifdef _WIN32

	#define snprintf _snprintf
	#define isnan _isnan

	#define gameswf_module __declspec(dllexport)

#endif // _WIN32

struct plugin_value;
struct gameswf_plugin;

typedef void (*plugin_function_ptr)(plugin_value* result, 
																		gameswf_plugin* this_ptr, 
																		const array<plugin_value>& params);

struct plugin_value
{
	enum type
	{
		UNDEFINED,
		NULLTYPE,	// unused
		BOOLEAN,
		STRING,
		NUMBER,
		OBJECT,	// unused
		C_FUNCTION,	// unused
		AS_FUNCTION,	// unused
		PROPERTY,	// unused
		PLUGIN_FUNCTION
	};

	type	m_type;

	mutable tu_string	m_string_value;
	union
	{
		bool m_boolean_value;
		// @@ hm, what about PS2, where double is bad?	should maybe have int&float types.
		mutable	double	m_number_value;
		gameswf_plugin* m_object_value;
		plugin_function_ptr	m_plugin_function_value;
	};

	plugin_value() :
		m_type(UNDEFINED),
		m_number_value(0.0)
	{
	}

	plugin_value(bool val) :
		m_type(BOOLEAN),
		m_boolean_value(val)
	{
	}

	plugin_value(int val)	:
		m_type(NUMBER),
		m_number_value(double(val))
	{
	}

	plugin_value(float val) :
		m_type(NUMBER),
		m_number_value(double(val))
	{
	}

	plugin_value(double val) :
		m_type(NUMBER),
		m_number_value(val)
	{
	}

	plugin_value(gameswf_plugin* obj)	:
		m_type(OBJECT),
		m_object_value(obj)
	{
		if (m_object_value)
		{
//			m_object_value->add_ref();
		}
	}

	plugin_value(plugin_function_ptr func)	:
		m_type(PLUGIN_FUNCTION),
		m_plugin_function_value(func)
	{
	}

	void	set_string(const char* str) { m_type = STRING; m_string_value = str; }
	void	set_double(double val) { m_type = NUMBER; m_number_value = val; }
	void	set_bool(bool val) { m_type = BOOLEAN; m_boolean_value = val; }
	void	set_int(int val) { set_double(val); }
	void	set_plugin_function_ptr(plugin_function_ptr func)
	{
		m_type = PLUGIN_FUNCTION; m_plugin_function_value = func;
	}
	void	set_object(gameswf_plugin* obj) { m_type = OBJECT; m_object_value = obj; }

	const tu_string&	to_tu_string() const
	// Conversion to const tu_string&.
	{
		if (m_type == STRING) { /* don't need to do anything */ }
		else if (m_type == NUMBER)
		{
			// @@ Moock says if value is a NAN, then result is "NaN"
			// INF goes to "Infinity"
			// -INF goes to "-Infinity"
			if (isnan(m_number_value)) 
			{
				m_string_value = "NaN";
			} 
			else
			{
				char buffer[50];
				snprintf(buffer, 50, "%.14g", m_number_value);
				m_string_value = buffer;
			}
		}
		else if (m_type == UNDEFINED)
		{
			m_string_value = "undefined";
		}
		else if (m_type == NULLTYPE)
		{ 
			m_string_value = "null";
		}
		else if (m_type == BOOLEAN)
		{
			m_string_value = this->m_boolean_value ? "true" : "false";
		}
		else if (m_type == PLUGIN_FUNCTION)
		{
			char buffer[50];
			snprintf(buffer, 50, "<plugin_function 0x%X>", (void*) (m_plugin_function_value));
			m_string_value = buffer;
		}
		else
		{
			m_string_value = "<bad type>";
			assert(0);
		}
		
		return m_string_value;
	}


	const char*	to_string() const
	// Conversion to string.
	{
		return to_tu_string().c_str();
	}

	gameswf_plugin*	to_object() const
	// Return value as an object.
	{
		if (m_type == OBJECT)
		{
			// OK.
			return m_object_value;
		}
		return NULL;
	}

};


struct gameswf_plugin
// base class for gameswf plugin
{
	stringi_hash<plugin_value>	m_members;

	gameswf_plugin() {};
	virtual ~gameswf_plugin() {};

	virtual bool	get_member(const tu_stringi& varname, plugin_value* val) const
	{
		return m_members.get(varname, val);
	}

	virtual bool	set_member(const tu_stringi& varname, const plugin_value& val)
	{
		m_members.set(varname, val);
		return true;
	}

};


#endif
