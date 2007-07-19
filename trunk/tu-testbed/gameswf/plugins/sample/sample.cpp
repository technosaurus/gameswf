// the sample of gameswf plugin

// in process of developing for now

#include <stdio.h>
#include "gameswf/gameswf_plugin.h"

#ifdef _WIN32
	#define gameswf_module __declspec(dllexport)
#else
	
#endif


static void hello(plugin_value* result, const array<plugin_value>& params)
{
	printf("hello, world\n");
	for (int i = 0, n = params.size(); i < n; i++)
	{
		printf("%s\n", params[i].to_string());
	}
}

extern "C"
{
	gameswf_module bool gameswf_module_init()
	{
		printf("gameswf_init\n");
		return true;
	}

	gameswf_module bool gameswf_module_close()
	{
		printf("gameswf_close\n");
		return true;
	}

	gameswf_module bool gameswf_module_getmember(const tu_stringi& name, plugin_value* pval)
	{
		printf("gameswf_get_member: %s\n", name.c_str());
		if (name == "hello")
		{
			pval->set_plugin_function_ptr(hello);
			return true;
		}
		else
		if (name == "xxx")
		{
			pval->set_int(111);
			return true;
		}
		return false;
	}

	gameswf_module bool gameswf_module_setmember(const tu_stringi& name, const plugin_value& pval)
	{
		printf("gameswf_setmember: %s %s\n", name.c_str(), pval.m_string_value.c_str());
		return true;
	}

}
