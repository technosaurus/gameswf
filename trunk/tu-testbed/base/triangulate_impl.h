// triangulate_impl.h	-- Thatcher Ulrich 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to triangulate arbitrary 2D polygonal regions.
//
// Use the basic robust ear-clipping algorithm from "FIST: Fast
// Industrial-Strength Triangulation of Polygons" by Martin Held.


#include "base/utility.h"
#include "base/triangulate.h"
#include "base/tu_random.h"



// Template this whole thing on coord_t, so sint16 and float versions
// can easily reuse the code.
//
// These templates are instantiated in triangulate_<type>.cpp files.
// They're in separate cpp files so the linker will discard code for
// unused types.


// convenience struct; this could use some public vec2 type, but often
// it's nicer for users if the external interface is smaller and more
// c-like, since they probably have their own vec2 that they prefer.
template<class coord_t>
struct vec2
{
	vec2() : x(0), y(0) {}
	vec2(coord_t _x, coord_t _y) : x(_x), y(_y) {}

	bool	operator==(const vec2<coord_t>& v) const
	{
		return x == v.x && y == v.y;
	}

//data:
	coord_t	x, y;
};


inline double	determinant_float(const vec2<float>& a, const vec2<float>& b, const vec2<float>& c)
{
	return (double(b.x) - double(a.x)) * (double(c.y) - double(a.y))
		- (double(b.y) - double(a.y)) * (double(c.x) - double(a.x));
}


inline sint64	determinant_sint32(const vec2<sint32>& a, const vec2<sint32>& b, const vec2<sint32>& c)
{
	return (sint64(b.x) - sint64(a.x)) * (sint64(c.y) - sint64(a.y))
		- (sint64(b.y) - sint64(a.y)) * (sint64(c.x) - sint64(a.x));
}


// Return true if c is to the left of the directed edge defined by
// a->b.
template<class coord_t>
inline bool	vertex_left_test(const vec2<coord_t>& a, const vec2<coord_t>& b, const vec2<coord_t>& c)
{
	compiler_assert(0);	// must specialize
}


template<>
inline bool	vertex_left_test(const vec2<float>& a, const vec2<float>& b, const vec2<float>& c)
// Specialize for vec2<float>
{
	return determinant_float(a, b, c) > 0;
}


template<>
inline bool	vertex_left_test(const vec2<sint32>& a, const vec2<sint32>& b, const vec2<sint32>& c)
// Specialize for vec2<sint32>
{
	return determinant_sint32(a, b, c) > 0;
}


template<class coord_t>
bool	vertex_in_triangle(const vec2<coord_t>& v, const vec2<coord_t>& a, const vec2<coord_t>& b, const vec2<coord_t>& c)
// Return true if v is inside or on the border of (a,b,c).
// (a,b,c) must be in ccw order!
{
	assert(vertex_left_test(b, a, c) == false);	// check ccw order

	// Inverted tests, so that our triangle includes the boundary.
	return
		vertex_left_test(b, a, v) == false
		&& vertex_left_test(c, b, v) == false
		&& vertex_left_test(a, c, v) == false;
}


template<class coord_t> struct poly;



inline int	remap_index_for_duped_verts(int index, int duped_v0, int duped_v1)
// Helper.  Return the new value of index, for the case of verts [duped_v0]
// and [duped_v1] being duplicated and subsequent verts being shifted up.
{
	assert(duped_v0 < duped_v1);
	if (index <= duped_v0)
	{
		// No shift.
		return index;
	}
	else if (index <= duped_v1)
	{
		// Shift up one.
		return index + 1;
	}
	else
	{
		// Shift up two.
		return index + 2;
	}
}


template<class coord_t>
struct poly_vert
{
	poly_vert() {}
	poly_vert(coord_t x, coord_t y, poly<coord_t>* owner, int my_index)
		:
		m_v(x, y),
		m_next(-1),
		m_prev(-1),
		m_poly_owner(owner),
		m_my_index(my_index)
	{
	}

	void	remap(const array<int>& remap_table)
	{
		m_my_index = remap_table[m_my_index];
		m_next = remap_table[m_next];
		m_prev = remap_table[m_prev];
	}

//data:
	vec2<coord_t>	m_v;
	int	m_my_index;	// my index into sorted_verts array
	int	m_next;
	int	m_prev;
	poly<coord_t>*	m_poly_owner;	 // needed?
};


template<class coord_t>
int	compare_vertices(const void* a, const void* b)
// For qsort.  Sort by x, then by y.
{
	const poly_vert<coord_t>* vert_a = (const poly_vert<coord_t>*) a;
	const poly_vert<coord_t>* vert_b = (const poly_vert<coord_t>*) b;

	if (vert_a->m_v.x < vert_b->m_v.x)
		return -1;
	else if (vert_a->m_v.x > vert_b->m_v.x)
		return 1;
	else
	{
		if (vert_a->m_v.y < vert_b->m_v.y)
			return -1;
		else if (vert_a->m_v.y > vert_b->m_v.y)
			return 1;
	}

	return 0;
}


template<class coord_t>
inline bool	edges_intersect_sub(const array<poly_vert<coord_t> >& sorted_verts, int e0v0, int e0v1, int e1v0, int e1v1)
// Return true if edge (e0v0,e0v1) intersects (e1v0,e1v1).
{
	// Need to specialize this on coord_t, in order to get it
	// correct and avoid overflow.
	COMPILER_ASSERT(0);
}


