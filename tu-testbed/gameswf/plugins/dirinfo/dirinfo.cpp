// dirinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, gets dir entity

#include <stdio.h>
#include "dirinfo.h"

// methods that is called from Action Scirpt

void	getInfo(const fn_call& fn)
{
	assert(fn.this_ptr);
	dirinfo* di = (dirinfo*) fn.this_ptr; //hack
	assert(di);
	if (fn.nargs > 0)
	{
		as_object* info = new as_object();
		di->get_info(info, fn.arg(0).to_string());
		*fn.result = info;
	}
}

// DLL interface

extern "C"
{
	exported_module as_plugin* gameswf_module_init(const array<as_value>& params)
	{
		printf("gameswf_init\n");
		dirinfo* di = new dirinfo();
		di->set_member("getInfo", getInfo);
		return di;
	}
}

void dirinfo::get_info(as_object* info, const tu_string& path)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		return;
	}

//	as_object* info = new as_object();
	while (dirent* de = readdir(dir))
	{
		tu_string name = de->d_name;
		if (name == "." || name == "..")
		{
			continue;
		}

		bool is_readonly = de->data.dwFileAttributes & 0x01;
		bool is_hidden = de->data.dwFileAttributes & 0x02;
//??		bool b2 = de->data.dwFileAttributes & 0x04;
//??		bool b3 = de->data.dwFileAttributes & 0x08;
		bool is_dir = de->data.dwFileAttributes & 0x10;

		as_object* item = new as_object();
		info->set_member(name, item);

		item->set_member("is_dir", is_dir);
		item->set_member("is_readonly", is_readonly);
		item->set_member("is_hidden", is_hidden);
		if (is_dir)
		{
			as_object* info = new as_object();
			item->set_member("dirinfo", info);
			get_info(info, path + de->d_name + "/");
		}

	}
	closedir(dir);
}

