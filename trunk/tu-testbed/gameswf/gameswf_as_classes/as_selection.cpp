// as_color.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_selection.h"
#include "gameswf/gameswf_character.h"

namespace gameswf
{

	void	as_selection_setfocus(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_selection* obj = fn.this_ptr->cast_to_as_selection();
		if (obj)
		{
			if (fn.nargs > 0)
			{
				character* target = fn.env->find_target(fn.arg(0));
				if (target)
				{
					target->on_event(event_id::SETFOCUS);
					fn.result->set_bool(true);
				}
			}
		}
		fn.result->set_bool(false);
	}

	as_object* selection_init()
	{
		as_object* sel = new as_selection();

		// methods
		sel->set_member("setFocus", as_selection_setfocus);

// TODO
//		sel->set_member("getFocus", as_selection_setfocus);
//		sel->set_member("addListener", as_selection_setfocus);
//		sel->set_member("removeListener", as_selection_setfocus);
		// ...
		return sel;
	}

};
