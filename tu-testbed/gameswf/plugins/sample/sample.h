#ifndef SAMPLE_H
#define SAMPLE_H

#include "gameswf/gameswf_object.h"

using namespace gameswf;

struct my_plugin : public as_plugin
{

	my_plugin(double param);
	~my_plugin();

	virtual bool	get_member(const tu_stringi& name, as_value* val);
	virtual bool	set_member(const tu_stringi& name, const as_value& val);

	void	hello(const char* msg);
	
};

#endif

