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
// Self-intersecting input is not good though; it can block our fill
// and make us terminate early.
//
// * can we get positive/negative fill rule?
//
// * can flood-filling be made fast?  (Issue is search for valid next
// vert; maybe have to do line-sweep approach to make it simple/fast?)
//
// * how does it fail when given bad input?  (i.e. self-intersection)
// -- do we have to pre-resolve self-intersections, or is it OK to
// just leave them in if they're minor?


#include "base/constrained_triangulate.h"
#include "base/vert_types.h"


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


// Triangulator state.
struct tristate {
	array<sint16>* m_results;
	array<poly_vert> m_verts;
	array<edge> m_input;
	array<active_edge> m_active;
	array<edge> m_created;

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


void init(tristate* ts, array<sint16>* results, int path_count, const array<sint16> paths[],
	  int debug_halt_step, array<sint16>* debug_edges)
// Pull the paths into *tristate.
{
	assert(results);
	ts->m_results = results;
	ts->m_debug_halt_step = debug_halt_step;
	ts->m_debug_edges = debug_edges;
	
	// Dump verts and edges into tristate.
	// TODO: sort, scrub dupes
	for (int i = 0; i < path_count; i++) {
		assert((paths[i].size() & 1) == 0);
		int first_vert_index = ts->m_verts.size();
		for (int j = 0; j < paths[i].size(); j += 2) {
			int vert_index = ts->m_verts.size();
			ts->m_verts.push_back(poly_vert(paths[i][j], paths[i][j + 1]));
			edge e(vert_index, vert_index + 1);
			ts->m_input.push_back(e);
		}
		ts->m_input.back().m_1 = first_vert_index;  // close the path
	}

	// TODO: find & fix edge intersections
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


int find_apex_vertex(tristate* ts, const edge& e)
// find a vertex v such that:
//
// * there's no existing edge (e.m_1, e.m_0) (thus
// indicating that the space to the right of e has
// already been triangulated).
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

	// Check candidate verts.
	{for (int i = 0; i < ts->m_verts.size(); i++) {
		if (vertex_left_test(ts->m_verts[e.m_0].m_v, ts->m_verts[e.m_1].m_v, ts->m_verts[i].m_v) < 0) {
			// i is to the right of e.
			if (any_edge_intersects(ts, ts->m_created, edge(e.m_0, i)) == false
			    && any_edge_intersects(ts, ts->m_input, edge(e.m_0, i)) == false
			    && any_edge_intersects(ts, ts->m_created, edge(i, e.m_1)) == false
			    && any_edge_intersects(ts, ts->m_input, edge(i, e.m_1)) == false) {
				if (any_vert_in_triangle(ts->m_verts, e.m_0, i, e.m_1) == false) {
					// We've found a good vert to attach to.
					return i;
				}
			}
		}
	}}

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


void triangulate_plane(tristate* ts)
// Make triangles.
{
	assert(ts->m_active.size() == 0);
	assert(ts->m_created.size() == 0);
	
	// Pick a good seed edge.
	{for (int i = 0; i < ts->m_input.size(); i++) {
		// Use the leftmost non-degenerate edge.
		update_seed_edge(ts, ts->m_input[i]);
		assert(ts->m_active.size() == 1);
	}}
	
	// Flood-fill.  Spread out from current active edges, until
	// the shape is consumed.
	while (ts->m_active.size()) {
		active_edge e = ts->m_active.back();
		ts->m_active.pop_back();

		int v = find_apex_vertex(ts, e);
		if (v >= 0) {
			// New triangle IN status = (e.m_in) ^ (# of
			// edges in the input set, coincident with e)
			int edge_ct = count_coincident_edges(ts, ts->m_input, e);
			bool in_status = e.m_in;
			if (edge_ct & 1) {
				in_status = !in_status;
			}

			// Found one.  Add the triangle (e.m_0, v, e.m_1).
			add_triangle(ts, e, v, in_status);
		}

		// Debug dumping.
		if (ts->m_debug_halt_step > 0) {
			ts->m_debug_halt_step--;
			if (ts->m_debug_halt_step == 0) {
				// Emit some debug info.

				// Put the current active edges in the debug output.
				for (int i = 0; i < ts->m_active.size(); i++) {
					ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.x);
					ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_0].m_v.y);
					ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.x);
					ts->m_debug_edges->push_back(ts->m_verts[ts->m_active[i].m_1].m_v.y);
				}

				return;
			}
		}
	}
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

