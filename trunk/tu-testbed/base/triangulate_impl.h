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


// Return {-1,0,1} if c is {to the right, on, to the left} of the
// directed edge defined by a->b.
template<class coord_t>
inline int	vertex_left_test(const vec2<coord_t>& a, const vec2<coord_t>& b, const vec2<coord_t>& c)
{
	compiler_assert(0);	// must specialize
}


template<>
inline int	vertex_left_test(const vec2<float>& a, const vec2<float>& b, const vec2<float>& c)
// Specialize for vec2<float>
{
	double	det = determinant_float(a, b, c);
	if (det > 0) return 1;
	else if (det < 0) return -1;
	else return 0;
}


template<>
inline int	vertex_left_test(const vec2<sint32>& a, const vec2<sint32>& b, const vec2<sint32>& c)
// Specialize for vec2<sint32>
{
	sint64	det = determinant_sint32(a, b, c);
	if (det > 0) return 1;
	else if (det < 0) return -1;
	else return 0;
}


template<class coord_t>
bool	vertex_in_ear(const vec2<coord_t>& v, const vec2<coord_t>& a, const vec2<coord_t>& b, const vec2<coord_t>& c)
// Return true if v is on or inside the ear (a,b,c).
// (a,b,c) must be in ccw order!
{
	assert(vertex_left_test(b, a, c) <= 0);	// check ccw order

	if (v == a || v == c)
	{
		// Special case; we don't care if v is coincident with a or c.
		return false;
	}

	// Include the triangle boundary in our test.
	bool	ab_in = vertex_left_test(a, b, v) >= 0;
	bool	bc_in = vertex_left_test(b, c, v) >= 0;
	bool	ca_in = vertex_left_test(c, a, v) >= 0;

	return ab_in && bc_in && ca_in;
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

	// Note: exact equality here.  I think the reason to use
	// epsilons would be underflow in case of very small
	// determinants.  Our determinants are doubles, so I think
	// we're good.
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

	// Note: we do >= 0 checks here, instead of > 0.  We are
	// intentionally not considering it an intersection if an
	// endpoint is coincident with one edge, or if the edges
	// overlap perfectly.  The overlap case is common with
	// perfectly vertical edges at the same coordinate; it doesn't
	// hurt us any to treat them as non-crossing.

	if (det10 * det11 >= 0)
	{
		// e1 doesn't cross the line of e0.
		return false;
	}

	// See if e0 crosses line of e1.
	double	det00 = determinant_float(e1v0, e1v1, e0v0);
	double	det01 = determinant_float(e1v0, e1v1, e0v1);

	if (det00 * det01 >= 0)
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
			// Both edges are zero length.  Treat them as
			// non-coincident (even if they're all on the
			// same point).
			return false;
		}
	}

	// See if e1 crosses line of e0.
	sint64	det10 = determinant_sint32(e0v0, e0v1, e1v0);
	sint64	det11 = determinant_sint32(e0v0, e0v1, e1v1);

	// Note: we do >= 0 checks here, instead of > 0.  We are
	// intentionally not considering it an intersection if an
	// endpoint is coincident with one edge, or if the edges
	// overlap perfectly.  The overlap case is common with
	// perfectly vertical edges at the same coordinate; it doesn't
	// hurt us any to treat them as non-crossing.

	if (det10 * det11 >= 0)
	{
		// e1 doesn't cross the line of e0.
		return false;
	}

	// See if e0 crosses line of e1.
	sint64	det00 = determinant_sint32(e1v0, e1v1, e0v0);
	sint64	det01 = determinant_sint32(e1v0, e1v1, e0v1);

	if (det00 * det01 >= 0)
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
	//
	// Note: treat this as no intersection!  This is mainly useful
	// for things like coincident vertical bridge edges.
	if (coincident[0][0] && coincident[1][1]) return false;
	if (coincident[1][0] && coincident[0][1]) return false;

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

	return vertex_left_test<coord_t>(pv_prev->m_v, pvi->m_v, pv_next->m_v) > 0;
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

	void	build_ear_list(array<int>* ear_list, const array<vert_t>& sorted_verts, tu_random::generator* rg);

	void	emit_and_remove_ear(array<coord_t>* result, array<vert_t>* sorted_verts, int v0, int v1, int v2);
	bool	any_edge_intersection(const array<vert_t>& sorted_verts, int v1, int v2);

	bool	ear_contains_reflex_vertex(const array<vert_t>& sorted_verts, int v0, int v1, int v2);
	bool	vert_in_cone(const array<vert_t>& sorted_verts, int vert, int cone_v0, int cone_v1, int cone_v2);

	bool	is_valid(const array<vert_t>& sorted_verts, bool check_consecutive_dupes = true) const;

	void	invalidate(const array<vert_t>& sorted_verts);

