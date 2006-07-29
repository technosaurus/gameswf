// constrained_triangulate.h	-- Thatcher Ulrich 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to triangulate arbitrary 2D polygonal regions.
//
// Ear-clipping (similar to FIST), but loop-less; i.e. not relying on
// ordering of the edges.  This avoids complications with loop twists
// and determining loop order when joining loops together.


#include "base/constrained_triangulate.h"
#include "base/grid_index.h"
#include "base/tu_random.h"
#include "base/vert_types.h"
#include <algorithm>


struct poly_vert {
	vec2<sint16> m_v;
	int m_next;
	int m_prev;
	// flags: DELETED, COINCIDENT, REFLEX, DIRTY, ...?

	poly_vert() : m_next(-1), m_prev(-1) {}
	poly_vert(sint16 x, sint16 y) : m_v(x, y), m_next(-1), m_prev(-1) {}

	index_point<sint16>	get_index_point() const { return index_point<sint16>(m_v.x, m_v.y); }
};


// Represents an edge.  Stores the indices of the two verts.
struct edge {
	int m_0, m_1;

	edge(int v0 = -1, int v1 = -1) : m_0(v0), m_1(v1) {}

	static bool sort_by_startpoint(const edge& a, const edge& b) {
		if (a.m_0 < b.m_0) {
			return true;
		} else if (a.m_0 == b.m_0) {
			return a.m_1 < b.m_1;
		} else {
			return false;
		}
	}

	static bool sort_by_endpoint(const edge& a, const edge& b) {
		if (a.m_1 < b.m_1) {
			return true;
		} else if (a.m_1 == b.m_1) {
			return a.m_0 < b.m_0;
		} else {
			return false;
		}
	}
};

struct active_edge : public edge {
	bool m_in;

	active_edge(int v0 = -1, int v1 = -1, bool in = false) : edge(v0, v1), m_in(in) {}
	active_edge(const edge& e, bool in = false) : edge(e), m_in(in) {}
};


// Info for path joining.
struct path_info {
	// Paths have contiguous indices in original input vert array.
	int m_begin_vert_orig;  // original index of first vert in path
	int m_end_vert_orig;    // original index of last+1 vert in path
	int m_leftmost_vert_orig;
	int m_leftmost_vert_new;

	path_info() :
		m_begin_vert_orig(-1), m_end_vert_orig(-1),
		m_leftmost_vert_orig(-1), m_leftmost_vert_new(-1) {
	}

	bool operator<(const path_info& pi) {
		assert(m_leftmost_vert_new >= 0);
		assert(pi.m_leftmost_vert_new >= 0);

		return m_leftmost_vert_new < pi.m_leftmost_vert_new;
	}
};


// Triangulator state.
struct tristate {
	tristate()
		: m_reflex_point_index(NULL)
	{
	}
	~tristate()
	{
		delete m_reflex_point_index;
	}
	
	array<sint16>* m_results;
	array<poly_vert> m_verts;
	array<edge> m_edges;
	array<path_info> m_input_paths;

	// This is the working set of live edges in the polygon.  We
	// look for ears here.
	array<active_edge> m_active;
	int m_next_active;

	// A search index for fast checking of reflex verts within a
	// triangle.
	//
	// TODO: we don't need a payload, but having one bloats the
	// index by 2x.  Is it worth making a version of
	// grid_index_point that stores the point locations only, with
	// no payload?
	grid_index_point<sint16, bool>* m_reflex_point_index;

	// For debugging.
	int m_debug_halt_step;
	array<sint16>* m_debug_edges;
};


int	compare_vertices(const poly_vert& a, const poly_vert& b)
// For qsort.  Sort by x, then by y.
{
	if (a.m_v.x < b.m_v.x) {
		return -1;
	} else if (a.m_v.x > b.m_v.x) {
		return 1;
	} else {
		if (a.m_v.y < b.m_v.y)
			return -1;
		else if (a.m_v.y > b.m_v.y)
			return 1;
	}

	return 0;
}


