// constrained_triangulate.h	-- Thatcher Ulrich 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to triangulate arbitrary 2D polygonal regions.
//
// see constrained_triangulate.cpp for more details.


#ifndef CONSTRAINED_TRIANGULATE_H
#define CONSTRAINED_TRIANGULATE_H

#include "base/container.h"
#include "base/tu_types.h"

namespace constrained_triangulate {

	// Output a triangle list.
	void compute(
		array<sint16>* results,
		int path_count,
		const array<sint16> paths[],
		int debug_halt_step = -1,
		array<sint16>* debug_remaining_loop = NULL);
}


#endif // CONSTRAINED_TRIANGULATE_H
