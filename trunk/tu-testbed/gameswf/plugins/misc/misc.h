// misc.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, gets dir entity

#ifndef GAMESWF_misc_PLUGIN_H
#define GAMESWF_misc_PLUGIN_H

#include "gameswf/gameswf_object.h"

using namespace gameswf;

namespace misc_plugin
{

	struct misc : public as_object
	{                                      
		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_MISC };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		misc(player* player);

		exported_module void get_dir(as_object* info, const tu_string& path);
		exported_module bool get_hdd_serno(tu_string* sn, const char* dev);
		exported_module int get_freemem();

	};
}
#endif	// GAMESWF_misc_PLUGIN_H

