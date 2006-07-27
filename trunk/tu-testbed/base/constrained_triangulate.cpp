// constrained_triangulate.h	-- Thatcher Ulrich 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to triangulate arbitrary 2D polygonal regions.
//
// This uses an experimental algorithm.  The basic idea is to eschew
// maintaining a notion of loop boundaries, and deal with raw points
// and edges only.  The thought is that this avoids having to deal
// with tricky consistency issues in degenerate inputs (long thin
// 0-area bridges and such, which cause problems when implementing
// FIST).
//
// We basically flood-fill triangles inside the convex hull of the
// input (including "negative" spaces), and keep track of input-edge
// crossings to determine if our tris are in/out.

// questions:
//
// * does flood-filling easily give us correct odd-even rules (in the
// face of various kinds of degenerate inputs)? --> Yes.
//
// * can we get positive/negative fill rule?
//
// * can flood-filling be made fast?  (Issue is search for valid next
// vert; maybe have to do line-sweep approach to make it simple/fast?)
//
// * how does it fail when given bad input?  (i.e. self-intersection)
// -- do we have to pre-resolve self-intersections, or is it OK to
// just leave them in if they're minor?  Self-intersecting input is
// not good; it can block our fill and make us terminate early.



#include "base/constrained_triangulate.h"
#include "base/tu_random.h"
#include "base/vert_types.h"
#include <algorithm>


struct poly_vert {
	vec2<sint16> m_v;

	poly_vert() {}
	poly_vert(sint16 x, sint16 y) : m_v(x, y) {}
};


// Represents an edge.  Stores the indices of the two verts.
struct edge {
	int m_0, m_1;

	edge(int v0 = -1, int v1 = -1) : m_0(v0), m_1(v1) {}
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
	array<sint16>* m_results;
	array<poly_vert> m_verts;
	array<edge> m_input;
	array<path_info> m_input_paths;

	// NOTE: an unstructured active edge set is a very robust way
	// to flood-fill.  The problems:
	//
	// * how to efficiently advance?  The obvious thing is, given
	// an edge, search for a valid vert to join with.  But that
	// kind of search is expensive.
	//
	// * would be nice to have a way to discount verts that are
	// completely filled around.  Looks like this can be done
	// using a bounding box, and counting incident edges vs. tris
	// for each vert.
	array<active_edge> m_active;
	int m_next_active;

	// NOTE: m_sweep is a nice easy way to determine whether a
	// vert has been dealt with or not, and keep a set of active
	// verts to try to clip ears off of.  The achilles heels are:
	//
	// * sometimes/frequently have to jump the sweep frontier to
	// an unconnected vert, i.e. relatively expensive or
	// complicated search
	//
	// * or, can advance the sweep frontier using zero-area
	// probes, but then have to keep the loop polyline from
	// getting twisted --> complicated and potentially expensive.
	array<int> m_sweep;  // this is a polyloop that sweeps through the shape
	array<bool> m_swept;  // TODO: put this flag in the poly_vert struct?  Or, bitarray.

	array<edge> m_created;

	// Ideas:
	//
	// * line sweep?
	//
	// * active edge set; preprocess to connect islands, similar
	// to FIST; only advance an active edge by crawling an
	// existing (exposed) edge.

	// For debugging.
	int m_debug_halt_step;
	array<sint16>* m_debug_edges;
};