template<>
inline bool	edges_intersect_sub(const array<poly_vert<float> >& sorted_verts, int e0v0i, int e0v1i, int e1v0i, int e1v1i)
// Return true if edge (e0v0,e0v1) intersects (e1v0,e1v1).
//
// Specialized for float.
{
	// If e1v0,e1v1 are on opposite sides of e0, and e0v0,e0v1 are
	// on opposite sides of e1, then the segments cross.  These
	// are all determinant checks.

	// The main degenerate case we need to watch out for is if
	// both segments are zero-length.
	//
	// If only one is degenerate, our tests are still OK.

	const vec2<float>&	e0v0 = sorted_verts[e0v0i].m_v;
	const vec2<float>&	e0v1 = sorted_verts[e0v1i].m_v;
	const vec2<float>&	e1v0 = sorted_verts[e1v0i].m_v;
	const vec2<float>&	e1v1 = sorted_verts[e1v1i].m_v;

	// @@ need epsilons here, or are zeros OK?  I think the issue
	// is underflow in case of very small determinants.  Our
	// determinants are doubles, so I think we're good.
	if (e0v0.x == e0v1.x && e0v0.y == e0v1.y)
	{
		// e0 is zero length.
		if (e1v0.x == e1v1.x && e1v0.y == e1v1.y)
		{
			// Both edges are zero length.
			// They intersect only if they're coincident.
			return e0v0.x == e1v0.x && e0v0.y == e1v0.y;
		}
	}

	// See if e1 crosses line of e0.
	double	det10 = determinant_float(e0v0, e0v1, e1v0);
	double	det11 = determinant_float(e0v0, e0v1, e1v1);

	if (det10 * det11 > 0)
	{
		// e1 doesn't cross the line of e0.
		return false;
	}

	// See if e0 crosses line of e1.
	double	det00 = determinant_float(e1v0, e1v1, e0v0);
	double	det01 = determinant_float(e1v0, e1v1, e0v1);

	if (det00 * det01 > 0)
	{
		// e0 doesn't cross the line of e1.
		return false;
	}

	// They both cross each other; the segments intersect.
	return true;
}


template<>
inline bool	edges_intersect_sub(const array<poly_vert<sint32> >& sorted_verts, int e0v0i, int e0v1i, int e1v0i, int e1v1i)
// Return true if edge (e0v0,e0v1) intersects (e1v0,e1v1).
//
// Specialized for sint32
{
	// If e1v0,e1v1 are on opposite sides of e0, and e0v0,e0v1 are
	// on opposite sides of e1, then the segments cross.  These
	// are all determinant checks.

	// The main degenerate case we need to watch out for is if
	// both segments are zero-length.
	//
	// If only one is degenerate, our tests are still OK.

	const vec2<sint32>&	e0v0 = sorted_verts[e0v0i].m_v;
	const vec2<sint32>&	e0v1 = sorted_verts[e0v1i].m_v;
	const vec2<sint32>&	e1v0 = sorted_verts[e1v0i].m_v;
	const vec2<sint32>&	e1v1 = sorted_verts[e1v1i].m_v;

	if (e0v0.x == e0v1.x && e0v0.y == e0v1.y)
	{
		// e0 is zero length.
		if (e1v0.x == e1v1.x && e1v0.y == e1v1.y)
		{
			// Both edges are zero length.
			// They intersect only if they're coincident.
			return e0v0.x == e1v0.x && e0v0.y == e1v0.y;
		}
	}

	// See if e1 crosses line of e0.
	sint64	det10 = determinant_sint32(e0v0, e0v1, e1v0);
	sint64	det11 = determinant_sint32(e0v0, e0v1, e1v1);

	if (det10 * det11 > 0)
	{
		// e1 doesn't cross the line of e0.
		return false;
	}

	// See if e0 crosses line of e1.
	sint64	det00 = determinant_sint32(e1v0, e1v1, e0v0);
	sint64	det01 = determinant_sint32(e1v0, e1v1, e0v1);

	if (det00 * det01 > 0)
	{
		// e0 doesn't cross the line of e1.
		return false;
	}

	// They both cross each other; the segments intersect.
	return true;
}


template<class coord_t>
bool	edges_intersect(const array<poly_vert<coord_t> >& sorted_verts, int e0v0, int e0v1, int e1v0, int e1v1)
// Return true if edge (e0v0,e0v1) intersects (e1v0,e1v1).
{
	// Deal with special case: edges that share exactly one vert.
	// We treat these as no intersection, even though technically
	// they share one point.
	//
	// We're not just comparing indices, because duped verts (for
	// bridges) might have different indices.
	bool	coincident[2][2];
	coincident[0][0] = (sorted_verts[e0v0].m_v == sorted_verts[e1v0].m_v);
	coincident[0][1] = (sorted_verts[e0v0].m_v == sorted_verts[e1v1].m_v);
	coincident[1][0] = (sorted_verts[e0v1].m_v == sorted_verts[e1v0].m_v);
	coincident[1][1] = (sorted_verts[e0v1].m_v == sorted_verts[e1v1].m_v);
	if (coincident[0][0] && !coincident[1][1]) return false;
	if (coincident[1][0] && !coincident[0][1]) return false;
	if (coincident[0][1] && !coincident[1][0]) return false;
	if (coincident[1][1] && !coincident[0][0]) return false;

	// Both verts identical: early out.
	if (coincident[0][0] && coincident[1][1]) return true;
	if (coincident[1][0] && coincident[0][1]) return true;

	// Check for intersection.
	return edges_intersect_sub(sorted_verts, e0v0, e0v1, e1v0, e1v1);
}