// Helper for sorting verts.
struct vert_index_sorter {
	vert_index_sorter(const array<poly_vert>& verts)
		: m_verts(verts) {
	}

	// Return true if m_verts[a] < m_verts[b].
	bool operator()(int a, int b) {
		return compare_vertices(m_verts[a], m_verts[b]) == -1;
	}

	const array<poly_vert>& m_verts;
};


void edges_intersect_sub(int* e0_vs_e1, int* e1_vs_e0,
			 const vec2<sint16>& e0v0, const vec2<sint16>& e0v1,
			 const vec2<sint16>& e1v0, const vec2<sint16>& e1v1)
// Return {-1,0,1} for edge A {crossing, vert-touching, not-crossing}
// the line of edge B.
//
// Specialized for sint16
{
	// If e1v0,e1v1 are on opposite sides of e0, and e0v0,e0v1 are
	// on opposite sides of e1, then the segments cross.  These
	// are all determinant checks.

	// The main degenerate case we need to watch out for is if
	// both segments are zero-length.
	//
	// If only one is degenerate, our tests are still OK.

	if (e0v0.x == e0v1.x && e0v0.y == e0v1.y)
	{
		// e0 is zero length.
		if (e1v0.x == e1v1.x && e1v0.y == e1v1.y)
		{
			if (e1v0.x == e0v0.x && e1v0.y == e0v0.y) {
				// Coincident.
				*e0_vs_e1 = 0;
				*e1_vs_e0 = 0;
				return;
			}
		}
	}

	// See if e1 crosses line of e0.
	sint64	det10 = determinant_sint16(e0v0, e0v1, e1v0);
	sint64	det11 = determinant_sint16(e0v0, e0v1, e1v1);

	// Note: we do > 0, which means a vertex on a line counts as
	// intersecting.  In general, if one vert is on the other
	// segment, we have to go searching along the path in either
	// direction to see if it crosses or not, and it gets
	// complicated.  Better to treat it as intersection.

	int det1sign = 0;
	if (det11 < 0) det1sign = -1;
	else if (det11 > 0) det1sign = 1;
	if (det10 < 0) det1sign = -det1sign;
	else if (det10 == 0) det1sign = 0;

	if (det1sign > 0) {
		// e1 doesn't cross the line of e0.
		*e1_vs_e0 = 1;
	} else if (det1sign < 0) {
		// e1 does cross the line of e0.
		*e1_vs_e0 = -1;
	} else {
		// One (or both) of the endpoints of e1 are on the
		// line of e0.
		*e1_vs_e0 = 0;
	}

	// See if e0 crosses line of e1.
	sint64	det00 = determinant_sint16(e1v0, e1v1, e0v0);
	sint64	det01 = determinant_sint16(e1v0, e1v1, e0v1);

	int det0sign = 0;
	if (det01 < 0) det0sign = -1;
	else if (det01 > 0) det0sign = 1;
	if (det00 < 0) det0sign = -det0sign;
	else if (det00 == 0) det0sign = 0;

	if (det0sign > 0) {
		// e0 doesn't cross the line of e1.
		*e0_vs_e1 = 1;
	} else if (det0sign < 0) {
		// e0 crosses line of e1
		*e0_vs_e1 = -1;
	} else {
		// One (or both) of the endpoints of e0 are on the
		// line of e1.
		*e0_vs_e1 = 0;
	}
}


bool any_edge_intersects(const tristate* ts, const array<edge>& edgeset, const edge& e)
// Return true if any edge in the given edgeset intersects the edge e.
//
// Intersection is defined as any part of an edgeset edge touching any
// part of the *interior* of e.
{
	for (int i = 0; i < edgeset.size(); i++) {
		const edge& ee = edgeset[i];

		int e_vs_ee, ee_vs_e;
		edges_intersect_sub(&e_vs_ee, &ee_vs_e,
				    ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v,
				    ts->m_verts[ee.m_0].m_v, ts->m_verts[ee.m_1].m_v);

		bool e_crosses_line_of_ee = e_vs_ee < 0;
		bool ee_touches_line_of_e = ee_vs_e <= 0;
		if (e_crosses_line_of_ee && ee_touches_line_of_e) {
			return true;
		}
	}
	return false;
}


