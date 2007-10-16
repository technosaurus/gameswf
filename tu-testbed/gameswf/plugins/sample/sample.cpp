// the sample of gameswf plugin

// in process of developing for now

#include <stdio.h>
#include "sample.h"

// methods that is called from Action Scirpt

void	hello(const fn_call& fn)
{
	assert(fn.this_ptr);
	my_plugin* my = (my_plugin*) fn.this_ptr; //hack
	my->hello(fn.arg(0).to_string());
}

// DLL interface

extern "C"
{
	exported_module as_plugin* gameswf_module_init(const array<as_value>& params)
	{
		printf("gameswf_init\n");
		my_plugin* plugin = new my_plugin(params[0].to_number());
		plugin->set_member("hello", hello);
		return plugin;	
	}
}

my_plugin::my_plugin(double param)
{
	printf("new plugin, param=%f\n", param);
}

my_plugin::~my_plugin()
{
//	printf("delete plugin\n");
}

bool	my_plugin::get_member(const tu_stringi& name, as_value* val)
{
//	printf("get_member %s\n", name.c_str());
	return as_object::get_member(name, val);
}

bool	my_plugin::set_member(const tu_stringi& name, const as_value& val)
{
//	printf("set_member %s %s\n", name.c_str(), val.to_string());
	return as_object::set_member(name, val);
}

void	my_plugin::hello(const char* msg)
{
	printf("%s\n", msg);
}