template<class coord_t>
bool	is_convex_vert(const array<poly_vert<coord_t> >& sorted_verts, int vi)
// Return true if vert vi is convex.
{
	const poly_vert<coord_t>*	pvi = &(sorted_verts[vi]);
	const poly_vert<coord_t>*	pv_prev = &(sorted_verts[pvi->m_prev]);
	const poly_vert<coord_t>*	pv_next = &(sorted_verts[pvi->m_next]);

	return vertex_left_test<coord_t>(pv_prev->m_v, pvi->m_v, pv_next->m_v);
}


template<class coord_t>
struct poly
{
	typedef poly_vert<coord_t> vert_t;

	poly()
		:
		m_loop(-1),
		m_leftmost_vert(-1),
		m_vertex_count(0)
	{
	}

	void	append_vert(array<vert_t>* sorted_verts, int vert_index);
	void	remap(const array<int>& remap_table);

	void	remap_for_duped_verts(int v0, int v1);

	int	find_valid_bridge_vert(const array<poly_vert<coord_t> >& sorted_verts, int v1);

	void	build_ear_list(array<int>* ear_list, const array<vert_t>& sorted_verts);

	void	emit_and_remove_ear(array<coord_t>* result, array<vert_t>* sorted_verts, int v0, int v1, int v2);
	bool	any_edge_intersection(const array<vert_t>& sorted_verts, int v1, int v2);

	bool	triangle_contains_reflex_vertex(const array<vert_t>& sorted_verts, int v0, int v1, int v2);
	bool	vert_in_cone(const array<vert_t>& sorted_verts, int vert, int cone_v0, int cone_v1, int cone_v2);

	bool	is_valid(const array<vert_t>& sorted_verts) const;

	void	invalidate(const array<vert_t>& sorted_verts);

//data:
	int	m_loop;	// index of first vert
	int	m_leftmost_vert;
	int	m_vertex_count;

	// edge search_index;
};


template<class coord_t>
bool	poly<coord_t>::is_valid(const array<vert_t>& sorted_verts) const
// Assert validity.
{
#ifndef NDEBUG

	if (m_loop == -1 && m_leftmost_vert == -1 && m_vertex_count == 0)
	{
		// Empty poly.
		return true;
	}

	assert(m_leftmost_vert == -1 || sorted_verts[m_leftmost_vert].m_poly_owner == this);

	// Check vert count.
	int	first_vert = m_loop;
	int	vi = first_vert;
	int	vert_count = 0;
	bool	found_leftmost = false;
	do
	{
		// Check ownership.
		assert(sorted_verts[vi].m_poly_owner == this);

		// Check leftmost vert.
		assert(m_leftmost_vert == -1
		       || compare_vertices<coord_t>(
			       (const void*) &sorted_verts[m_leftmost_vert],
			       (const void*) &sorted_verts[vi]) <= 0);

		// Check link integrity.
		int	v_next = sorted_verts[vi].m_next;
		assert(sorted_verts[v_next].m_prev == vi);

		if (vi == m_leftmost_vert)
		{
			found_leftmost = true;
		}

		vert_count++;
		vi = sorted_verts[vi].m_next;
	}
	while (vi != first_vert);

	assert(vert_count == m_vertex_count);
	assert(found_leftmost || m_leftmost_vert == -1);

	// Might be nice to check that all verts with (m_poly_owner ==
	// this) are in our loop.

#endif // not NDEBUG

	return true;
}


template<class coord_t>
void	poly<coord_t>::invalidate(const array<vert_t>& sorted_verts)
// Mark as invalid/empty.  Do this after linking into another poly,
// for safety/debugging.
{
	assert(m_loop == -1 || sorted_verts[m_loop].m_poly_owner != this);	// make sure our verts have been stolen already.

	m_loop = -1;
	m_leftmost_vert = -1;
	m_vertex_count = 0;

	assert(is_valid(sorted_verts));
}


template<class coord_t>
int	compare_polys_by_leftmost_vert(const void* a, const void* b)
{
	const poly<coord_t>*	poly_a = * (const poly<coord_t>**) a;
	const poly<coord_t>*	poly_b = * (const poly<coord_t>**) b;

	// Vert indices are sorted, so we just compare the indices,
	// not the actual vert coords.
	if (poly_a->m_leftmost_vert < poly_b->m_leftmost_vert)
	{
		return -1;
	}
	else
	{
		// polys are not allowed to share verts, so the
		// leftmost vert must be different!
		assert(poly_a->m_leftmost_vert > poly_b->m_leftmost_vert);

		return 1;
	}
}


template<class coord_t>
void	poly<coord_t>::append_vert(array<vert_t>* sorted_verts, int vert_index)
// Link the specified vert into our loop.
{
	assert(vert_index >= 0 && vert_index < sorted_verts->size());
	assert(is_valid(*sorted_verts));

	m_vertex_count++;

	if (m_loop == -1)
	{
		// First vert.
		assert(m_vertex_count == 1);
		m_loop = vert_index;
		poly_vert<coord_t>*	v = &(*sorted_verts)[vert_index];
		v->m_next = vert_index;
		v->m_prev = vert_index;
		v->m_poly_owner = this;

		m_leftmost_vert = vert_index;
	}
	else
	{
		// We have a loop.  Link the new vert in, behind the
		// first vert.
		poly_vert<coord_t>*	pv0 = &(*sorted_verts)[m_loop];
		poly_vert<coord_t>*	pv = &(*sorted_verts)[vert_index];
		pv->m_next = m_loop;
		pv->m_prev = pv0->m_prev;
		(*sorted_verts)[pv0->m_prev].m_next = vert_index;
		pv0->m_prev = vert_index;

		// Update m_leftmost_vert
		poly_vert<coord_t>*	pvl = &(*sorted_verts)[m_leftmost_vert];
		if (compare_vertices<coord_t>(pv, pvl) < 0)
		{
			// v is to the left of vl; it's the new leftmost vert
			m_leftmost_vert = vert_index;
		}
	}

	assert(is_valid(*sorted_verts));
}