int find_valid_bridge_vert(const tristate* ts, int v1 /*, edge index */)
// Find v2 such that v2 is left of v1, and the segment v1-v2 doesn't
// cross any edges.
{
	// We work backwards from v1.  Normally we shouldn't have to
	// look very far.
	assert(v1 > 0);
	for (int i = v1 - 1; i >= 0; i--) {
		if (!any_edge_intersects(ts, ts->m_edges, edge(v1, i))) {
			// This vert is good.
			return i;
		}
	}

	// If we get here, then the input is not well formed.
	// TODO log something
	assert(0);  // temp xxxxx

	// Default fallback: join to the next-most-leftmost vert,
	// regardless of crossing other edges.
	return v1 - 1;
}


void join_paths_into_one_poly(tristate* ts, const array<int>& old_to_new)
// Use zero-area bridges to connect separate polys & islands into one
// big continuous poly.
//
// Uses info from ts->m_input_paths and the old_to_new remap table.
{
	// Connect separate paths with bridge edges, into one big path.
	//
	// Bridges are zero-area regions that connect a vert on each
	// of two paths.
	if (ts->m_input_paths.size() > 1) {
		// Sort polys in order of each poly's leftmost vert.
		std::sort(&ts->m_input_paths[0], &ts->m_input_paths[0] + ts->m_input_paths.size());
		assert(ts->m_input_paths[0].m_leftmost_vert_new <= ts->m_input_paths[1].m_leftmost_vert_new);

		// TODO
		//full_poly->init_edge_index(m_sorted_verts, m_bound);

		// Iterate from left to right
		for (int i = 1; i < ts->m_input_paths.size(); i++) {
			const path_info& pi = ts->m_input_paths[i];
			
			int	v1 = pi.m_leftmost_vert_new;
			assert(v1 >= 0);

			//     find a vert v2, such that:
			//       v2 is to the left of v1,
			//       and v1-v2 seg doesn't intersect any other edges

			//     // (note that since v1 is next-most-leftmost, v1-v2 can't
			//     // hit anything in pi, nor any paths further down the list,
			//     // it can only hit edges in the joined poly) (need to think
			//     // about equality cases)
			//
			if (v1 > 0) {
				int	v2 = find_valid_bridge_vert(ts, v1 /*, edge index */);
				assert(v2 != v1);

				// Join pi.
				ts->m_edges.push_back(edge(v1, v2));
				ts->m_edges.push_back(edge(v2, v1));
			} // else no joining required; vert[0] is already joined and v1 is coincident with it
			
			// TODO: update edge index
		}
	}
}


void init(tristate* ts, array<sint16>* results, int path_count, const array<sint16> paths[],
	  int debug_halt_step, array<sint16>* debug_edges)
