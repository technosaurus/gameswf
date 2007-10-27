// sysinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, gets dir entity

#ifndef GAMESWF_sysinfo_PLUGIN_H
#define GAMESWF_sysinfo_PLUGIN_H

#include <dirent.h>
#include "gameswf/gameswf_object.h"

using namespace gameswf;

struct sysinfo : public as_plugin
{

	exported_module void get_dir(as_object* info, const tu_string& path);
	exported_module void get_hdd_serno(tu_string* sn, const char* dev);
	exported_module int get_freemem();

};

#endif	// GAMESWF_sysinfo_PLUGIN_H