template<class coord_t>
int	poly<coord_t>::find_valid_bridge_vert(const array<vert_t>& sorted_verts, int v1)
// Find a vert v, in this poly, such that the edge (v,v1) doesn't
// intersect any edges in this poly.
{
	assert(is_valid(sorted_verts));

	const poly_vert<coord_t>*	pv1 = &(sorted_verts[v1]);

	int	first_vert = m_loop;
	int	vi = first_vert;
	int	vert_count = 0;
	do
	{
		const poly_vert<coord_t>*	pvi = &sorted_verts[vi];

		if (compare_vertices<coord_t>((void*) pvi, (void*) pv1) <= 0)
		{
			if (any_edge_intersection(sorted_verts, v1, vi) == false)
			{
				return vi;
			}
		}
		
		vi = sorted_verts[vi].m_next;
	}
	while (vi != first_vert);

	// Ugh!  No valid bridge vert.  Shouldn't happen with valid
	// data.  For invalid data, just pick something and live with
	// the intersection.
	assert(0);
	return m_leftmost_vert;
}


template<class coord_t>
void	poly<coord_t>::remap(const array<int>& remap_table)
{
	assert(m_loop > -1);
	assert(m_leftmost_vert > -1);

	m_loop = remap_table[m_loop];
	m_leftmost_vert = remap_table[m_leftmost_vert];
}


template<class coord_t>
void	poly<coord_t>::remap_for_duped_verts(int v0, int v1)
// Remap for the case of v0 and v1 being duplicated, and subsequent
// verts being shifted up.
{
	assert(m_loop > -1);
	assert(m_leftmost_vert > -1);

	m_loop = remap_index_for_duped_verts(m_loop, v0, v1);
	m_leftmost_vert = remap_index_for_duped_verts(m_leftmost_vert, v0, v1);
}


template<class coord_t>
void	poly<coord_t>::build_ear_list(array<int>* ear_list, const array<vert_t>& sorted_verts)
// Fill in *ear_list with a list of ears that can be clipped.
{
	assert(is_valid(sorted_verts));
	assert(ear_list);
	assert(ear_list->size() == 0);

	int	first_vert = m_loop;
	int	vi = first_vert;
	int	vert_count = 0;
	do
	{
		const poly_vert<coord_t>*	pvi = &(sorted_verts[vi]);
		const poly_vert<coord_t>*	pv_prev = &(sorted_verts[pvi->m_prev]);
		const poly_vert<coord_t>*	pv_next = &(sorted_verts[pvi->m_next]);

		// classification of ear, CE2 from FIST paper:
		//
		// v[i-1],v[i],v[i+1] of P form an ear of P iff
		// 
		// 1. v[i] is a convex vertex
		//
		// 2. the interior plus boundary of triangle
		// v[i-1],v[i],v[i+1] does not contain any reflex vertex of P
		// (except v[i-1] or v[i+1])
		//
		// 3. v[i-1] is in the cone(v[i],v[i+1],v[i+2]) and v[i+1] is
		// in the cone(v[i-2],v[i-1],v[i]) (not strictly necessary,
		// but used for efficiency and robustness)

		if (vertex_left_test<coord_t>(pv_prev->m_v, pvi->m_v, pv_next->m_v))
		{
			if (! triangle_contains_reflex_vertex(sorted_verts, pvi->m_prev, vi, pvi->m_next))
			{
				if (vert_in_cone(sorted_verts, pvi->m_prev, vi, pvi->m_next, pv_next->m_next)
				    && vert_in_cone(sorted_verts, pvi->m_next, pv_prev->m_prev, pvi->m_prev, vi))
				{
					// Valid ear.
					ear_list->push_back(vi);
				}
			}
		}
		
		vi = pvi->m_next;
	}
	while (vi != first_vert);
}


template<class coord_t>
void	poly<coord_t>::emit_and_remove_ear(
	array<coord_t>* result,
	array<vert_t>* sorted_verts,
	int v0,
	int v1,
	int v2)
// Push the ear triangle into the output; remove the triangle
// (i.e. vertex v1) from this poly.
{
	assert(is_valid(*sorted_verts));
	assert(m_vertex_count >= 3);

	poly_vert<coord_t>*	pv0 = &(*sorted_verts)[v0];
	poly_vert<coord_t>*	pv1 = &(*sorted_verts)[v1];
	poly_vert<coord_t>*	pv2 = &(*sorted_verts)[v2];

	if (m_loop == v1)
	{
		// Change m_loop, since we're about to lose it.
		m_loop = v0;
	}

	// emit the vertex list for the triangle.
	result->push_back(pv0->m_v.x);
	result->push_back(pv0->m_v.y);
	result->push_back(pv1->m_v.x);
	result->push_back(pv1->m_v.y);
	result->push_back(pv2->m_v.x);
	result->push_back(pv2->m_v.y);

	// Unlink v1.

	assert(pv0->m_poly_owner == this);
	assert(pv1->m_poly_owner == this);
	assert(pv2->m_poly_owner == this);

	pv0->m_next = v2;
	pv2->m_prev = v0;

	pv1->m_next = -1;
	pv1->m_prev = -1;
	pv1->m_poly_owner = NULL;

	// We lost v1.
	m_vertex_count--;

	if (m_leftmost_vert == v1)
	{
		// We shouldn't actually need m_leftmost_vert during
		// this phase of the algo, so kill it!
		m_leftmost_vert = -1;
	}

	assert(is_valid(*sorted_verts));
}


