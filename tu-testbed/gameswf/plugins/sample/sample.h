#ifndef SAMPLE_H
#define SAMPLE_H

#include "gameswf/gameswf_plugin.h"

struct my_plugin : public gameswf_plugin
{

	my_plugin();
	~my_plugin();

	virtual bool	get_member(const tu_stringi& name, plugin_value* val);
	virtual bool	set_member(const tu_stringi& name, const plugin_value& val);

	void	hello(plugin_value* result, const array<plugin_value>& params);
	
};

#endif