// Pull the paths into *tristate.
{
	assert(results);
	ts->m_results = results;
	ts->m_debug_halt_step = debug_halt_step;
	ts->m_debug_edges = debug_edges;
	ts->m_next_active = 0;

	// TODO: verify no verts on boundary of coord bounding box.

	// Dump verts and edges into tristate.
	index_box<sint16> bbox;
	array<poly_vert> verts;
	for (int i = 0; i < path_count; i++) {
		assert((paths[i].size() & 1) == 0);

		// Keep some info about the path itself (so we can
		// sort and join paths later).
		ts->m_input_paths.resize(ts->m_input_paths.size() + 1);
		path_info* pi = &ts->m_input_paths.back();
		pi->m_begin_vert_orig = verts.size();

		// Grab all the verts and edges.
		for (int j = 0; j < paths[i].size(); j += 2) {
			int vert_index = verts.size();
			verts.push_back(poly_vert(paths[i][j], paths[i][j + 1]));
			edge e(vert_index, vert_index + 1);
			ts->m_edges.push_back(e);

			// Update bounding box.
			if (vert_index == 0) {
				bbox.set_to_point(verts.back().get_index_point());
			} else {
				bbox.expand_to_enclose(verts.back().get_index_point());
			}

			
			if (pi->m_leftmost_vert_orig == -1
			    || compare_vertices(verts[pi->m_leftmost_vert_orig], verts[vert_index]) > 0) {
				pi->m_leftmost_vert_orig = vert_index;
			}
		}
		ts->m_edges.back().m_1 = pi->m_begin_vert_orig;  // close the path
		pi->m_end_vert_orig = verts.size();
	}

	// Init reflex point search index.
	//
	// Compute grid density.
	int	x_cells = 1;
	int	y_cells = 1;
	if (verts.size() > 0)
	{
		const float	GRID_SCALE = sqrtf(0.5f);
		sint16	width = bbox.get_width();
		sint16	height = bbox.get_height();
		float	area = float(width) * float(height);
		if (area > 0)
		{
			float	sqrt_n = sqrt((float) verts.size());
			float	w = width * width / area * GRID_SCALE;
			float	h = height * height / area * GRID_SCALE;
			x_cells = int(w * sqrt_n);
			y_cells = int(h * sqrt_n);
		}
		else
		{
			// Zero area.
			if (width > 0)
			{
				x_cells = int(GRID_SCALE * GRID_SCALE * verts.size());
			}
			else
			{
				y_cells = int(GRID_SCALE * GRID_SCALE * verts.size());
			}
		}
		x_cells = iclamp(x_cells, 1, 256);
		y_cells = iclamp(y_cells, 1, 256);
	}
	ts->m_reflex_point_index = new grid_index_point<sint16, bool>(bbox, x_cells, y_cells);
	
	for (int i = 0; i < ts->m_input_paths.size(); i++) {
		const path_info* pi = &ts->m_input_paths[i];
		
		// Identify any reflex verts and put them in an index.
		int pathsize = pi->m_end_vert_orig - pi->m_begin_vert_orig;
		if (pathsize > 2) {
			// Thanks Sean Barrett for the fun/sneaky
			// trick for iterating over subsequences of 3
			// verts.
			for (int j = pi->m_begin_vert_orig, k = pi->m_end_vert_orig - 1, l = pi->m_end_vert_orig - 2;
			     j < pi->m_end_vert_orig;
			     l = k, k = j, j++) {
				const vec2<sint16>& v0 = verts[l].m_v;
				const vec2<sint16>& v1 = verts[k].m_v;
				const vec2<sint16>& v2 = verts[j].m_v;
				if (vertex_left_test(v0, v1, v2) <= 0) {
					ts->m_reflex_point_index->add(index_point<sint16>(v1.x, v1.y), 0);
				}
			}
		}
	}

	// TODO: find & fix edge intersections

	//
	// Sort.
	//

	// Sort.
	array<int> vert_indices(verts.size());   // verts[vert_indices[0]] --> smallest vert
	for (int i = 0; i < verts.size(); i++) {
		vert_indices[i] = i;
	}
	vert_index_sorter sorter(verts);
	std::sort(&vert_indices[0], &vert_indices[0] + vert_indices.size(), sorter);

	// Make the old-to-new mapping.
	array<int> old_to_new;
	old_to_new.resize(verts.size());

	// Remove dupes.
	int last_unique_vi = 0;
	for (int i = 1; i < vert_indices.size(); i++) {
		int old_index = vert_indices[i];
		if (verts[old_index].m_v == verts[vert_indices[last_unique_vi]].m_v) {
			// Dupe!  Don't increment last_unique_vi.
		} else {
			last_unique_vi++;
			vert_indices[last_unique_vi] = old_index;
			old_to_new[old_index] = last_unique_vi;
		}
		old_to_new[old_index] = last_unique_vi;
	}
	vert_indices.resize(last_unique_vi + 1);  // drop duped verts.

// 	// Make old_to_new mapping.
// 	for (int i = 0; i < vert_indices.size(); i++) {
// 		int old_index = vert_indices[i];
// 		old_to_new[old_index] = i;
// 	}

	// Make output array.
	ts->m_verts.resize(vert_indices.size());
	for (int i = 0; i < vert_indices.size(); i++) {
		ts->m_verts[i] = verts[vert_indices[i]];
	}

	// Remap edge indices.
	for (int i = 0; i < ts->m_edges.size(); i++) {
		ts->m_edges[i].m_0 = old_to_new[ts->m_edges[i].m_0];
		ts->m_edges[i].m_1 = old_to_new[ts->m_edges[i].m_1];
	}

	// Update path info.
	for (int i = 0; i < ts->m_input_paths.size(); i++) {
		ts->m_input_paths[i].m_leftmost_vert_new = old_to_new[ts->m_input_paths[i].m_leftmost_vert_orig];
	}

	join_paths_into_one_poly(ts, old_to_new);

	// Sort the edges.  This makes it faster to locate the edges
	// of interest when evaluating an ear.
	std::sort(&ts->m_edges[0], &ts->m_edges[0] + ts->m_edges.size(), edge::sort_by_startpoint);
	// TODO: keep a m_edges_by_endpoint sorted list...
}


