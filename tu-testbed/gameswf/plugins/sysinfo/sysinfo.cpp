// sysinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, some useful misc programs

#include <stdio.h>
#include "sysinfo.h"

#ifndef WIN32
	#include <sys/stat.h>
	#include "unistd.h"
	#include "sys/ioctl.h"
	#include "fcntl.h"
	#include "linux/hdreg.h"
#endif

// methods that is called from Action Scirpt

// gets dir entity. It is useful for a preloading of the SWF files
void	getDir(const fn_call& fn)
{
	assert(fn.this_ptr);
	sysinfo* di = (sysinfo*) fn.this_ptr; //hack
	assert(di);
	if (fn.nargs > 0)
	{
		as_object* info = new as_object();
		di->get_dir(info, fn.arg(0).to_string());
		*fn.result = info;
	}
}
// gets HDD serial NO. It is useful for a binding the program to HDD
void	getHDDSerNo(const fn_call& fn)
{
	assert(fn.this_ptr);
	sysinfo* di = (sysinfo*) fn.this_ptr; //hack
	assert(di);
	if (fn.nargs > 0)
	{
		tu_string sn;
		di->get_hdd_serno(&sn, fn.arg(0).to_string());
		fn.result->set_tu_string(sn);
	}
}

// gets OS name, like "Linux", "Windows"
void	getOS(const fn_call& fn)
{
#ifdef WIN32
	fn.result->set_tu_string("WINDOWS");
#else
	fn.result->set_tu_string("LINUX");
#endif
}

// gets available free memory
void	getFreeMem(const fn_call& fn)
{
	assert(fn.this_ptr);
	sysinfo* di = (sysinfo*) fn.this_ptr; //hack
	assert(di);
	fn.result->set_int(di->get_freemem());
}

// DLL interface

extern "C"
{
	exported_module as_plugin* gameswf_module_init(const array<as_value>& params)
	{
		sysinfo* si = new sysinfo();
		si->set_member("getDir", getDir);
		si->set_member("getOS", getOS);
		si->set_member("getHDDSerNo", getHDDSerNo);
		si->set_member("getFreeMem", getFreeMem);
		return si;
	}
}

void sysinfo::get_dir(as_object* info, const tu_string& path)
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
		if (*name.c_str() == '.' || name == "..")
		{
			continue;
		}

#ifdef WIN32
		bool is_readonly = de->data.dwFileAttributes & 0x01;
		bool is_hidden = de->data.dwFileAttributes & 0x02;
//??		bool b2 = de->data.dwFileAttributes & 0x04;
//??		bool b3 = de->data.dwFileAttributes & 0x08;
		bool is_dir = de->data.dwFileAttributes & 0x10;
#else
		struct stat buf;
		stat(tu_string(path + de->d_name).c_str(), &buf);
		bool is_readonly = false;	//TODO
		bool is_hidden = false; //TODO
    bool is_dir = S_ISDIR(buf.st_mode);
#endif

		// printf("%s, %d\n", name.c_str(), is_dir);

		as_object* item = new as_object();
		info->set_member(name, item);

		item->set_member("is_dir", is_dir);
		item->set_member("is_readonly", is_readonly);
		item->set_member("is_hidden", is_hidden);
		if (is_dir)
		{
			as_object* info = new as_object();
			item->set_member("dirinfo", info);
			get_dir(info, path + de->d_name + "/");
		}

	}
	closedir(dir);
}

void sysinfo::get_hdd_serno(tu_string* sn, const char* dev)
{
	assert(sn);

	if (dev == NULL)
	{
		return;
	}

#ifdef WIN32

	//TODO

#else

	struct hd_driveid id;
	int fd = open(dev, O_RDONLY | O_NONBLOCK);
	if (fd < 0)
	{
//		printf("can't get info about '%s'\n", dev);
		return false;
	}

	int rc = ioctl(fd, HDIO_GET_IDENTITY, &id);
	close(fd);

	if (rc != 0)
	{
//		printf("can't get info about '%s'\n", dev);
		return;
	}

	//  printf("serial no %s\n", id.serial_no);
	//  printf("model number %s\n", id.model);
	//  printf("firmware revision %s\n", id.fw_rev);
	//  printf("cylinders %s\n", id.cyls);
	//  printf("heads %s\n", id.heads);
	//  printf("sectors per track %s\n", id.sectors);

	sn = (char*) id.serial_no;

#endif

}

int sysinfo::get_freemem()
{

#ifdef WIN32

	return 0;		//TODO

#else

	ifstream meminfo("/proc/meminfo");
	if (meminfo.is_open() == false)
	{
		return -1;
	}

	char szTmp[256];
	char szMem[256];

	string s0("MemFree:");
	string s1("kB");

	while (meminfo.eof() == false)
	{
		meminfo.getline(szTmp, 256);
		string s2(szTmp);
		string::size_type pos0 = s2.find(s0);

		if (pos0 != string::npos)
		{
			string::size_type pos1 = s2.find(s1);
			if (pos1 != string::npos)
			{
				string s3 = s2.substr(pos0 + s0.size(), pos1 - (pos0 + s0.size()));
				strncpy(szMem, s3.c_str(), s3.size());
				return (int(atoi(szMem) / 1024.0f)) ;	// MB
			}
		}
	}
	return -1;
#endif
}
