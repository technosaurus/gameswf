// ear_clip_triangulate_sint16.cpp	-- Thatcher Ulrich 2006

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Triangulation, specialized for floating-point coords.


#include "base/ear_clip_triangulate_impl.h"


namespace ear_clip_triangulate {
	void compute(
		array<sint16>* results,
		int path_count,
		const array<sint16> paths[],
		int debug_halt_step,
		array<sint16>* debug_edges)
	{
		coord_type_wrapper<sint16>::compute_triangulation(results, path_count, paths, debug_halt_step, debug_edges);
	}
}