bool vert_in_triangle(const vec2<sint16>& v, const vec2<sint16>& v0, const vec2<sint16>& v1, const vec2<sint16>& v2)
// Return true if v touches the boundary or the interior of triangle (v0, v1, v2).  
{
	sint64 det0 = determinant_sint16(v0, v1, v);
	if (det0 >= 0) {
		sint64 det1 = determinant_sint16(v1, v2, v);
		if (det1 >= 0) {
			sint64 det2 = determinant_sint16(v2, v0, v);
			if (det2 >= 0) {
				// Point touches the triangle.
				return true;
			}
		}
	}
	return false;
}


bool any_reflex_vert_in_triangle(const tristate* ts, int vi0, int vi1, int vi2)
// Return true if there is any reflex vertex in tristate that touches
// the interior or edges of the given triangle.  Verts coincident with
// the actual triangle verts will return false.
{
	const vec2<sint16>& v0 = ts->m_verts[vi0].m_v;
	const vec2<sint16>& v1 = ts->m_verts[vi1].m_v;
	const vec2<sint16>& v2 = ts->m_verts[vi2].m_v;

	const index_point<sint16>& ip0 = ts->m_verts[vi0].get_index_point();
	const index_point<sint16>& ip1 = ts->m_verts[vi1].get_index_point();
	const index_point<sint16>& ip2 = ts->m_verts[vi2].get_index_point();

	// Compute the bounding box of reflex verts we want to check.
	index_box<sint16>	query_bound(ip0);
	query_bound.expand_to_enclose(ip1);
	query_bound.expand_to_enclose(ip2);

	for (grid_index_point<sint16, bool>::iterator it = ts->m_reflex_point_index->begin(query_bound);
	     ! it.at_end();
	     ++it)
	{
		if (ip0 == it->location || ip1 == it->location || ip2 == it->location) {
			continue;
		}

		if (query_bound.contains_point(it->location)) {
			vec2<sint16> v(it->location.x, it->location.y);
			if (vert_in_triangle(v, v0, v1, v2)) {
				return true;
			}
		}
	}
	return false;

// 	// TODO: use point index!
// 	//
// 	// TODO: try checking only reflex verts.
// 	for (int i = 0; i < ts->m_verts.size(); i++) {
// 		if (i == vi0 || i == vi1 || i == vi2) {
// 			// Concident -- exclude.
// 			continue;
// 		}
// 		const vec2<sint16>& v = ts->m_verts[i].m_v;
// 		if (v == v0 || v == v1 || v == v2) {
// 			// Coincident.
// 			assert(0); // we should have already removed all dupes!
// 			continue;
// 		}

// 		if (vert_in_triangle(v, v0, v1, v2)) {
// 			return true;
// 		}
// 	}
	return false;
}


