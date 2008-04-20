// as_global.h	-- Thatcher Ulrich <tu@tulrich.com> 2003
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script global functions implementation

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_as_classes/as_timer.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_root.h"

namespace gameswf
{

	//
	// Built-in objects
	//

	void	as_global_trace(const fn_call& fn)
	{
		assert(fn.nargs >= 1);

		// Log our argument.
		//
		// @@ what if we get extra args?

		const char* val = "";
		if (fn.arg(0).is_undefined())
		{
			val = "undefined";
		} 
		else
		{
			val = fn.arg(0).to_string();
		}
		log_msg("%s\n", val);
	}

	void  as_global_setinterval(const fn_call& fn)
	{
		as_timer* timer = NULL;
		if (fn.nargs >= 3)
		{
			as_value func;
			if (fn.arg(0).is_object())
			{
				fn.arg(0).to_object()->get_member(fn.arg(1).to_tu_string(), &func);
			}
			else
			{
				func = fn.arg(0);
			}

			if (func.is_function())
			{
				timer = new as_timer(func, fn.arg(2).to_number(), fn);
			}
		}
		fn.result->set_as_object(timer);
	}

	void  as_global_clearinterval(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			as_timer* timer = cast_to<as_timer>(fn.arg(0).to_object());
			if (timer)
			{
				timer->clear();
			}
		}
	}

	void	as_global_assetpropflags(const fn_call& fn)
	// Undocumented ASSetPropFlags function
	// Works only for as_object for now
	{
		int version = fn.get_player()->get_root()->get_movie_version();

		// Check the arguments
		assert(fn.nargs == 3 || fn.nargs == 4);
		assert((version == 5) ? (fn.nargs == 3) : true);

		// object
		as_object* obj = fn.arg(0).to_object();
		if (obj == NULL)
		{
			log_error("error: assetpropflags for NULL object\n");
			return;
		}

		// The second argument is a list of child names,
		// may be in the form array(like ["abc", "def", "ggggg"]) or in the form a string(like "abc, def, ggggg")
		// the NULL second parameter means that assetpropflags is applied to all children
		as_object* props = fn.arg(1).to_object();

		// a number which represents three bitwise flags which
		// are used to determine whether the list of child names should be hidden,
		// un-hidden, protected from over-write, un-protected from over-write,
		// protected from deletion and un-protected from deletion
		int set_true = fn.arg(2).to_int() & as_prop_flags::as_prop_flags_mask;

		// Is another integer bitmask that works like set_true,
		// except it sets the attributes to false. The
		// set_false bitmask is applied before set_true is applied

		// ASSetPropFlags was exposed in Flash 5, however the fourth argument 'set_false'
		// was not required as it always defaulted to the value '~0'. 
		int set_false = (fn.nargs == 3 ? 
				 (version == 5 ? ~0 : 0) : fn.arg(3).to_int())
			& as_prop_flags::as_prop_flags_mask;

		// Evan: it seems that if set_true == 0 and set_false == 0, this function
		// acts as if the parameters where (object, null, 0x1, 0) ...
		if (set_false == 0 && set_true == 0)
		{
			props = NULL;
			set_false = 0;
			set_true = 0x1;
		}

		if (props == NULL)
		{
			// Takes all members of the object and sets its property flags
			for (stringi_hash<as_member>::const_iterator it = obj->m_members.begin(); 
				it != obj->m_members.end(); ++it)
			{
				as_member member = it.get_value();

				as_prop_flags f = member.get_member_flags();
				f.set_flags(set_true, set_false);
				member.set_member_flags(f);

				obj->m_members.set(it.get_key(), member);
			}
		}
		else
		{
			// Takes all string type prop and sets property flags of obj[prop]
			for (stringi_hash<as_member>::const_iterator it = props->m_members.begin(); 
				it != props->m_members.end(); ++it)
			{
				const as_value& key = it->second.get_member_value();
				if (key.is_string())
				{
					stringi_hash<as_member>::iterator obj_it = obj->m_members.find(key.to_tu_string());
					if (obj_it != obj->m_members.end())
					{
						as_member& member = obj_it->second;

						as_prop_flags f = member.get_member_flags();
						f.set_flags(set_true, set_false);
						member.set_member_flags(f);
					}
				}
			}
		}
	}

	// getVersion() : String
	void	as_global_get_version(const fn_call& fn)
	// Returns a string containing Flash Player version and platform information.
	{
		fn.result->set_tu_string(get_gameswf_version());
	}


};
