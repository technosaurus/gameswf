// the sample of gameswf plugin

// in process of developing for now

#include <stdio.h>
#include "sample.h"

// methods that is called from Action Scirpt

static void hello(plugin_value* result, gameswf_plugin* this_ptr, const array<plugin_value>& params)
// this method is called from ActionScript
{
	assert(this_ptr);
	my_plugin* my = (my_plugin*) this_ptr; //hack
	my->hello(result, params);
}

// DLL interface

extern "C"
{
	gameswf_module gameswf_plugin* gameswf_module_init()
	{
//		printf("gameswf_init\n");
		my_plugin* plugin = new my_plugin();
		plugin->set_member("hello", hello);
		return plugin;	
	}
}

my_plugin::my_plugin()
{
//	printf("new plugin\n");
}

my_plugin::~my_plugin()
{
//	printf("delete plugin\n");
}

bool	my_plugin::get_member(const tu_stringi& name, plugin_value* val)
{
//	printf("get_member %s\n", name.c_str());
	return gameswf_plugin::get_member(name, val);
}

bool	my_plugin::set_member(const tu_stringi& name, const plugin_value& val)
{
//	printf("set_member %s %s\n", name.c_str(), val.to_string());
	return gameswf_plugin::set_member(name, val);
}

void	my_plugin::hello(plugin_value* result, const array<plugin_value>& params)
{
	printf("hello, world\n");
	printf("params:");
	for (int i = 0, n = params.size(); i < n; i++)
	{
		printf(" %s", params[i].to_string());
	}

	plugin_value pval;
	get_member("xxx", &pval);
	printf("\nxxx=%s\n", pval.to_string());
}