void update_seed_edge(tristate* ts, const edge& e)
// Update m_active[0] based on e.  If e is a better seed edge than
// m_active[0], then replace m_active[0].  If e is coincident with
// m_active[0], then toggle its m_in status.
{
	if (ts->m_active.size() == 0) {
		// Make sure edge points upwards.
		if (ts->m_verts[e.m_0].m_v.y < ts->m_verts[e.m_1].m_v.y) {
			ts->m_active.push_back(active_edge(e.m_0, e.m_1, false));
		} else {
			ts->m_active.push_back(active_edge(e.m_1, e.m_0, false));
		}
		return;
	}

	assert(ts->m_active.size() == 1);

	vec2<sint16> ev0 = ts->m_verts[e.m_0].m_v;
	vec2<sint16> ev1 = ts->m_verts[e.m_1].m_v;

	vec2<sint16> sv0 = ts->m_verts[ts->m_active[0].m_0].m_v;
	vec2<sint16> sv1 = ts->m_verts[ts->m_active[0].m_1].m_v;

	if ((ev0 == sv0 && ev1 == sv1) || (ev0 == sv1 && ev1 == sv0)) {
		// Coincident; no need to do anything.
		return;
	}

	// Decide if e is a better seed edge than m_active[0].
	bool e_is_better = false;
	if (sv0 == sv1) {
		e_is_better = true;
	} else if (!(ev0 == ev1)) {
		// Sort verts.
		if (ev1 < ev0) {
			swap(&ev0, &ev1);
		}
		if (sv1 < sv0) {
			swap(&sv0, &sv1);
		}
		if (ev0 < sv0) {
			e_is_better = true;
		} else if (ev0 == sv0) {
			// Take the edge which is more vertical.
			int64 verticalness_e = i64abs((ev1.y - ev0.y) * (sv1.x - sv0.x));
			int64 verticalness_s = i64abs((sv1.y - sv0.y) * (ev1.x - ev0.x));
			if (verticalness_e > verticalness_s) {
				e_is_better = true;
			} else if (verticalness_e == verticalness_s) {

			}

			// see if ev1 is closer to the outside of the
			// shape than the existing seed edge.
			int e1_is_left = vertex_left_test(ev0, ev1, sv1);
			if (e1_is_left == 0) {
				// Take the shorter edge.
				if (ev1.x < sv1.x) {
					e_is_better = true;
				}
			} else {
				// Same slope; prefer the shorter edge.
				if (ev1.x < sv1.x) {
					e_is_better = true;
				}
			}
		}
	}

	if (e_is_better) {
		// Replace m_active[0].  Edge must point upward.
		if (ts->m_verts[e.m_0].m_v.y < ts->m_verts[e.m_1].m_v.y) {
			ts->m_active[0] = active_edge(e.m_0, e.m_1, false);
		} else {
			ts->m_active[0] = active_edge(e.m_1, e.m_0, false);
		}
	}
}


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


void mark_joined(array<bool>* joined, const path_info& pi, const array<int> old_to_new)
// Helper to set joined[] values to true for verts from path pi.
{
	for (int i = pi.m_begin_vert_orig; i < pi.m_end_vert_orig; i++) {
		int ni = old_to_new[i];
		(*joined)[ni] = true;
	}
}