template<class coord_t>
bool	poly<coord_t>::any_edge_intersection(const array<vert_t>& sorted_verts, int v1, int v2)
// Return true if edge (v1,v2) intersects any edge in our poly.
{
	// @@ TODO implement spatial search structure to accelerate this!

	// For now, brute force O(N) :^o
	assert(is_valid(sorted_verts));

	int	first_vert = m_loop;
	int	vi = first_vert;
	do
	{
		int	v_next = sorted_verts[vi].m_next;

		if (edges_intersect(sorted_verts, vi, v_next, v1, v2))
		{
			return true;
		}
		
		vi = v_next;
	}
	while (vi != first_vert);

	return false;
}


template<class coord_t>
bool	poly<coord_t>::triangle_contains_reflex_vertex(const array<vert_t>& sorted_verts, int v0, int v1, int v2)
// Return true if any of this poly's reflex verts are inside the
// specified triangle.
{
	// TODO an accel structure would help here, for very large
	// polys.  Could use the edge spatial index to find verts
	// within the triangle bound.

	assert(is_valid(sorted_verts));

	// Find index bounds for the triangle, for culling.
	int	min_index = v0;
	int	max_index = v0;
	if (v1 < min_index)
	{
		min_index = v1;
	}
	if (v1 > max_index)
	{
		max_index = v1;
	}
	if (v2 < min_index)
	{
		min_index = v2;
	}
	if (v2 > max_index)
	{
		max_index = v2;
	}
	assert(min_index < max_index);
	assert(min_index <= v0 && min_index <= v1 && min_index <= v2);
	assert(max_index >= v0 && max_index >= v1 && max_index >= v2);

	int	first_vert = m_loop;
	int	vi = first_vert;
	do
	{
		int	v_next = sorted_verts[vi].m_next;

		if (vi > min_index && vi < max_index
		    && vi != v0 && vi != v1 && vi != v2)
		{
			int	v_prev = sorted_verts[vi].m_prev;

			// @@ cache this in vert?  Must refresh when
			// ear is clipped, polys are joined, etc.
			bool	reflex = vertex_left_test(
				sorted_verts[v_next].m_v,
				sorted_verts[vi].m_v,
				sorted_verts[v_prev].m_v);

			if (reflex
			    && vertex_in_triangle(
				    sorted_verts[vi].m_v, sorted_verts[v0].m_v, sorted_verts[v1].m_v, sorted_verts[v2].m_v))
			{
				// Found one.
				return true;
			}
		}
		
		vi = v_next;
	}
	while (vi != first_vert);

	// Didn't find any qualifying verts.
	return false;
}


template<class coord_t>
bool	poly<coord_t>::vert_in_cone(const array<vert_t>& sorted_verts, int vert, int cone_v0, int cone_v1, int cone_v2)
// Returns true if vert is within the cone defined by [v0,v1,v2].
//
//  (out)  v0
//        /
//    v1 <   (in)
//        \
//         v2
{
	bool	acute_cone = vertex_left_test(sorted_verts[cone_v0].m_v, sorted_verts[cone_v1].m_v, sorted_verts[cone_v2].m_v);

	if (acute_cone)
	{
		// Inverted tests, so that our cone includes the boundary.
		bool	left_of_01 = 
			vertex_left_test(sorted_verts[cone_v1].m_v, sorted_verts[cone_v0].m_v, sorted_verts[vert].m_v) == false;
		bool	left_of_12 =
			vertex_left_test(sorted_verts[cone_v2].m_v, sorted_verts[cone_v1].m_v, sorted_verts[vert].m_v) == false;

		return left_of_01 && left_of_12;
	}
	else
	{
		// Obtuse cone.  Vert is outside cone if it's to the right of both edges.
		bool	right_of_01 =
			vertex_left_test(sorted_verts[cone_v1].m_v, sorted_verts[cone_v0].m_v, sorted_verts[vert].m_v);
		bool	right_of_12 =
			vertex_left_test(sorted_verts[cone_v2].m_v, sorted_verts[cone_v1].m_v, sorted_verts[vert].m_v);

		return right_of_01 == false || right_of_12 == false;
	}
}


//
// poly_env
//


template<class coord_t>
struct poly_env
// Struct that holds the state of a triangulation.
{
	array<poly_vert<coord_t> >	m_sorted_verts;
	array<poly<coord_t>*>	m_polys;

	void	init(int path_count, const array<coord_t> paths[]);
	void	join_paths_into_one_poly();

	~poly_env()
	{
		// delete the polys that got new'd during init()
		for (int i = 0, n = m_polys.size(); i < n; i++)
		{
			delete m_polys[i];
		}
	}

private:
	// Internal helpers.
	void	join_paths_with_bridge(int vert_on_main_poly, int vert_on_sub_poly);
	void	dupe_two_verts(int v0, int v1);
};


