// gameswf_render.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_RENDER_H
#define GAMESWF_RENDER_H

#include "gameswf_types.h"
#include "base/image.h"


namespace gameswf
{
	struct render_handler;

	void	set_render_handler(render_handler* r);
        render_handler*	get_render_handler();
	
};	// end namespace gameswf


#endif // GAMESWF_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