bool vertex_in_cone(const vec2<sint16>& vert,
		    const vec2<sint16>& cone_v0, const vec2<sint16>& cone_v1, const vec2<sint16>& cone_v2)
// Returns true if vert is within the cone defined by [v0,v1,v2].
/*
//  (out)  v0
//        /
//    v1 <   (in)
//        \
//         v2
*/
{
	bool	acute_cone = vertex_left_test(cone_v0, cone_v1, cone_v2) > 0;

	// Include boundary in our tests.
	bool	left_of_01 =
		vertex_left_test(cone_v0, cone_v1, vert) >= 0;
	bool	left_of_12 =
		vertex_left_test(cone_v1, cone_v2, vert) >= 0;

	if (acute_cone)
	{
		// Acute cone.  Cone is intersection of half-planes.
		return left_of_01 && left_of_12;
	}
	else
	{
		// Obtuse cone.  Cone is union of half-planes.
		return left_of_01 || left_of_12;
	}
}


void fill_debug_out(tristate* ts)
{
	// Put the current active edges in the debug output.
	for (int i = 0; i < ts->m_active.size(); i++) {
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.x);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.y);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.x);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.y);
	}
}


bool check_debug_dump(tristate* ts)
// If we should debug dump now, this function returns true and fills
// the debug output.
//
// Call this each iteration of triangulate_plane(); if it returns
// true, then return early.
{
	if (ts->m_debug_halt_step > 0) {
		ts->m_debug_halt_step--;
		if (ts->m_debug_halt_step == 0) {
			// Emit some debug info.
			fill_debug_out(ts);
			return true;
		}
	}
	return false;
}