template<class coord_t>
void	poly_env<coord_t>::init(int path_count, const array<coord_t> paths[])
// Initialize our state, from the given set of paths.  Sort vertices
// and component polys.
{
	// Only call this on a fresh poly_env
	assert(m_sorted_verts.size() == 0);
	assert(m_polys.size() == 0);

	// Count total verts.
	int	vert_count = 0;
	for (int i = 0; i < path_count; i++)
	{
		vert_count += paths[i].size();
	}

	// Collect the input verts and create polys for the input paths.
	m_sorted_verts.reserve(vert_count + (path_count - 1) * 2);	// verts, plus two duped verts for each path, for bridges
	m_polys.reserve(path_count);

	for (int i = 0; i < path_count; i++)
	{
		// Create a poly for this path.
		const array<coord_t>&	path = (paths[i]);

		if (path.size() < 3)
		{
			// Degenerate path, ignore it.
			continue;
		}

		poly<coord_t>*	p = new poly<coord_t>;
		m_polys.push_back(p);

		// Add this path's verts to our list.
		assert((path.size() & 1) == 0);
		for (int j = 0, n = path.size(); j < n; j += 2)	// vertex coords come in pairs.
		{
			int	vert_index = m_sorted_verts.size();

			poly_vert<coord_t>	vert(path[j], path[j+1], p, vert_index);
			m_sorted_verts.push_back(vert);

			p->append_vert(&m_sorted_verts, vert_index);
		}
	}

	// Sort the vertices.
	qsort(&m_sorted_verts[0], m_sorted_verts.size(), sizeof(m_sorted_verts[0]), compare_vertices<coord_t>);
	assert(m_sorted_verts.size() <= 1
	       || compare_vertices<coord_t>((void*) &m_sorted_verts[0], (void*) &m_sorted_verts[1]) == -1);	// check order

	// Remap the vertex indices, so that the polys and the
	// sorted_verts have the correct, sorted, indices.  We can
	// then use vert indices to judge the left/right relationship
	// of two verts.
	array<int>	vert_remap;	// vert_remap[i] == new index of original vert[i]
	vert_remap.resize(m_sorted_verts.size());
	for (int i = 0, n = m_sorted_verts.size(); i < n; i++)
	{
		int	new_index = i;
		int	original_index = m_sorted_verts[new_index].m_my_index;
		vert_remap[original_index] = new_index;
	}
	{for (int i = 0, n = m_sorted_verts.size(); i < n; i++)
	{
		m_sorted_verts[i].remap(vert_remap);
	}}
	{for (int i = 0, n = m_polys.size(); i < n; i++)
	{
		m_polys[i]->remap(vert_remap);
		assert(m_polys[i]->is_valid(m_sorted_verts));
	}}
}


template<class coord_t>
void	poly_env<coord_t>::join_paths_into_one_poly()
// Use zero-area bridges to connect separate polys & islands into one
// big continuous poly.
{
	// Connect separate paths with bridge edges, into one big path.
	//
	// Bridges are zero-area regions that connect a vert on each
	// of two paths.
	if (m_polys.size() > 1)
	{
		// Sort polys in order of each poly's leftmost vert.
		qsort(&m_polys[0], m_polys.size(), sizeof(m_polys[0]), compare_polys_by_leftmost_vert<coord_t>);
		assert(m_polys.size() <= 1
		       || compare_polys_by_leftmost_vert<coord_t>((void*) &m_polys[0], (void*) &m_polys[1]) == -1);

		// assume that the enclosing boundary is the leftmost
		// path; this is true if the regions are valid and
		// don't intersect.
		//
		// @@ are we screwed in case the leftmost path isn't
		// actually the enclosing path (i.e. the data is
		// bugged)?  Or does it not really matter...
		poly<coord_t>*	full_poly = m_polys[0];

		// Iterate from left to right
		while (m_polys.size() > 1)
		{
			int	v1 = m_polys[1]->m_leftmost_vert;
			
			//     find v2 in full_poly, such that:
			//       v2 is to the left of v1,
			//       and v1-v2 seg doesn't intersect any other edges

			// @@ we should try verts closer to v1 first; they're more likely
			// to be valid
			//     // (note that since v1 is next-most-leftmost, v1-v2 can't
			//     // hit anything in p, or any paths further down the list,
			//     // it can only hit edges in full_poly) (need to think
			//     // about equality cases)
			//
			int	v2 = full_poly->find_valid_bridge_vert(m_sorted_verts, v1);

			//     once we've found v1 & v2, we use it to make a bridge,
			//     inserting p into full_poly
			//
			assert(m_sorted_verts[v2].m_poly_owner == m_polys[0]);
			assert(m_sorted_verts[v1].m_poly_owner == m_polys[1]);
			join_paths_with_bridge(v2, v1);

			// Drop the joined poly.
			delete m_polys[1];
			m_polys.remove(1);
		}

		//   that we can run ear-clipping on.
	}

	assert(m_polys.size() == 1);
	// assert(all verts in m_sorted_verts have m_polys[0] as their owner);
}