//data:
	int	m_loop;	// index of first vert
	int	m_leftmost_vert;
	int	m_vertex_count;

	// edge search_index;
};


template<class coord_t>
bool	poly<coord_t>::is_valid(const array<vert_t>& sorted_verts, bool check_consecutive_dupes /* = true */) const
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

		if (check_consecutive_dupes && v_next != vi)
		{
			// Subsequent verts are not allowed to be
			// coincident; that causes errors in ear
			// classification.
			assert((sorted_verts[vi].m_v == sorted_verts[v_next].m_v) == false);
		}

		vert_count++;
		vi = v_next;
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
	assert(is_valid(*sorted_verts, false /* don't check for consecutive dupes, poly is not finished */));

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

	assert(is_valid(*sorted_verts, false /* don't check for consecutive dupes, poly is not finished */));
}


template<class coord_t>
int	poly<coord_t>::find_valid_bridge_vert(const array<vert_t>& sorted_verts, int v1)
// Find a vert v, in this poly, such that v is to the left of v1, and
// the edge (v,v1) doesn't intersect any edges in this poly.
{
	assert(is_valid(sorted_verts));

	const poly_vert<coord_t>*	pv1 = &(sorted_verts[v1]);

	// Held recommends searching verts near v1 first.  And for
	// correctness, we may only consider verts to the left of v1.
	// A fast & easy way to implement this is to walk backwards in
	// our vert array, starting with v1-1.

	for (int vi = v1 - 1; vi >= 0; vi--)
	{
		const poly_vert<coord_t>*	pvi = &sorted_verts[vi];

		assert(compare_vertices<coord_t>((void*) pvi, (void*) pv1) <= 0);

		if (pvi->m_poly_owner == this)
		{
			// Candidate is to the left of pv1, so it
			// might be valid.  We don't consider verts to
			// the right of v1, because of possible
			// intersection with other polys.  Due to the
			// poly sorting, we know that the edge
			// (pvi,pv1) can only intersect this poly.

			if (any_edge_intersection(sorted_verts, v1, vi) == false)
			{
				return vi;
			}
		}
		else
		{
			// pvi is owned by some other poly, which I
			// believe signifies bad input, or possibly
			// bad poly sorting?
			//
			// Assert for now; maybe later this gets
			// demoted to a log message, or even ignored.
			assert(0);
		}
	}

	// Ugh!  No valid bridge vert.  Shouldn't happen with valid
	// data.  For invalid data, just pick something and live with
	// the intersection.
	fprintf(stderr, "can't find bridge for vert %d!\n", v1);//xxxxxxxxx

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
void	poly<coord_t>::build_ear_list(array<int>* ear_list, const array<vert_t>& sorted_verts, tu_random::generator* rg)
// Fill in *ear_list with a list of ears that can be clipped.
{
	assert(is_valid(sorted_verts));

	// Optimization: limit the size of the ear list to some
	// smallish number.  After scanning, move m_loop to after the
	// last ear, so the next scan will pick up in fresh territory.
	//
	// We do this to avoid an N^2 search for invalid ears, after
	// clipping an ear.
	//
	// I'm not sure there's much/any cost to keeping this value
	// small.
	int	MAX_EAR_COUNT = 4;

// define this if you want to randomize the ear selection (should
// improve the average ear shape, at low cost).
//#define RANDOMIZE
#ifdef RANDOMIZE
	// Randomization: skip a random number of verts before
	// starting to look for ears.  (Or skip some random verts in
	// between ears?)
	if (m_vertex_count > 6)
	{
		// Decide how many verts to skip.

		// Here's a lot of twiddling to avoid a % op.  Worth it?
		int	random_range = m_vertex_count >> 2;
		static const int	MASK_TABLE_SIZE = 8;
		int	random_mask[MASK_TABLE_SIZE] = {
			1, 1, 1, 3, 3, 3, 3, 7	// roughly, the largest (2^N-1) <= index
		};
		if (random_range >= MASK_TABLE_SIZE) random_range = MASK_TABLE_SIZE - 1;
		assert(random_range > 0);

		int	random_skip = rg->next_random() & random_mask[random_range];

		// Do the skipping, by manipulating m_loop.
		while (random_skip-- > 0)
		{
			m_loop = sorted_verts[m_loop].m_next;
		}
	}
#endif // RANDOMIZE

	assert(is_valid(sorted_verts));
	assert(ear_list);
	assert(ear_list->size() == 0);

	int	first_vert = m_loop;
	int	vi = first_vert;
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

		bool	is_ear = false;

		if ((pvi->m_v == pv_next->m_v)
		    || (pvi->m_v == pv_prev->m_v)
		    || vertex_left_test(pv_prev->m_v, pvi->m_v, pv_next->m_v) == 0)	// @@ correct?
		{
			// Degenerate case: zero-area triangle.
			//
			// Clip it first.  (Clipper pops from the back.)
			ear_list->push_back(vi);

			// Clip it right away.
			break;
		}
		else if (vertex_left_test<coord_t>(pv_prev->m_v, pvi->m_v, pv_next->m_v) > 0)
		{
			if (vert_in_cone(sorted_verts, pvi->m_prev, vi, pvi->m_next, pv_next->m_next)
			    && vert_in_cone(sorted_verts, pvi->m_next, pv_prev->m_prev, pvi->m_prev, vi))
			{
				if (! ear_contains_reflex_vertex(sorted_verts, pvi->m_prev, vi, pvi->m_next))
				{
					// Valid ear.
					ear_list->push_back(vi);
					is_ear = true;
				}
			}
		}

		if (is_ear)
		{
			// Add to the ear list.
			if (ear_list->size() >= MAX_EAR_COUNT)
			{
				// Don't add any more ears.

				// Adjust m_loop so that the next
				// search starts at the end of the
				// current search.
				m_loop = pvi->m_next;

				break;
			}
		}

		vi = pvi->m_next;
	}
	while (vi != first_vert);

	assert(is_valid(sorted_verts));
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

	if (vertex_left_test(pv0->m_v, pv1->m_v, pv2->m_v) == 0)
	{
		// Degenerate triangle!  Don't emit it.
	}
	else
	{
		// emit the vertex list for the triangle.
		result->push_back(pv0->m_v.x);
		result->push_back(pv0->m_v.y);
		result->push_back(pv1->m_v.x);
		result->push_back(pv1->m_v.y);
		result->push_back(pv2->m_v.x);
		result->push_back(pv2->m_v.y);
	}

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

	if (pv0->m_v == pv2->m_v)
	{
		// We've created a duplicate vertex by removing a
		// degenerate ear.  Must eliminate the duplication.

		// Unlink pv2.
		if (m_loop == v2)
		{
			m_loop = v0;
		}
		if (m_leftmost_vert == v2)
		{
			m_leftmost_vert = -1;
		}

		pv0->m_next = pv2->m_next;
		(*sorted_verts)[pv2->m_next].m_prev = v0;

		m_vertex_count--;
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
bool	poly<coord_t>::ear_contains_reflex_vertex(const array<vert_t>& sorted_verts, int v0, int v1, int v2)
// Return true if any of this poly's reflex verts are inside the
// specified ear.  The definition of inside is: a reflex vertex in the
// interior of the triangle (v0,v1,v2), or on the segments [v1,v0) or
// [v1,v2).
{
	// @@ TODO an accel structure could help here, for very large
	// polys in some configurations.  Could use the edge spatial
	// index to find verts within the triangle bound.
	//
	// Currently the horizontal culling is fast due to the sorted
	// indices.  The vertical culling has early outs, but could
	// possibly have to look at lots of verts if there is a ton of
	// vertical overlap.

	assert(is_valid(sorted_verts));

	// Find the min/max of our triangle indices, for fast
	// index-based culling.
	int	min_index = v0;
	int	max_index = v0;
	if (v1 < min_index) min_index = v1;
	if (v1 > max_index) max_index = v1;
	if (v2 < min_index) min_index = v2;
	if (v2 > max_index) max_index = v2;

	// Careful here: must expand min/max to include coincident
	// verts in our traversal.
	while (min_index > 0 && sorted_verts[min_index - 1].m_v == sorted_verts[min_index].m_v)
	{
		min_index--;
	}
	while (max_index < sorted_verts.size() - 1 && sorted_verts[max_index + 1].m_v == sorted_verts[max_index].m_v)
	{
		max_index++;
	}

	assert(min_index < max_index);
	assert(min_index <= v0 && min_index <= v1 && min_index <= v2);
	assert(max_index >= v0 && max_index >= v1 && max_index >= v2);

	// Find min/max vertical bounds, for culling.
	coord_t	min_y = sorted_verts[v0].m_v.y;
	coord_t	max_y = sorted_verts[v0].m_v.y;
	if (sorted_verts[v1].m_v.y < min_y) min_y = sorted_verts[v1].m_v.y;
	if (sorted_verts[v1].m_v.y > max_y) max_y = sorted_verts[v1].m_v.y;
	if (sorted_verts[v2].m_v.y < min_y) min_y = sorted_verts[v2].m_v.y;
	if (sorted_verts[v2].m_v.y > max_y) max_y = sorted_verts[v2].m_v.y;
	assert(min_y <= max_y);
	assert(min_y <= sorted_verts[v0].m_v.y && min_y <= sorted_verts[v1].m_v.y && min_y <= sorted_verts[v2].m_v.y);
	assert(max_y >= sorted_verts[v0].m_v.y && max_y >= sorted_verts[v1].m_v.y && max_y >= sorted_verts[v2].m_v.y);

	// We're using the sorted vert array to reduce our search
	// area.  Walk between min_index and max_index, only
	// considering verts with m_poly_owner==this.  This cheaply
	// considers only verts within the horizontal bounds of the
	// triangle.
	for (int vk = min_index; vk <= max_index; vk++)
	{
		const vert_t*	pvk = &sorted_verts[vk];
		if (pvk->m_poly_owner != this)
		{
			// Not part of this poly; ignore it.
			continue;
		}

		if (vk != v0 && vk != v1 && vk != v2
		    && pvk->m_v.y >= min_y
		    && pvk->m_v.y <= max_y)
		{
			// vk is within the triangle's bounding box.

			//xxxxxx
			if (v1 == 131 && vk == 132)
			{
				vk = vk;//xxxxx
			}

			int	v_next = pvk->m_next;
			int	v_prev = pvk->m_prev;

			// @@ cache this in vert?  Must refresh when
			// ear is clipped, polys are joined, etc.
			bool	reflex = vertex_left_test(sorted_verts[v_next].m_v, pvk->m_v, sorted_verts[v_prev].m_v) > 0;

			if (reflex)
			{
				if (pvk->m_v == sorted_verts[v1].m_v)
				{
					// Tricky case.  See section 4.3 in FIST paper.

					int	v_prev_left = vertex_left_test(
						sorted_verts[v1].m_v,
						sorted_verts[v2].m_v,
						sorted_verts[v_prev].m_v);
					int	v_next_left = vertex_left_test(
						sorted_verts[v0].m_v,
						sorted_verts[v1].m_v,
						sorted_verts[v_next].m_v);

					if (v_prev_left > 0 || v_next_left > 0)
					{
						// Local interior near vk intersects this
						// ear; ear is clearly not valid.
						return true;
					}

					// Check colinear case, where cones of vk and v1 overlap exactly.
					if (v_prev_left == 0 && v_next_left == 0)
					{
						// @@ TODO: there's a somewhat complex non-local area test that tells us
						// whether vk qualifies as a contained reflex vert.
						//
						// For the moment, deny the ear.
						//
						// The question is, is this test required for correctness?  Because it
						// seems pretty expensive to compute.  If it's not required, I think it's
						// better to always assume the ear is invalidated.

						//xxx
						//fprintf(stderr, "colinear case in ear_contains_reflex_vertex; returning true\n");

						return true;
					}
				}
				else if (vertex_in_ear(
						 pvk->m_v, sorted_verts[v0].m_v, sorted_verts[v1].m_v, sorted_verts[v2].m_v))
				{
					// Found one.
					return true;
				}
			}
		}
	}

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
	bool	acute_cone = vertex_left_test(sorted_verts[cone_v0].m_v, sorted_verts[cone_v1].m_v, sorted_verts[cone_v2].m_v) > 0;

	// Include boundary in our tests.
	bool	left_of_01 = 
		vertex_left_test(sorted_verts[cone_v0].m_v, sorted_verts[cone_v1].m_v, sorted_verts[vert].m_v) >= 0;
	bool	left_of_12 =
		vertex_left_test(sorted_verts[cone_v1].m_v, sorted_verts[cone_v2].m_v, sorted_verts[vert].m_v) >= 0;

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
		const array<coord_t>&	path = paths[i];

		if (path.size() < 3)
		{
			// Degenerate path, ignore it.
			continue;
		}

		poly<coord_t>*	p = new poly<coord_t>;
		m_polys.push_back(p);

		// Add this path's verts to our list.
		int	path_size = path.size();
		if (path.size() & 1)
		{
			// Bad input, odd number of coords.
			assert(0);
			fprintf(stderr, "path[%d] has odd number of coords (%d), dropping last value\n", i, path.size());//xxxx
			path_size--;
		}
		for (int j = 0; j < path_size; j += 2)	// vertex coords come in pairs.
		{
			int	prev_point = j - 2;
			if (j == 0) prev_point = path_size - 2;

			if (path[j] == path[prev_point] && path[j + 1] == path[prev_point + 1])
			{
				// Duplicate point; drop it.
				continue;
			}

			// Insert point.
			int	vert_index = m_sorted_verts.size();

			poly_vert<coord_t>	vert(path[j], path[j+1], p, vert_index);
			m_sorted_verts.push_back(vert);

			p->append_vert(&m_sorted_verts, vert_index);
		}
		assert(p->is_valid(m_sorted_verts));

		if (p->m_vertex_count == 0)
		{
			// This path was degenerate; kill it.
			delete p;
			m_polys.pop_back();
		}
	}

	// Sort the vertices.
	qsort(&m_sorted_verts[0], m_sorted_verts.size(), sizeof(m_sorted_verts[0]), compare_vertices<coord_t>);
	assert(m_sorted_verts.size() <= 1
	       || compare_vertices<coord_t>((void*) &m_sorted_verts[0], (void*) &m_sorted_verts[1]) <= 0);	// check order

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
	const array<poly_vert<coord_t> >& sorted_verts,
	tu_random::generator* rg);


template<class coord_t>
inline void	debug_emit_poly_loop(
	array<coord_t>* result,
	const array<poly_vert<coord_t> >& sorted_verts,
	poly<coord_t>* P)
// Fill *result with a poly loop representing P.
{
	result->resize(0);	// clear existing junk.

	int	first_vert = P->m_loop;
	int	vi = first_vert;
	do
	{
		result->push_back(sorted_verts[vi].m_v.x);
		result->push_back(sorted_verts[vi].m_v.y);
		vi = sorted_verts[vi].m_next;
	}
	while (vi != first_vert);

	// Loop back to beginning, and pad to a multiple of 3 coords.
	do
	{
		result->push_back(sorted_verts[vi].m_v.x);
		result->push_back(sorted_verts[vi].m_v.y);
	}
	while (result->size() % 6);
}


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

	// Local generator, for some parts of the algo that need random numbers.
	tu_random::generator	rand_gen;

	// Poly environment; most of the state of the algo.
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
		P->build_ear_list(&Q, penv.m_sorted_verts, &rand_gen);

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

				P->emit_and_remove_ear(result, &penv.m_sorted_verts, v0, v1, v2);

				ear_was_clipped = true;

				// v0 and v2 must be removed from ear
				// list, if they're in it.
				//
				// @@ sucks, must optimize (perhaps limit the size of Q?)
				for (int i = 0, n = Q.size(); i < n; i++)
				{
					if (Q[i] == v0 || Q[i] == v2)
					{
						Q.remove(i);	// or erase_u() or something
						i--;
						n--;
					}
				}

#if 0
				// debug hack: emit current state of P
				static int s_tricount = 0;
				s_tricount++;
				if (s_tricount >= 100)	// 87
				{
					debug_emit_poly_loop(result, penv.m_sorted_verts, P);
					return;
				}
#endif // HACK


			}
			else if (ear_was_clipped == true)
			{
				// Re-examine P for new ears.
				P->build_ear_list(&Q, penv.m_sorted_verts, &rand_gen);
				ear_was_clipped = false;
			}
			else
			{
				// No valid ears; we're in trouble so try some fallbacks.

#if 0
				// xxx for debugging: show the state of P when we hit the recovery process.
				debug_emit_poly_loop(result, penv.m_sorted_verts, P);
				return;
#endif

				recovery_process(&penv.m_polys, P, &Q, penv.m_sorted_verts, &rand_gen);
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
	const array<poly_vert<coord_t> >& sorted_verts,
	tu_random::generator* rg)
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

			fprintf(stderr, "recovery_process: self-intersecting sequence, treating %d as an ear\n", ev2);//xxxx

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

			fprintf(stderr, "recovery_process: found convex vert, treating %d as an ear\n", vi);//xxxx

			// Resume regular processing.
			return;
		}
		
		vert_count++;
		vi = sorted_verts[vi].m_next;
	}
	while (vi != first_vert);

	// Case 4: Pick a random vert and treat it as an ear.
	int	random_vert = rg->next_random() % vert_count;
	for (vi = first_vert; random_vert > 0; random_vert--)
	{
		vi = sorted_verts[vi].m_next;
	}
	Q->push_back(vi);

	fprintf(stderr, "recovery_process: treating random vert %d as an ear\n", vi);//xxxx

	// Resume.
	return;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
