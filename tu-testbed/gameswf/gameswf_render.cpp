// gameswf_render.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_render.h"

struct render_handler;

namespace gameswf 
{
    static render_handler* s_render_handler;
    
    void set_render_handler(render_handler* r)
    {
        s_render_handler = r;
    }
    
    render_handler* get_render_handler()
    {
        return s_render_handler;
    }
}