// TODO
template<class coord_t>
void	poly_env<coord_t>::join_paths_with_bridge(int vert_on_main_poly, int vert_on_sub_poly)
{
	assert(vert_on_main_poly != vert_on_sub_poly);

	dupe_two_verts(vert_on_main_poly, vert_on_sub_poly);

	// Fixup the old indices to account for the new dupes.
	if (vert_on_sub_poly < vert_on_main_poly)
	{
		vert_on_main_poly++;
	}
	else
	{
		vert_on_sub_poly++;
	}

	poly_vert<coord_t>*	pv_main = &m_sorted_verts[vert_on_main_poly];
	poly_vert<coord_t>*	pv_sub = &m_sorted_verts[vert_on_sub_poly];
	poly_vert<coord_t>*	pv_main2 = &m_sorted_verts[vert_on_main_poly + 1];
	poly_vert<coord_t>*	pv_sub2 = &m_sorted_verts[vert_on_sub_poly + 1];

	// Link the loops together.
	pv_main2->m_next = pv_main->m_next;
	pv_main2->m_prev = vert_on_sub_poly + 1;	// (pv_sub2)
	m_sorted_verts[pv_main2->m_next].m_prev = pv_main2->m_my_index;

	pv_sub2->m_prev = pv_sub->m_prev;
	pv_sub2->m_next = vert_on_main_poly + 1;	// (pv_main2)
	m_sorted_verts[pv_sub2->m_prev].m_next = pv_sub2->m_my_index;

	pv_main->m_next = vert_on_sub_poly;		// (pv_sub)
	pv_sub->m_prev = vert_on_main_poly;		// (pv_main)

	// Fixup sub poly so it's now properly a part of the main poly.
	poly<coord_t>*	main_poly = pv_main->m_poly_owner;
	poly<coord_t>*	sub_poly = pv_sub->m_poly_owner;
	poly_vert<coord_t>*	v = pv_sub;
	do
	{
		v->m_poly_owner = main_poly;

		// Update leftmost vert.
		if (v->m_my_index < v->m_poly_owner->m_leftmost_vert)
		{
			v->m_poly_owner->m_leftmost_vert = v->m_my_index;
		}

		v = &m_sorted_verts[v->m_next];
	}
	while (v != pv_main2);

	main_poly->m_vertex_count += 2 + sub_poly->m_vertex_count;	// 2 for the duped verts, plus all the sub_poly's verts.

	sub_poly->invalidate(m_sorted_verts);

	assert(pv_main->m_poly_owner->is_valid(m_sorted_verts));
}


template<class coord_t>
void	poly_env<coord_t>::dupe_two_verts(int v0, int v1)
// Duplicate the two indexed verts, remapping polys & verts as necessary.
{
	// Order the verts.
	if (v0 > v1)
	{
		swap(&v0, &v1);
	}
	assert(v0 < v1);

	// Duplicate verts.
	poly_vert<coord_t>	v0_copy = m_sorted_verts[v0];
	poly_vert<coord_t>	v1_copy = m_sorted_verts[v1];

	// Insert v1 first, so v0 doesn't get moved.
	m_sorted_verts.insert(v1 + 1, v1_copy);
	m_sorted_verts.insert(v0 + 1, v0_copy);

	// Remap the indices within the verts.
	for (int i = 0, n = m_sorted_verts.size(); i < n; i++)
	{
		m_sorted_verts[i].m_my_index = i;
		m_sorted_verts[i].m_next = remap_index_for_duped_verts(m_sorted_verts[i].m_next, v0, v1);
		m_sorted_verts[i].m_prev = remap_index_for_duped_verts(m_sorted_verts[i].m_prev, v0, v1);
	}

	// Remap the polys.
	{for (int i = 0, n = m_polys.size(); i < n; i++)
	{
		m_polys[i]->remap_for_duped_verts(v0, v1);

		assert(m_polys[i]->is_valid(m_sorted_verts));
	}}
}


//
// Helpers.
//


template<class coord_t>
static void	recovery_process(
	array<poly<coord_t>*>* polys,	// polys waiting to be processed
	poly<coord_t>* P,	// current poly
	array<int>* Q,	// ears
	const array<poly_vert<coord_t> >& sorted_verts);


template<class coord_t>
static void compute_triangulation(
	array<coord_t>* result,
	int path_count,
	const array<coord_t> paths[])