int find_valid_bridge_vert(const tristate* ts, int v1 /*, edge index */)
// Find v2 such that v2 is left of v1, and the segment v1-v2 doesn't
// cross any edges.
{
	assert(v1 > 0);
	for (int i = v1 - 1; i >= 0; i--) {
		if (!any_edge_intersects(ts, ts->m_input, edge(v1, i))) {
			// This vert is good.
			return i;
		}
	}

	// If we get here, then the input is not well formed.
	// TODO log something
	assert(0);  // temp xxxxx

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
		array<bool> joined;
		joined.resize(ts->m_verts.size());
		
		std::sort(&ts->m_input_paths[0], &ts->m_input_paths[0] + ts->m_input_paths.size());
		
		// Sort polys in order of each poly's leftmost vert.
		assert(ts->m_input_paths[0].m_leftmost_vert_new <= ts->m_input_paths[1].m_leftmost_vert_new);

		// Assume that the enclosing boundary is the leftmost
		// path; this is true if the regions are valid and
		// don't cross.

		mark_joined(&joined, ts->m_input_paths[0], old_to_new);

		// TODO
		//full_poly->init_edge_index(m_sorted_verts, m_bound);

		// Iterate from left to right
		for (int i = 1; i < ts->m_input_paths.size(); i++) {
			const path_info& pi = ts->m_input_paths[i];
			
			int	v1 = pi.m_leftmost_vert_new;
			assert(v1 >= 0);

			//     find a joined v2, such that:
			//       v2 is to the left of v1,
			//       and v1-v2 seg doesn't intersect any other edges

			//     // (note that since v1 is next-most-leftmost, v1-v2 can't
			//     // hit anything in pi, nor any paths further down the list,
			//     // it can only hit edges in the joined poly) (need to think
			//     // about equality cases)
			//
			if (v1 > 0 && !joined[v1]) {
				int	v2 = find_valid_bridge_vert(ts, v1 /*, edge index */);
				assert(v2 != v1);
				assert(joined[v2]);

				// Join pi.
				ts->m_input.push_back(edge(v1, v2));
				ts->m_input.push_back(edge(v2, v1));
			}
			mark_joined(&joined, pi, old_to_new);
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
	array<poly_vert> verts;
	for (int i = 0; i < path_count; i++) {
		assert((paths[i].size() & 1) == 0);

		ts->m_input_paths.resize(ts->m_input_paths.size() + 1);
		path_info* pi = &ts->m_input_paths.back();
		pi->m_begin_vert_orig = verts.size();
		
		for (int j = 0; j < paths[i].size(); j += 2) {
			int vert_index = verts.size();
			verts.push_back(poly_vert(paths[i][j], paths[i][j + 1]));
			edge e(vert_index, vert_index + 1);
			ts->m_input.push_back(e);

			if (pi->m_leftmost_vert_orig == -1
			    || compare_vertices(verts[pi->m_leftmost_vert_orig], verts[vert_index]) > 0) {
				pi->m_leftmost_vert_orig = vert_index;
			}
		}
		ts->m_input.back().m_1 = pi->m_begin_vert_orig;  // close the path
		pi->m_end_vert_orig = verts.size();
	}

	// TODO: find & fix edge intersections

// 	// Add in the corners of the bounding box, as synthetic verts.
// 	// We are going to use these to initialize our sweep polyline.
// 	// TODO: size the bounding box according to the input.
// 	verts.push_back(poly_vert(-32768, -32768));
// 	verts.push_back(poly_vert(-32768,  32767));
// 	verts.push_back(poly_vert( 32767,  32767));
// 	verts.push_back(poly_vert( 32767, -32768));

	//
	// Sort and scrub dupes.
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

	// Make output array.
	ts->m_verts.resize(vert_indices.size());
	for (int i = 0; i < vert_indices.size(); i++) {
		ts->m_verts[i] = verts[vert_indices[i]];
	}

	// Remap edge indices.
	for (int i = 0; i < ts->m_input.size(); i++) {
		ts->m_input[i].m_0 = old_to_new[ts->m_input[i].m_0];
		ts->m_input[i].m_1 = old_to_new[ts->m_input[i].m_1];
	}

	// Update path info.
	for (int i = 0; i < ts->m_input_paths.size(); i++) {
		ts->m_input_paths[i].m_leftmost_vert_new = old_to_new[ts->m_input_paths[i].m_leftmost_vert_orig];
	}

	join_paths_into_one_poly(ts, old_to_new);

// 	// Make sure the synthetic verts ended up in the expected indices.
// 	assert(ts->m_verts[0].m_v.x == -32768);
// 	assert(ts->m_verts[0].m_v.y == -32768);
// 	assert(ts->m_verts[1].m_v.x == -32768);
// 	assert(ts->m_verts[1].m_v.y ==  32767);
// 	assert(ts->m_verts[ts->m_verts.size() - 2].m_v.x ==  32767);
// 	assert(ts->m_verts[ts->m_verts.size() - 2].m_v.y == -32768);
// 	assert(ts->m_verts[ts->m_verts.size() - 1].m_v.x ==  32767);
// 	assert(ts->m_verts[ts->m_verts.size() - 1].m_v.y ==  32767);
}


bool edge_exists(const array<edge>& edgeset, const edge& e)
// Return true if there's an edge in edgeset coincident and in the
// same direction as e.
{
	for (int i = 0; i < edgeset.size(); i++) {
		if (edgeset[i].m_0 == e.m_0 && edgeset[i].m_1 == e.m_1) {
			return true;
		}
	}
	return false;
}


int count_coincident_edges(const tristate* ts, const array<edge>& edgeset, const edge& e)
// Count the number of edges that are coincident with e (in either
// direction).
{
	const vec2<sint16>& v0 = ts->m_verts[e.m_0].m_v;
	const vec2<sint16>& v1 = ts->m_verts[e.m_1].m_v;

	int count = 0;
	for (int i = 0; i < edgeset.size(); i++) {
		if ((ts->m_verts[edgeset[i].m_0].m_v == v0 && ts->m_verts[edgeset[i].m_1].m_v == v1)
		    || (ts->m_verts[edgeset[i].m_0].m_v == v1 && ts->m_verts[edgeset[i].m_1].m_v == v0)) {
			count++;
		}
	}
	return count;
}


// Return true if there is any vertex in tristate that touches the
// interior of the given triangle.  Verts coincident with the triangle
// verts are excluded.
bool any_vert_in_triangle(const array<poly_vert>& verts, int vi0, int vi1, int vi2)
{
	const vec2<sint16>& v0 = verts[vi0].m_v;
	const vec2<sint16>& v1 = verts[vi1].m_v;
	const vec2<sint16>& v2 = verts[vi2].m_v;

	for (int i = 0; i < verts.size(); i++) {
		if (i == vi0 || i == vi1 || i == vi2) {
			continue;
		}
		const vec2<sint16>& v = verts[i].m_v;
		if (v == v0 || v == v1 || v == v2) {
			// Coincident.
			continue;
		}

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
	}
	return false;
}


bool vertex_in_cone(const vec2<sint16>& vert, const vec2<sint16>& cone_v0, const vec2<sint16>& cone_v1, const vec2<sint16>& cone_v2)
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


int test_other_edge_for_apex_vert(const tristate* ts, const edge& e, const edge& oe, int other_vert)
// Return a new value for other_edge, if a vert on oe can make a valid apex for edge e.
{
	int ov = -1;
	if (oe.m_1 == e.m_0) {
		ov = oe.m_0;
	} else if (oe.m_0 == e.m_0) {
		ov = oe.m_1;
	}
	if (ov >= 0
	    && vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ov].m_v) < 0) {
		// Is this the closest vert?
		if (other_vert == -1
		    || vertex_in_cone(ts->m_verts[ov].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[e.m_0].m_v, ts->m_verts[other_vert].m_v)) {
			return ov;
		}
	}
	return other_vert;
}


