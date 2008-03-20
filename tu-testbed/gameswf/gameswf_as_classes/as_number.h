// as_number.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_AS_NUMBER_H
#define GAMESWF_AS_NUMBER_H

#include "gameswf/gameswf_action.h"	// for as_object

namespace gameswf
{

	void	as_global_number_ctor(const fn_call& fn);
	void	as_number_to_string(const fn_call& fn);
	void	as_number_valueof(const fn_call& fn);

	/*
	struct as_number : public as_object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_NUMBER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_number(double val);

		virtual	bool get_member(const tu_stringi& name, as_value* val);
		virtual const char*	to_string();
		virtual const char*	to_string(int radix);
		virtual const char*	typeof() { return "number"; }
		virtual double	to_number() { return m_val; }
		virtual bool to_bool() { return m_val != 0; }

		double m_val;
	};
	*/

}	// end namespace gameswf


#endif // GAMESWF_AS_NUMBER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