int find_ear_edge(const tristate* ts, const edge& e)
// Look through m_active to find an edge e1 such that:
//
// * e-e1 is a left turn, and there are no other edges going into or
// out of the apex vert.
//
// * e and e1 are not both degenerate.
//
// * triangle e-e1 doesn't contain any (reflex) verts
//
// Return the index of oe, or -1 if we don't find anything valid.
{
	assert(e.m_0 != e.m_1);  // zero-length edges should not exist

	// Find an edge incident on e.m_0, whose other vert is a valid
	// left turn for edge e.

	// xxxx collect incident edges for second pass
	array<int> in_edges;

	const active_edge* out_edges_begin = std::lower_bound(
		&ts->m_active[0], &ts->m_active[0] + ts->m_active.size(),
		active_edge(e.m_1, 0), edge::sort_by_startpoint);
	const active_edge* out_edges_end = out_edges_begin;
	while (out_edges_end < (&ts->m_active[0] + ts->m_active.size())
	       && edge::sort_by_startpoint(*out_edges_end, active_edge(e.m_1 + 1, 0))) {
		out_edges_end++;
	}

	// TODO: use sorted edge list instead of checking all of m_active!
	int oe_i = -1;   // oe_i --> "out-edge index", index of inside-most outgoing edge (the other edge of the potential ear)
	int ie_i = -1;   // ie_i --> "in-edge index", index of inside-most incoming edge (can invalidate the ear)
	for (int i = 0; i < ts->m_active.size(); i++) {
		const edge& te = ts->m_active[i];  // te --> "temp edge"
		if (te.m_1 == e.m_1) {
			// in edge
			in_edges.push_back(i);
		}
	}
	for (const active_edge* p = out_edges_begin; p < out_edges_end; p++) {
		const edge& te = *p;
		assert(te.m_0 == e.m_1);

		// Is this a valid out edge?
		if (vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[te.m_1].m_v) > 0) {
			// Is this the inside-most outgoing edge so far?
			if (oe_i == -1
			    || vertex_in_cone(ts->m_verts[te.m_1].m_v,
					      ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ts->m_active[oe_i].m_1].m_v)) {
				oe_i = (p - &ts->m_active[0]);
			}
		}
	}
	// See if an in-edge is in our cone.
	if (oe_i >= 0) {
		// Check for an edge in the way.
		for (int i = 0; i < in_edges.size(); i++) {
			const edge& ie = ts->m_active[in_edges[i]];
			if (&ie != &e
			    && vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ie.m_0].m_v) > 0
			    && vertex_in_cone(ts->m_verts[ie.m_0].m_v,
					      ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ts->m_active[oe_i].m_1].m_v)) {
				// Can't clip this ear; there's an edge in the way.
				return -1;
			}
		}

		// Make sure at least one of the ear sides is not
		// degenerate.
		int valence0 = 0;
		int valence1 = 0;
		for (int i = 0; i < in_edges.size(); i++) {
			const edge& ie = ts->m_active[in_edges[i]];
			assert(ie.m_1 == e.m_1);
			if (ie.m_0 == ts->m_active[oe_i].m_1) {
				// coincident with oe_i, but in reverse
				valence1--;
			} else if (ie.m_0 == e.m_0) {
				// coincident with e
				valence0++;
			}
		}
		for (const active_edge* p = out_edges_begin; p < out_edges_end; p++) {
			const edge& oe = *p;
			assert(oe.m_0 == e.m_1);
			if (oe.m_1 == ts->m_active[oe_i].m_1) {
				// coincident with oe_i
				valence1++;
			} else if (oe.m_1 == e.m_0) {
				// coincident with e, but in reverse
				valence0--;
			}
		}
		if (valence0 < 1 && valence1 < 1) {
			return -1;
		}

		if (!any_reflex_vert_in_triangle(ts, e.m_0, e.m_1, ts->m_active[oe_i].m_1)) {
			return oe_i;
		}
	}

	// No valid other edge found.
	return -1;
}


bool find_and_clip_ear(tristate* ts, int* next_vert)
// Return true if we found an ear to clip.
{
	// Dumb slow brute-force, to see if it works.

	for (int i = 0; i < ts->m_active.size(); i++) {
		int oe_i = find_ear_edge(ts, ts->m_active[i]);
		if (oe_i >= 0) {
			assert(oe_i != i);
			// clip it!
			int i0 = ts->m_active[i].m_0;
			int i1 = ts->m_active[i].m_1;
			assert(i1 == ts->m_active[oe_i].m_0);
			int i2 = ts->m_active[oe_i].m_1;

			// Replace first edge of the ear with the newly created edge.
			ts->m_active[i].m_1 = i2;
			// TODO: mark dirty flags.
			//
			// Remove second edge of the ear.
			ts->m_active.remove(oe_i);
			//ts->m_active.remove(i);
			//ts->m_active.push_back(active_edge(i0, i2));
			// Emit triangle.
			ts->m_results->push_back(ts->m_verts[i0].m_v.x);
			ts->m_results->push_back(ts->m_verts[i0].m_v.y);
			ts->m_results->push_back(ts->m_verts[i1].m_v.x);
			ts->m_results->push_back(ts->m_verts[i1].m_v.y);
			ts->m_results->push_back(ts->m_verts[i2].m_v.x);
			ts->m_results->push_back(ts->m_verts[i2].m_v.y);
			return true;
		}
	}

	return false;
}


// Edge index ideas.  Needs to, given a vert, quickly find all edges
// into and out of that vert.
//
// * use two sorted lists of edges, m_out and m_in.  m_out is sorted
// by m_0, and m_in is sorted by m_1.  Edges for a particular vert are
// together; to find them, just do binary search.
//
// * when updating edge list, we always drop two and add one.  So to
// update indices, replace one edge (fortunately the new edge touches
// a vert we need to replace, so it can stay where it is) and mark the
// other one as invalid (perhaps with a skip count to get to the next
// valid edge?)
//
// * these indices can directly replace our m_active list.