int test_other_edge_for_apex_vert_inverted(const tristate* ts, const edge& e, const edge& oe, int other_vert)
// Return a new value for other_edge, if a vert on oe can make a valid apex for edge e.
{
	int ov = -1;
	if (oe.m_1 == e.m_1) {
		ov = oe.m_0;
	} else if (oe.m_0 == e.m_1) {
		ov = oe.m_1;
	}
	if (ov >= 0
	    && vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ov].m_v) < 0) {
		// Is this the closest vert?
		if (other_vert == -1
		    || vertex_in_cone(ts->m_verts[ov].m_v, ts->m_verts[other_vert].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[e.m_0].m_v)) {
			return ov;
		}
	}
	return other_vert;
}


int find_apex_vertex(tristate* ts, const edge& e)
// find a vertex v such that:
//
// * there's no existing edge (e.m_1, e.m_0) (thus indicating that the
// space to the right of e has not already been triangulated).
//
// * v is to the right of e
//
// * edges (e.m_0, v) and (v, e.m_1) don't intersect
// any input edges or generated edges
//
// * triangle (e.m_0, v, e.m_1) doesn't contain any verts
{
	if (ts->m_verts[e.m_0].m_v == ts->m_verts[e.m_1].m_v) {
		// zero-length edge -- not useful for triangulating; skip it.
		return -1;
	}

	// Make sure no (e.m_1, e.m_0).
	if (edge_exists(ts->m_created, edge(e.m_1, e.m_0))) {
		// The space to the right of e has already been filled...
		return -1;
	}

	// Find an edge incident on e.m_0, whose other vert is a valid
	// apex for edge e.
	int other_vert = -1;
	for (int i = 0; i < ts->m_input.size(); i++) {
		other_vert = test_other_edge_for_apex_vert(ts, e, ts->m_input[i], other_vert);
	}
	{for (int i = 0; i < ts->m_created.size(); i++) {
		other_vert = test_other_edge_for_apex_vert(ts, e, ts->m_created[i], other_vert);
	}}
	if (other_vert >= 0
	    && any_vert_in_triangle(ts->m_verts, e.m_0, other_vert, e.m_1) == false) {
		return other_vert;
	}

	// Repeat for edges incident on e.m_1.
	other_vert = -1;
	for (int i = 0; i < ts->m_input.size(); i++) {
		other_vert = test_other_edge_for_apex_vert_inverted(ts, e, ts->m_input[i], other_vert);
	}
	{for (int i = 0; i < ts->m_created.size(); i++) {
		other_vert = test_other_edge_for_apex_vert_inverted(ts, e, ts->m_created[i], other_vert);
	}}
	if (other_vert >= 0
	    && any_vert_in_triangle(ts->m_verts, e.m_0, other_vert, e.m_1) == false) {
		return other_vert;
	}

// 	// Check candidate verts.
// 	{for (int i = 0; i < ts->m_verts.size(); i++) {
// 		if (vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[i].m_v) < 0) {
// 			// i is to the right of e.
// 			if (any_edge_intersects(ts, ts->m_created, edge(e.m_0, i)) == false
// 			    && any_edge_intersects(ts, ts->m_input, edge(e.m_0, i)) == false
// 			    && any_edge_intersects(ts, ts->m_created, edge(i, e.m_1)) == false
// 			    && any_edge_intersects(ts, ts->m_input, edge(i, e.m_1)) == false) {
// 				if (any_vert_in_triangle(ts->m_verts, e.m_0, i, e.m_1) == false) {
// 					// We've found a good vert to attach to.
// 					return i;
// 				}
// 			}
// 		}
// 	}}

	// No good vert found.
	return -1;
}


