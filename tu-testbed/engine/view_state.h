// view_state.h	-- Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// "view_state", a struct that holds various important rendering
// states during render traversal.


#ifndef VIEW_STATE_H
#define VIEW_STATE_H


#include "geometry.h"
#include "cull.h"


class view_state
// Description of basic rendering state.  Passed as an arg to
// visual::render().
{
	int	m_frame_number;
	matrix	m_matrix;
	plane_info	m_frustum[6];
};


#endif // VIEW_STATE_H
