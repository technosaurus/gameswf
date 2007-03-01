// array.h	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_ARRAY_H
#define GAMESWF_AS_ARRAY_H

#include "../gameswf_action.h"	// for as_object

namespace gameswf
{

	void	as_global_array_ctor(const fn_call& fn);

	struct as_array : public as_object
	{
		as_array();

		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);

		tu_string to_string();

	};

}	// end namespace gameswf


#endif // GAMESWF_AS_ARRAY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