void add_triangle(tristate* ts, const edge& e, int v, bool in_status)
// Add the triangle formed by e and v to our triangulation.
{
	// Add the edges of the triangle, (e.m_1, e.m_0), (e.m_0, v), (v, e.m_1) to m_created
	edge a(e.m_1, e.m_0);
	edge b(e.m_0, v);
	edge c(v, e.m_1);
	ts->m_created.push_back(a);
	ts->m_created.push_back(b);
	ts->m_created.push_back(c);

	if (in_status) {
		ts->m_results->push_back(ts->m_verts[e.m_0].m_v.x);
		ts->m_results->push_back(ts->m_verts[e.m_0].m_v.y);
		ts->m_results->push_back(ts->m_verts[v].m_v.x);
		ts->m_results->push_back(ts->m_verts[v].m_v.y);
		ts->m_results->push_back(ts->m_verts[e.m_1].m_v.x);
		ts->m_results->push_back(ts->m_verts[e.m_1].m_v.y);
	}

	// Add the edges (e.m_0, v) and (v, e.m_1) to m_active, to continue the flood fill
	ts->m_active.push_back(active_edge(b, in_status));
	ts->m_active.push_back(active_edge(c, in_status));
}


void fill_debug_out(tristate* ts)
{
	// Put the current active edges in the debug output.
	for (int i = ts->m_next_active; i < ts->m_active.size(); i++) {
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.x);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.y);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.x);
		ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.y);
	}

// 	// Put the current sweep loop in the debug output.
// 	for (int i = 0; i < ts->m_sweep.size() - 1; i++) {
// 		ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[i]].m_v.x);
// 		ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[i]].m_v.y);
// 		ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[i + 1]].m_v.x);
// 		ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[i + 1]].m_v.y);
// 	}
// 	ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[ts->m_sweep.size() - 1]].m_v.x);
// 	ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[ts->m_sweep.size() - 1]].m_v.y);
// 	ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[0]].m_v.x);
// 	ts->m_debug_edges->push_back(ts->m_verts[ts->m_sweep[0]].m_v.y);
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


bool clip_sweep_corner(tristate* ts, int index)
// Try clipping a corner of the sweep line.  Return true if we were
// able to.
{
	if (index >= ts->m_sweep.size()) {
		return false;
	}

	int i0 = index;
	int i1 = index + 1;
	int i2 = index + 2;
	if (i1 >= ts->m_sweep.size()) {
		i1 -= ts->m_sweep.size();
	}
	if (i2 >= ts->m_sweep.size()) {
		i2 -= ts->m_sweep.size();
	}

	int vi0 = ts->m_sweep[i0];
	int vi1 = ts->m_sweep[i1];
	int vi2 = ts->m_sweep[i2];

	if (vertex_left_test(ts->m_verts[vi0].m_v, ts->m_verts[vi1].m_v, ts->m_verts[vi2].m_v) > 0) {
		if (!any_vert_in_triangle(ts->m_verts, vi0, vi1, vi2)
		    // This check can be replaced by: "! any_edges_emanating_from_vi1_within_cone(vi0, vi1, vi2)"
		    && any_edge_intersects(ts, ts->m_input, edge(vi2, vi0)) == false) {
			// Clip corner.

			edge a(vi0, vi1);
			edge b(vi1, vi2);
			edge c(vi2, vi0);
			ts->m_created.push_back(a);
			ts->m_created.push_back(b);
			ts->m_created.push_back(c);

			// if (in_status) {
			ts->m_results->push_back(ts->m_verts[vi0].m_v.x);
			ts->m_results->push_back(ts->m_verts[vi0].m_v.y);
			ts->m_results->push_back(ts->m_verts[vi1].m_v.x);
			ts->m_results->push_back(ts->m_verts[vi1].m_v.y);
			ts->m_results->push_back(ts->m_verts[vi2].m_v.x);
			ts->m_results->push_back(ts->m_verts[vi2].m_v.y);
			// }

			// Adjust sweep line.
			ts->m_sweep.remove(i1);

			return true;
		}
	}

	return false;
}


