// as_boolean.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_AS_BOOLEAN_H
#define GAMESWF_AS_BOOLEAN_H

#include "gameswf/gameswf_action.h"	// for as_object

namespace gameswf
{

	void	as_global_boolean_ctor(const fn_call& fn);

	struct as_boolean : public as_object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_BOOLEAN };
		virtual bool is(int class_id)
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_boolean(double val);

		virtual	bool get_member(const tu_stringi& name, as_value* val);
		virtual const char*	to_string();
		virtual const char*	typeof() { return "boolean"; }
		virtual double	to_number() { return m_val ? 1 : 0; }
		virtual bool to_bool() { return m_val; }

		bool m_val;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_BOOLEAN_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
