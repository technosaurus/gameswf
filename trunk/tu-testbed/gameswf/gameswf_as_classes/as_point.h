// as_point.h	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_AS_POINT_H
#define GAMESWF_AS_POINT_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_character.h"

namespace gameswf
{

	void	as_global_point_ctor(const fn_call& fn);

	struct as_point : public as_object
	{
		as_point(float x, float y);
		virtual as_point* cast_to_as_point() { return this; }

		exported_module virtual bool	set_member(const tu_stringi& name, const as_value& val);
		exported_module virtual bool	get_member(const tu_stringi& name, as_value* val);
		//exported_module virtual bool	get_member(const tu_stringi& name, as_member* member) const;

		gameswf::point m_point;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_POINT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