bool split_sweep_edge(tristate* ts, int index)
{
	assert(ts->m_sweep.size() >= 2);
	assert(index < ts->m_sweep.size());

	int i0 = index;
	int i1 = index + 1;
	if (i1 >= ts->m_sweep.size()) {
		i1 -= ts->m_sweep.size();
	}
	int vi0 = ts->m_sweep[i0];
	int vi1 = ts->m_sweep[i1];

	assert(ts->m_swept[vi0]);
	assert(ts->m_swept[vi1]);

	// Find a vert that validly completes the triangle.

	// TODO: how to make this search faster?
	//
	// * would be good & ~cheap to follow edge(s) emanating from
	// vi0 or vi1, if present

	// * would be good to do a shallow search of verts close to
	// the edge, before doing any deep search

	// N
	// Check candidate verts.
	{for (int i = 0; i < ts->m_verts.size(); i++) {
		if (ts->m_swept[i]) {
			continue;
		}

		if (vertex_left_test(ts->m_verts[vi0].m_v, ts->m_verts[vi1].m_v, ts->m_verts[i].m_v) > 0) {
			// i is to the left of (vi0, vi1).
			if (any_edge_intersects(ts, ts->m_created, edge(vi0, i)) == false
			    && any_edge_intersects(ts, ts->m_input, edge(vi0, i)) == false
			    // TODO: replace next two w/ !any_edge_emanating_from_vi0_in_cone(i, vi0, vi1)?
			    && any_edge_intersects(ts, ts->m_created, edge(i, vi1)) == false
			    && any_edge_intersects(ts, ts->m_input, edge(i, vi1)) == false
			    && any_vert_in_triangle(ts->m_verts, vi0, vi1, i) == false) {
				// We've found a good vert to include in the sweepline.

				// Add triangle.
				edge a(vi0, vi1);
				edge b(vi1, i);
				edge c(i, vi0);
				ts->m_created.push_back(a);
				ts->m_created.push_back(b);
				ts->m_created.push_back(c);

				// if (in_status) {
				ts->m_results->push_back(ts->m_verts[vi0].m_v.x);
				ts->m_results->push_back(ts->m_verts[vi0].m_v.y);
				ts->m_results->push_back(ts->m_verts[vi1].m_v.x);
				ts->m_results->push_back(ts->m_verts[vi1].m_v.y);
				ts->m_results->push_back(ts->m_verts[i].m_v.x);
				ts->m_results->push_back(ts->m_verts[i].m_v.y);
				// }

				// Adjust sweep line.
				ts->m_sweep.insert(i1, i);
				ts->m_swept[i] = true;

				return true;
			}
		}
	}}

	return false;
}


bool find_sweep_probe(tristate* ts, int index)
// Look for a place in the sweep loop where we can extend a zero-area
// probe into virgin territory.
//
// Start looking around sweep[index], but check all possibilities.
{
	for (int i = 0, n = ts->m_sweep.size(); i < n; i++) {
		int i0 = index + i;
		if (i0 >= n) {
			i0 -= n;
		}
		assert(i0 >= 0);
		assert(i0 < n);

		int vi0 = ts->m_sweep[i0];

		// Does vert[vi0] have any edge that connects to a
		// non-swept vert?
		//
		// TODO use an index to make this much faster
		int vi = -1;
		for (int j = 0; j < ts->m_input.size(); j++) {
			if (ts->m_input[j].m_0 == vi0 && ts->m_swept[ts->m_input[j].m_1] == false) {
				vi = ts->m_input[j].m_1;
				break;
			} else if (ts->m_input[j].m_1 == vi0 && ts->m_swept[ts->m_input[j].m_0] == false) {
				vi = ts->m_input[j].m_0;
				break;
			}
		}
		if (vi >= 0) {
			// Found something.
			ts->m_sweep.insert(i0, vi);
			ts->m_sweep.insert(i0, vi0);
			ts->m_swept[vi] = true;
			return true;
		}
	}

	return false;
}


