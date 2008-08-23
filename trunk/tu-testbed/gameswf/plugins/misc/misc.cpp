// misc.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// gameSWF plugin, some useful misc programs

#include <stdio.h>
#include <dirent.h>

#include "base/tu_file.h"
#include "misc.h"

#ifndef WIN32
	#include <sys/stat.h>
	#include "unistd.h"
	#include "sys/ioctl.h"
	#include "fcntl.h"
	#include "linux/hdreg.h"
#endif

namespace misc_plugin
{

	// methods that is called from Action Scirpt

	// gets dir entity. It is useful for a preloading of the SWF files
	void	getDir(const fn_call& fn)
	{
		misc* si = cast_to<misc>(fn.this_ptr);
		if (si)
		{
			if (fn.nargs > 0)
			{
				as_object* dir = new as_object(fn.get_player());
				si->get_dir(dir, fn.arg(0).to_string());
				fn.result->set_as_object(dir);
			}
		}

	}
	// gets HDD serial NO. It is useful for a binding the program to HDD
	void	getHDDSerNo(const fn_call& fn)
	{
		misc* si = cast_to<misc>(fn.this_ptr);
		if (si)
		{
			if (fn.nargs > 0)
			{
				tu_string sn;
				si->get_hdd_serno(&sn, fn.arg(0).to_string());
				fn.result->set_tu_string(sn);
			}
		}
	}

	// gets available free memory
	void	getFreeMem(const fn_call& fn)
	{
		misc* si = cast_to<misc>(fn.this_ptr);
		if (si)
		{
			fn.result->set_int(si->get_freemem());
		}
	}

	void	setDateTime(const fn_call& fn)
	{
#ifdef WIN32
		// TODO
		return;
#endif
		assert(fn.this_ptr);
		assert(fn.env);
		assert(fn.nargs == 5);

//		tm dt;
//		dt.tm_sec = 0;

//		dt.tm_min = (int) fn.arg(4).to_number();	// 0..59
//		dt.tm_hour = (int) fn.arg(3).to_number();				// 0..23
//		dt.tm_mday = (int) fn.arg(2).to_number();				// 1..31
//		dt.tm_mon = (int) fn.arg(1).to_number() - 1;			// 0..11
//		dt.tm_year = (int) fn.arg(0).to_number() - 1900;     // 1900+...
//		time_t t;

//		t = mktime(&dt);
//		int rc = stime(&t); 
//		if (rc == -1)
//		{
//		perror("can't set date-time: \n");
//		}

		char s[80];
		snprintf(s, 80, "date -s '%s/%s/%s %s:%s' | hwclock --systohc",
			fn.arg(0).to_string(),
			fn.arg(1).to_string(),
			fn.arg(2).to_string(),
			fn.arg(3).to_string(),
			fn.arg(4).to_string());

		system(s);
	}

	// DLL interface

	extern "C"
	{
		exported_module as_object* gameswf_module_init(player* player, const array<as_value>& params)
		{
			return new misc(player);
		}
	}

	misc::misc(player* player) :
		as_object(player)
	{
		builtin_member("getDir", getDir);
		builtin_member("getHDDSerNo", getHDDSerNo);
		builtin_member("getFreeMem", getFreeMem);
		builtin_member("setDateTime", setDateTime);
	}

	void misc::get_dir(as_object* info, const tu_string& path)
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
			bool is_readonly = de->data.dwFileAttributes & 0x01 ? true : false;
			bool is_hidden = de->data.dwFileAttributes & 0x02 ? true : false;
	//??		bool b2 = de->data.dwFileAttributes & 0x04;
	//??		bool b3 = de->data.dwFileAttributes & 0x08;
			bool is_dir = de->data.dwFileAttributes & 0x10 ? true : false;
	#else
			struct stat buf;
			stat(tu_string(path + de->d_name).c_str(), &buf);
			bool is_readonly = false;	//TODO
			bool is_hidden = false; //TODO
			bool is_dir = S_ISDIR(buf.st_mode);
	#endif

			// printf("%s, %d\n", name.c_str(), is_dir);

			as_object* item = new as_object(get_player());
			info->set_member(name, item);

			item->set_member("is_dir", is_dir);
			item->set_member("is_readonly", is_readonly);
			item->set_member("is_hidden", is_hidden);
			if (is_dir)
			{
				as_object* info = new as_object(get_player());
				item->set_member("dirinfo", info);
				get_dir(info, path + de->d_name + "/");
			}

		}
		closedir(dir);
	}

	bool misc::get_hdd_serno(tu_string* sn, const char* dev)
	{
		assert(sn);

		if (dev == NULL)
		{
			return false;
		}

	#ifdef WIN32
		//TODO
		return false;
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
			return false;
		}


		//  printf("serial no %s\n", id.serial_no);
		//  printf("model number %s\n", id.model);
		//  printf("firmware revision %s\n", id.fw_rev);
		//  printf("cylinders %s\n", id.cyls);
		//  printf("heads %s\n", id.heads);
		//  printf("sectors per track %s\n", id.sectors);

		*sn = (char*) id.serial_no;

	#endif

	}

	// gets free memory size in KB
	int misc::get_freemem()
	{
	#ifdef WIN32
		return 0;		//TODO
	#else

		int free_mem = 0;
		FILE* fi = fopen("/proc/meminfo", "r");
		if (fi)
		{
			char s[256];
			while (true)
			{
				// read one line
				int i = 0;
				while (i < 255)
				{
					int n = fread(s + i, 1, 1, fi);
					if (n == 0 || s[i] == 0x0A)	// eol ? eof ?
					{
						break;
					}
					i++;
				}
				s[i] = 0;

				const char* p = strstr(s, "MemFree:");
				if (p)
				{
					p = p + 8;
					free_mem = atoi(p);
					break;
				}
			}
			fclose(fi);
		}
		return free_mem;

	#endif
	}
}
