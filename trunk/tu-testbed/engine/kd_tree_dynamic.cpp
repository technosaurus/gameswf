// kd_tree_dynamic.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility kd-tree structure, for building kd-trees from triangle
// soup.


#include "engine/kd_tree_dynamic.h"


kd_tree_dynamic::kd_tree_dynamic(
	int vert_count,
	const vec3 verts[],
	int triangle_count,
	const int indices[])
// Constructor; build the kd-tree from the given triangle soup.
{
	assert(vert_count > 0);
	assert(triangle_count > 0);

	// Copy the verts.
	m_verts.resize(vert_count);
	memcpy(&m_verts[0], verts, sizeof(verts[0]) * vert_count);

	// Make a mutable array of faces, and also compute our bounds.
	axial_box	bounds(vec3::flt_max, vec3::minus_flt_max);
	array<face>	faces;
	for (int i = 0; i < triangle_count; i++)
	{
		face	f;
		f.m_vi[0] = indices[i * 3 + 0];
		f.m_vi[1] = indices[i * 3 + 1];
		f.m_vi[2] = indices[i * 3 + 2];
		f.m_flags = 0;	// @@ should be a way to initialize this

		faces.push_back(f);

		// Update bounds.
		bounds.set_enclosing(m_verts[f.m_vi[0]]);
		bounds.set_enclosing(m_verts[f.m_vi[1]]);
		bounds.set_enclosing(m_verts[f.m_vi[2]]);
	}

	// @@ round the node bounds up, to account for the low bits we're going to munge.

	m_root = build_tree(faces.size(), &faces[0], bounds);
}


kd_tree_dynamic::node*	kd_tree_dynamic::build_tree(int face_count, face faces[], const axial_box& bounds)
// Recursively build a kd-tree from the given set of faces.  Return
// the root of the tree.
{
	assert(face_count >= 0);

	if (face_count == 0)
	{
		return NULL;
	}

	// Should we make a leaf?
	const int	c_leaf_face_count = 6;
	if (face_count <= c_leaf_face_count)
	{
		// Make a leaf
		node*	n = new node;
		n->m_leaf = new leaf;
		n->m_leaf->m_faces.resize(face_count);
		memcpy(&(n->m_leaf->m_faces[0]), faces, sizeof(faces[0]) * face_count);

		return n;
	}

	axial_box	actual_bounds;
	compute_actual_bounds(&actual_bounds, face_count, faces);

	// Find a splitting plane.
	float	best_split_quality = 0.0f;
	int	best_split_axis = -1;
	float	best_split_offset = 0.0f;

	for (int axis = 0; axis < 3; axis++)
	{
		if (actual_bounds.get_extent()[axis] < 1e-6f)
		{
			// Don't try to divide 
			continue;
		}

		const int	c_divisions = 10;
		for (int i = 1; i < c_divisions - 1; i++)
		{
			float	f = /* some_curve( */ i / float(c_divisions - 1) /* ) */;
			float	offset = flerp(actual_bounds.get_min()[axis], actual_bounds.get_max()[axis], f);
			// @@ force the offset LSBits to something appropriate, based on later munging?

			// How good is this split?
			float	quality = evaluate_split(face_count, faces, bounds, axis, offset);
			if (quality >= best_split_quality)
			{
				// Best so far.
				best_split_quality = quality;
				best_split_axis = axis;
				best_split_offset = offset;
			}
		}
	}

	if (best_split_axis == -1)
	{
		// Couldn't find any acceptable split!
		// Make a leaf.
		node*	n = new node;
		n->m_leaf = new leaf;
		n->m_leaf->m_faces.resize(face_count);
		memcpy(&(n->m_leaf->m_faces[0]), faces, sizeof(faces[0]) * face_count);

		return n;
	}
	else
	{
		// Make the split.
		int	back_end = 0;
		int	cross_end = 0;
		int	front_end = 0;

		do_split(&back_end, &cross_end, &front_end, face_count, faces, best_split_axis, best_split_offset);

		node*	n = new node;
		n->m_axis = best_split_axis;
		n->m_offset = best_split_offset;

		// Recursively build sub-trees.

		// We use the implicit node bounds, not the actual bounds of
		// the face sets, for computing split quality etc, since that
		// is what the run-time structures have when they are
		// computing query results.

		axial_box	back_bounds(bounds);
		back_bounds.set_axis_max(best_split_axis, best_split_offset);
		n->m_back = build_tree(back_end, faces + 0, back_bounds);

		n->m_cross = build_tree(cross_end - back_end, faces + back_end, bounds);

		axial_box	front_bounds(bounds);
		front_bounds.set_axis_min(best_split_axis, best_split_offset);
		n->m_front = build_tree(front_end - cross_end, faces + cross_end, front_bounds);

		return n;
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