bool full_sweep_check(tristate* ts)
// Recovery fallback -- try all ears & edges in the sweep line, when
// our simple heuristic check didn't find a way to advance.
//
// Returns true if we advanced, false if not.  If we return false
// here, it means our algorithm is hung and can't advance.  This
// should only happen if the input has self-intersections.
{
	// Try clipping an ear off the sweep line.
	for (int i = 0; i < ts->m_sweep.size(); i++) {
		if (clip_sweep_corner(ts, i)) {
			return true;
		}
	}

// 	// Couldn't clip an ear; try splitting a sweep edge.
// 	{for (int i = 0; i < ts->m_sweep.size(); i++) {
// 		if (split_sweep_edge(ts, i)) {
// 			return true;
// 		}
// 	}}

	return false;
}


int find_ear_edge(const tristate* ts, const edge& e)
// Look through m_active to find an edge e1 such that:
//
// * e-e1 is a left turn, and there are no edges within the cone
//
// * e.m_1 is not degenerate -- i.e. there are unpaired edges in/out of e.m_1
//
// * triangle e-e1 doesn't contain any (reflex) verts
//
// Return the index of oe, or -1 if we don't find anything valid.
{
	assert(e.m_0 != e.m_1);  // zero-length edges should not exist

	// Find an edge incident on e.m_0, whose other vert is a valid
	// left turn for edge e.
	hash<int, int> in_out;
	int oe_i = -1;
	for (int i = 0; i < ts->m_active.size(); i++) {
		const edge& te = ts->m_active[i];  // te --> "temp edge"
		if (te.m_1 == e.m_1) {
			// in edge
			int total = 0;
			in_out.get(te.m_0, &total);
			total++;
			in_out.set(te.m_0, total);
		} else if (te.m_0 == e.m_1) {
			// out edge
			int total = 0;
			in_out.get(te.m_1, &total);
			total--;
			in_out.set(te.m_1, total);

			// Is this a valid out edge?
			if (vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[te.m_1].m_v) > 0) {
				// Is this the inside-most out edge so far?
				if (oe_i == -1
				    || vertex_in_cone(ts->m_verts[te.m_1].m_v,
						      ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ts->m_active[oe_i].m_1].m_v)) {
					oe_i = i;
				}
			}
		}
	}
	if (oe_i >= 0) {
		// Check degeneracy.
		bool non_degenerate_edge = false;
		bool edge_in_cone = false;
		for (hash<int, int>::iterator it = in_out.begin(); it != in_out.end(); ++it) {
			if (it->second != 0) {
				non_degenerate_edge = true;
			}
			if (it->first != ts->m_active[oe_i].m_1
			    && it->first != e.m_0
			    && vertex_in_cone(ts->m_verts[it->first].m_v,
					      ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[ts->m_active[oe_i].m_1].m_v)) {
				// Can't clip; there's another edge in the way.
				edge_in_cone = true;
				//break;
			}
		}
		if (non_degenerate_edge && !edge_in_cone && !any_vert_in_triangle(ts->m_verts, e.m_0, e.m_1, ts->m_active[oe_i].m_1)) {
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
			
			if (oe_i < i) {
				swap(&oe_i, &i);
			}
			// remove later index first!
			ts->m_active.remove(oe_i);
			ts->m_active.remove(i);
			ts->m_active.push_back(active_edge(i0, i2));
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


void triangulate_plane(tristate* ts)
// Make triangles.
{
	assert(ts->m_active.size() == 0);
	assert(ts->m_created.size() == 0);

	// The synthetic bounding box looks like this:
	//
	// v[1] *--* v[size-1]
	//      |  |
	// v[0] *--* v[size-2]

// 	// Create the initial sweep polyline.
// 	//
// 	// *--*
// 	// |
// 	// *--*
// 	ts->m_sweep.push_back(ts->m_verts.size() - 1);
// 	ts->m_sweep.push_back(1);
// 	ts->m_sweep.push_back(0);
// 	ts->m_sweep.push_back(ts->m_verts.size() - 2);

// 	ts->m_swept.resize(ts->m_verts.size());
// 	for (int i = 0, n = ts->m_swept.size(); i < n; i++) {
// 		ts->m_swept[i] = false;
// 	}
// 	ts->m_swept[ts->m_verts.size() - 1] = true;
// 	ts->m_swept[1] = true;
// 	ts->m_swept[0] = true;
// 	ts->m_swept[ts->m_verts.size() - 2] = true;

// 	// TODO: join the islands a la FIST.  For quick testing, do
// 	// something simple & cheesy -- connect the bounding box to
// 	// the leftmost input vert.
// 	ts->m_input.push_back(edge(0, 2));
// 	ts->m_input.push_back(edge(2, 0));

// 	// Pick a good seed edge.
// 	{for (int i = 0; i < ts->m_input.size(); i++) {
// 		// Use the leftmost non-degenerate edge.
// 		update_seed_edge(ts, ts->m_input[i]);
// 		assert(ts->m_active.size() == 1);
// 	}}

// 	// Use the bounding box as seed edges.
// 	ts->m_active.push_back(active_edge(ts->m_verts.size() - 2, 0));
// 	ts->m_active.push_back(active_edge(ts->m_verts.size() - 1, ts->m_verts.size() - 2));
// 	ts->m_active.push_back(active_edge(1, ts->m_verts.size() - 1));
// 	ts->m_active.push_back(active_edge(0, 1));

// 	// Flood-fill.  Spread out from current active edges, until
// 	// the shape is consumed.
// 	while (ts->m_next_active < ts->m_active.size()) {
// 		active_edge e = ts->m_active[ts->m_next_active];
// 		ts->m_next_active++;

// 		int v = find_apex_vertex(ts, e);
// 		if (v >= 0) {
// 			// New triangle IN status = (e.m_in) ^ (# of
// 			// edges in the input set, coincident with e)
// 			int edge_ct = count_coincident_edges(ts, ts->m_input, e);
// 			bool in_status = e.m_in;
// 			if (edge_ct & 1) {
// 				in_status = !in_status;
// 			}

// 			// Found one.  Add the triangle (e.m_0, v, e.m_1).
// 			add_triangle(ts, e, v, in_status);
// 		}

// 		// Debug dumping.
// 		if (check_debug_dump(ts)) {
// 			return;
// 		}
// 	}


	// Ear-clip, not based on loops.  (Reasoning: don't need to be
	// careful to do non-local analysis when constructing master
	// loop.)
	//
	// Copy all input edges to active list.
	ts->m_active.reserve(ts->m_input.size());
	for (int i = 0; i < ts->m_input.size(); i++) {
		const edge& e = ts->m_input[i];
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


// 	// Flood-fill by sweeping a polyline through the shape.
// 	//
// 	// When m_sweep reaches (size - 1, size - 2), or equivalently,
// 	// has only two verts, then we know we're done triangulating.
// 	int sweep_index = 0;
// 	for (;;) {
// 		//assert(ts->m_sweep[0] == ts->m_verts.size() - 1);
// 		// TODO: check this!
// 		if (ts->m_sweep.size() == 2) {
// 			//assert(ts->m_sweep[1] == ts->m_verts.size() - 2);
// 			// Termination condition -- we're done!
// 			return;
// 		}

// 		// heuristically pick an edge or a corner to advance
// 		// the sweepline.

// 		// Try the next few verts to see if they can be clipped.
// 		bool did_advance = false;
// 		for (int i = 0; i < 3; i++) {
// 			sweep_index++;
// 			if (sweep_index >= ts->m_sweep.size()) {
// 				sweep_index = 0;
// 			}

// 			if (clip_sweep_corner(ts, sweep_index)) {
// 				did_advance = true;
// 				// adjust sweep_index?
// 				break;
// 			}
// 		}

// 		if (!did_advance) {
// 			// Couldn't clip an ear; try extending the sweep loop via a probe.
// 			if (sweep_index >= ts->m_sweep.size()) {
// 				sweep_index = 0;
// 			}

// 			if (find_sweep_probe(ts, sweep_index)) {
// 				did_advance = true;
// 			}
// 		}

// 		if (!did_advance) {
// 			if (!full_sweep_check(ts)) {
// 				// Uh oh, we can't advance.
// 				// TODO log something
// 				fill_debug_out(ts);
// 				return;
// 			}
// 		}

// 		// Debug dumping.
// 		if (check_debug_dump(ts)) {
// 			return;
// 		}
// 	}
}


void eat_bad_triangles(tristate* ts)
// Remove triangles that aren't in the interior of the shape.
{
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
		eat_bad_triangles(&ts);
	}
}





/* triangulation notes

Nice lecture notes:
http://arachne.ics.uci.edu/~eppstein/junkyard/godfried.toussaint.html

Narkhede & Manocha's description/code of Seidel's alg:
http://www.cs.unc.edu/~dm/CODE/GEM/chapter.html



*/
