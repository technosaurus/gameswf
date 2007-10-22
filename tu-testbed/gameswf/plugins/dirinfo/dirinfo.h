// dirinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, gets dir entity

#ifndef GAMESWF_DIRINFO_PLUGIN_H
#define GAMESWF_DIRINFO_PLUGIN_H

#include "dirent.h"
#include "gameswf/gameswf_object.h"

using namespace gameswf;

struct dirinfo : public as_plugin
{
	exported_module void get_info(as_object* info, const tu_string& path);
};

#endif	// GAMESWF_DIRINFO_PLUGIN_H

