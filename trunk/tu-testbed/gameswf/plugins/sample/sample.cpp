// the sample of gameswf plugin

// in process of developing for now

#include <stdio.h>

#ifdef _WIN32
	#define gameswf_module __declspec(dllexport)
#else
	
#endif

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

	gameswf_module bool gameswf_module_getmember()
	{
		printf("gameswf_get_member\n");
		return true;
	}

	gameswf_module bool gameswf_module_setmember()
	{
		printf("gameswf_setmember\n");
		return true;
	}

}