// Compute triangulation.
{
	if (path_count <= 0)
	{
		// Empty paths --> no triangles to emit.
		return;
	}

	poly_env<coord_t>	penv;

	penv.init(path_count, paths);

	penv.join_paths_into_one_poly();

// Debugging only: just emit our joined poly, without triangulating.
//#define EMIT_JOINED_POLY
#ifdef EMIT_JOINED_POLY
	{
		int	first_vert = penv.m_polys[0]->m_loop;
		int	vi = first_vert;
		do
		{
			result->push_back(penv.m_sorted_verts[vi].m_v.x);
			result->push_back(penv.m_sorted_verts[vi].m_v.y);
			vi = penv.m_sorted_verts[vi].m_next;
		}
		while (vi != first_vert);

		// Loop back to beginning, and pad to a multiple of 3 coords.
		do
		{
			result->push_back(penv.m_sorted_verts[vi].m_v.x);
			result->push_back(penv.m_sorted_verts[vi].m_v.y);
		}
		while (result->size() % 6);
	}

	return;
#endif // EMIT_JOINED_POLY

	// classification of ear, CE2 from FIST paper:
	//
	// v[i-1],v[i],v[i+1] of P form an ear of P iff
	// 
	// 1. v[i] is a convex vertex
	//
	// 2. the interior plus boundary of triangle
	// v[i-1],v[i],v[i+1] does not contain any reflex vertex of P
	// (except v[i-1] or v[i+1])
	//
	// 3. v[i-1] is in the cone(v[i],v[i+1],v[i+2]) and v[i+1] is
	// in the cone(v[i-2],v[i-1],v[i]) (not strictly necessary,
	// but used for efficiency and robustness)

	// ear-clip, adapted from FIST paper:
	//   
	//   list<poly> L;
	//   L.insert(full_poly)
	//   while L not empty:
	//     P = L.pop()
	//     Q = priority queue of ears of P
	//     while P.vert_count > 3 do:
	//       if Q not empty:
	//         e = Q.pop
	//         emit e
	//         update P by deleting e
	//       else if an ear was clipped in previous pass then:
	//         Q = priority queue of ears of P (i.e. reexamine P)
	//       else
	//         // we're stuck
	//         recovery_process()	// do something drastic to make the next move
	//     emit last 3 verts of P as the final triangle

	while (penv.m_polys.size())
	{
		poly<coord_t>*	P = penv.m_polys.back();
		penv.m_polys.pop_back();

		array<int>	Q;	// Q is the ear list	
		P->build_ear_list(&Q, penv.m_sorted_verts);

		bool	ear_was_clipped = false;
		while (P->m_vertex_count > 3)
		{
			if (Q.size())
			{
				// Clip the next ear from Q.
				int	v1 = Q.back();	// @@ or pop randomly!  or sort Q!
				Q.pop_back();
				int	v0 = penv.m_sorted_verts[v1].m_prev;
				int	v2 = penv.m_sorted_verts[v1].m_next;

//				emit_triangle(result, penv.m_sorted_verts, v0, v1, v2);
//				P->remove_ear(&sorted_verts, v1);
				P->emit_and_remove_ear(result, &penv.m_sorted_verts, v0, v1, v2);

				ear_was_clipped = true;

				// @@ is it true that Q always remains valid here?
			}
			else if (ear_was_clipped == true)
			{
				// Re-examine P for new ears.
				P->build_ear_list(&Q, penv.m_sorted_verts);
				ear_was_clipped = false;
			}
			else
			{
				// No valid ears; we're in trouble so try some fallbacks.
				recovery_process(&penv.m_polys, P, &Q, penv.m_sorted_verts);
				ear_was_clipped = false;
			}
		}
		if (P->m_vertex_count == 3)
		{
			// Emit the final triangle.
			P->emit_and_remove_ear(
				result,
				&penv.m_sorted_verts,
				P->m_loop,
				penv.m_sorted_verts[P->m_loop].m_next,
				penv.m_sorted_verts[penv.m_sorted_verts[P->m_loop].m_next].m_next);
		}
		delete P;
	}

	assert(penv.m_polys.size() == 0);
	// assert(for all penv.m_sorted_verts: owning poly == NULL);

	assert((result->size() % 6) == 0);
}


template<class coord_t>
void	recovery_process(
	array<poly<coord_t>*>* polys,
	poly<coord_t>* P,
	array<int>* Q,
	const array<poly_vert<coord_t> >& sorted_verts)
{
	// recovery_process:
	//   if two edges in P, e[i-1] and e[i+1] intersect:
	//     insert two tris incident on e[i-1] & e[i+1] as ears into Q
	//   else if P can be split with a valid diagonal:
	//     P = one side
	//     L += the other side
	//     Q = ears of P
	//   else if P has any convex vertex:
	//     pick a random convex vert and add it to Q
	//   else
	//     pick a random vert and add it to Q
	
	// Case 1: two edges, e[i-1] and e[i+1], intersect; we insert
	// the overlapping ears into Q and resume.
	{for (int vi = sorted_verts[P->m_loop].m_next; vi != P->m_loop; vi = sorted_verts[vi].m_next)
	{
		int	ev0 = vi;
		int	ev1 = sorted_verts[ev0].m_next;
		int	ev2 = sorted_verts[ev1].m_next;
		int	ev3 = sorted_verts[ev2].m_next;
		if (edges_intersect(sorted_verts, ev0, ev1, ev2, ev3))
		{
			// Insert (1,2,3) as an ear.
			Q->push_back(ev2);

			// Resume regular processing.
			return;
		}
	}}

// Because I'm lazy, I'm skipping this test for now...
#if 0
	// Case 2: P can be split with a valid diagonal.
	//
	// A "valid diagonal" passes these checks, according to FIST:
	//
	// 1. diagonal is locally within poly
	//
	// 2. its relative interior does not intersect any edge of the poly
	//
	// 3. the winding number of the polygon w/r/t the midpoint of
	// the diagonal is one
	//
	{for (int vi = verts[0, end])
	{
		for (int vj = verts[vi->m_next, end])
		{
			if (P->valid_diagonal(vi, vj))
			{
				// Split P, insert leftover piece into polys
				poly*	leftover = P->split(vi, vj);
				polys->push_back(leftover);

				// Resume regular processing.
				return;
			}
		}
	}}
#endif // 0

	// Case 3: P has any convex vert
	int	first_vert = P->m_loop;
	int	vi = first_vert;
	int	vert_count = 0;
	do
	{
		if (is_convex_vert(sorted_verts, vi))
		{
			// vi is convex; treat it as an ear,
			// regardless of other problems it may have.
			Q->push_back(vi);

			// Resume regular processing.
			return;
		}
		
		vert_count++;
		vi = sorted_verts[vi].m_next;
	}
	while (vi != first_vert);

	// Case 4: Pick a random vert and treat it as an ear.
	int	random_vert = tu_random::next_random() % vert_count;
	for (vi = first_vert; random_vert > 0; random_vert--)
	{
		vi = sorted_verts[vi].m_next;
	}
	Q->push_back(vi);

	// Resume.
	return;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