// Problems with the above: it makes what should be O(1) (finding
// edges incident to a vertex) an O(logN) operation.  Plus it's still
// not that lean in terms of memory, because you need two sorted lists
// (one for incoming, one for outgoing).  Also, deleting edges is kind
// of awkward.

// Alternative: use loops (a la FIST), but have special logic to deal
// with coincident vertices.  If a vert is not coincident, then normal
// loop logic works fine.  Most verts are expected to not be
// coincident.  This should be fairly compact, simple and efficient.


void triangulate_plane(tristate* ts)
// Make triangles.
{
	assert(ts->m_active.size() == 0);

	// Ear-clip, not based on loops.  (Reasoning: don't need to be
	// careful to do non-local analysis when constructing master
	// loop.)
	//
	// Copy all input edges to active list.
	ts->m_active.reserve(ts->m_edges.size());
	for (int i = 0; i < ts->m_edges.size(); i++) {
		const edge& e = ts->m_edges[i];
		if (e.m_0 != e.m_1) {  // omit zero-length edges
			ts->m_active.push_back(active_edge(e.m_0, e.m_1));
		}
	}
	// Clip all available ears.
	int next_vert = 0;
	while (find_and_clip_ear(ts, &next_vert)) {
		if (check_debug_dump(ts)) {
			return;
		}
	}
}


namespace constrained_triangulate {
	void compute(
		array<sint16>* results,
		int path_count,
		const array<sint16> paths[],
		int debug_halt_step,
		array<sint16>* debug_edges)
	{
		tristate ts;
		init(&ts, results, path_count, paths, debug_halt_step, debug_edges);
		triangulate_plane(&ts);
	}
}


/* triangulation notes

Nice lecture notes:
http://arachne.ics.uci.edu/~eppstein/junkyard/godfried.toussaint.html

Narkhede & Manocha's description/code of Seidel's alg:
http://www.cs.unc.edu/~dm/CODE/GEM/chapter.html

Some school project notes w/ triangulation overview & diagrams:
http://www.mema.ucl.ac.be/~wu/FSA2716-2002/project.html

Toussaint paper about sleeve-following, including interesting
description & opinion on various other algorithms:
http://citeseer.ist.psu.edu/toussaint91efficient.html

Toussaint outline & links:
http://cgm.cs.mcgill.ca/~godfried/teaching/cg-web.html


http://geometryalgorithms.com/algorithms.htm

History Of Triangulation Algorithms
http://cgm.cs.mcgill.ca/~godfried/teaching/cg-projects/97/Thierry/thierry507webprj/complexity.html

Ear Cutting For Simple Polygons
http://cgm.cs.mcgill.ca/~godfried/teaching/cg-projects/97/Ian//cutting_ears.html

Intersections for a set of 2D segments
http://geometryalgorithms.com/Archive/algorithm_0108/algorithm_0108.htm

Simple Polygon Triangulation
http://cgafaq.info/wiki/Simple_Polygon_Triangulation

KKT O(n log log n) algo
http://portal.acm.org/citation.cfm?id=150693&dl=ACM&coll=portal&CFID=11111111&CFTOKEN=2222222

Poly2Tri implemenation, good notes and looks like good code, sadly the
license is non-commercial only:
http://www.mema.ucl.ac.be/~wu/Poly2Tri/poly2tri.html

FIST
http://www.cosy.sbg.ac.at/~held/projects/triang/triang.html

Nice slides on monotone subdivision & triangulation:
http://www.cs.ucsb.edu/~suri/cs235/Triangulation.pdf

Interesting forum post re monotone subdivision in Amanith:
http://www.amanith.org/forum/viewtopic.php?pid=43

*/
