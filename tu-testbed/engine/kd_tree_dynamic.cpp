// kd_tree_dynamic.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility kd-tree structure, for building kd-trees from triangle
// soup.


#include "engine/kd_tree_dynamic.h"


static const float EPSILON = 1e-6f;


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
	axial_box	bounds(axial_box::INVALID, vec3::flt_max, vec3::minus_flt_max);
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


kd_tree_dynamic::~kd_tree_dynamic()
// Destructor; make sure to delete the stuff we allocated.
{
	delete m_root;
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

	assert(bounds.encloses(actual_bounds));

	// Find a good splitting plane.
	float	best_split_quality = 0.0f;
	int	best_split_axis = -1;
	float	best_split_offset = 0.0f;

	for (int axis = 0; axis < 3; axis++)
	{
		if (actual_bounds.get_extent()[axis] < EPSILON)
		{
			// Don't try to divide 
			continue;
		}

		// @@ perhaps make the # of tries dependent on # of faces?
		// Try to align the offset with existing face boundaries?
		// Many opportunities for more smartness here.

		const int	c_divisions = 11;	// odd, so that we always test the bisecting plane.
		for (int i = 0; i < c_divisions; i++)
		{
			float	f = /* some_curve( */ i / float(c_divisions - 1) /* ) */;
			float	offset = flerp(actual_bounds.get_min()[axis], actual_bounds.get_max()[axis], f);
			// @@ force the offset LSBits to something appropriate, based on later munging?

			if (fabsf(offset - bounds.get_min().get(axis)) < EPSILON
				|| fabsf(offset - bounds.get_max().get(axis)) < EPSILON)
			{
				// This plane doesn't actually split the node.
				continue;
			}

			// How good is this split?
			float	quality = evaluate_split(face_count, faces, bounds, axis, offset);
			if (quality > best_split_quality)
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

		axial_box	back_bounds(bounds);
		back_bounds.set_axis_max(best_split_axis, best_split_offset);

		axial_box	front_bounds(bounds);
		front_bounds.set_axis_min(best_split_axis, best_split_offset);

		do_split(&back_end, &cross_end, &front_end, face_count, faces, best_split_axis, best_split_offset, back_bounds, bounds, front_bounds);

		node*	n = new node;
		n->m_axis = best_split_axis;
		n->m_offset = best_split_offset;

		// Recursively build sub-trees.

		// We use the implicit node bounds, not the actual bounds of
		// the face sets, for computing split quality etc, since that
		// is what the run-time structures have when they are
		// computing query results.

		n->m_back = build_tree(back_end, faces + 0, back_bounds);

		n->m_cross = build_tree(cross_end - back_end, faces + back_end, bounds);

		n->m_front = build_tree(front_end - cross_end, faces + cross_end, front_bounds);

		return n;
	}
}


kd_tree_dynamic::node::node()
// Default constructor, null everything out.
	:
	m_back(0),
	m_cross(0),
	m_front(0),
	m_leaf(0),
	m_axis(0),
	m_offset(0.0f)
{
}


kd_tree_dynamic::node::~node()
// Destructor, delete children if any.
{
	delete m_back;
	delete m_cross;
	delete m_front;
	delete m_leaf;
}


bool	kd_tree_dynamic::node::is_valid() const
{
	return
		// internal node.
		(m_leaf == 0
		 && m_axis >= 0
		 && m_axis < 3)
		||
		// leaf node
		(m_leaf != 0
		 && m_back == 0
		 && m_cross == 0
		 && m_front == 0)
		;
}


void	kd_tree_dynamic::compute_actual_bounds(axial_box* result, int face_count, face faces[])
// Compute the actual bounding box around the given list of faces.
{
	assert(face_count > 0);

	result->set_min_max_invalid(vec3::flt_max, vec3::minus_flt_max);
	
	for (int i = 0; i < face_count; i++)
	{
		const face&	f = faces[i];

		// Update bounds.
		result->set_enclosing(m_verts[f.m_vi[0]]);
		result->set_enclosing(m_verts[f.m_vi[1]]);
		result->set_enclosing(m_verts[f.m_vi[2]]);
	}
}


void	kd_tree_dynamic::do_split(
	int* back_end,
	int* cross_end,
	int* front_end,
	int face_count,
	face faces[],
	int axis,
	float offset
	, const axial_box& back_bounds,
	const axial_box& bounds,
	const axial_box& front_bounds
)
// Sort the given faces into three segments within the faces[] array.
// The faces within faces[] are shuffled.  On exit, the faces[] array has the following segments:
//
// [0, *back_end-1]             -- the faces behind the plane axis=offset
// [back_end, cross_end]        -- the faces that cross axis=offset
// [cross_end, face_count-1]    -- the faces in front of axis=offset
{
	array<face>	back_faces;
	array<face>	front_faces;
	array<face>	cross_faces;

	for (int i = 0; i < face_count; i++)
	{
		const face&	f = faces[i];

		int	result = classify_face(f, axis, offset);
		if (result == -1)
		{
			// Behind.
			back_faces.push_back(f);

			assert(back_bounds.encloses(m_verts[f.m_vi[0]]));
			assert(back_bounds.encloses(m_verts[f.m_vi[1]]));
			assert(back_bounds.encloses(m_verts[f.m_vi[2]]));
		}
		else if (result == 0)
		{
			// Crossing.
			cross_faces.push_back(f);
		}
		else
		{
			// Front.
			assert(result == 1);
			front_faces.push_back(f);
		}
	}

	assert(back_faces.size() + cross_faces.size() + front_faces.size() == face_count);

	*back_end = back_faces.size();
	if (back_faces.size() > 0)
	{
		memcpy(&(faces[0]), &(back_faces[0]), back_faces.size() * sizeof(faces[0]));
	}

	*cross_end = *back_end + cross_faces.size();
	if (cross_faces.size() > 0)
	{
		memcpy(&faces[*back_end], &cross_faces[0], cross_faces.size() * sizeof(faces[0]));
	}

	*front_end = *cross_end + front_faces.size();
	if (front_faces.size() > 0)
	{
		memcpy(&faces[*cross_end], &front_faces[0], front_faces.size() * sizeof(faces[0]));
	}

	assert(*back_end <= *cross_end);
	assert(*cross_end <= *front_end);
	assert(*front_end == face_count);
}


float	kd_tree_dynamic::evaluate_split(int face_count, face faces[], const axial_box& bounds, int axis, float offset)
// Compute the "value" of splitting the given set of faces, bounded by
// the given box, along the plane [axis]=offset.  A value of 0 means
// that a split is possible, but has no value.  A negative value means
// that the split is not valid at all.  Positive values indicate
// increasing goodness.
//
// This is kinda heuristicy -- it's where the "special sauce" comes
// in.
{
	// Count the faces that will end up in the groups
	// back,cross,front.
	int	back_count = 0;
	int	cross_count = 0;
	int	front_count = 0;

	for (int i = 0; i < face_count; i++)
	{
		const face&	f = faces[i];

		int	result = classify_face(f, axis, offset);
		if (result == -1)
		{
			// Behind.
			back_count++;
		}
		else if (result == 0)
		{
			// Crossing.
			cross_count++;
		}
		else
		{
			// Front.
			assert(result == 1);
			front_count++;
		}
	}


	if (cross_count == face_count)
	{
		assert(back_count == 0);
		assert(front_count == 0);

		// No faces are separated by this plane; this split is
		// entirely useless.
		return -1;
	}

	float	center = bounds.get_center().get(axis);
	float	extent = bounds.get_extent().get(axis);

	axial_box	back_bounds(bounds);
	back_bounds.set_axis_max(axis, offset);
	axial_box	front_bounds(bounds);
	front_bounds.set_axis_min(axis, offset);

#if 0
	// Special case: if the plane carves off space at one side or the
	// other, without orphaning any faces, then we reward a large
	// empty space.
	float	space_quality = 0.0f;
	if (front_count == 0 && cross_count == 0)
	{
		// All the faces are in back -- reward a bigger empty front volume.
		return space_quality = back_count * front_bounds.get_surface_area();
	}
	else if (back_count == 0 && cross_count == 0)
	{
		// All the faces are in front.
		return space_quality = front_count * back_bounds.get_surface_area();
	}
#endif // 0


// My ad-hoc metric
#if 0
	// compute a figure for how close to the center this splitting
	// plane is.  Normalize in [0,1].
	float	volume_balance = 1.0f - fabsf(center - offset) / extent;

	// Compute a figure for how well we balance the faces.  0 == bad,
	// 1 == good.
	float	face_balance = 1.0f - (fabsf(front_count - back_count) + float(cross_count)) / face_count;

	float	split_quality = bounds.get_surface_area() * volume_balance * face_balance;

	return split_quality;
#endif // 0

	// MacDonald and Booth's metric, as quoted by Havran, endorsed by
	// Ville Miettinen and Atman Binstock:

	float	cost_back = back_bounds.get_surface_area() * back_count;
	float	cost_front = front_bounds.get_surface_area() * front_count;
	float	cost_cross = bounds.get_surface_area() * cross_count;

	float	havran_cost = cost_back + cost_front + cost_cross;

	// We need to turn the cost into a quality, so subtract it from a
	// big number.
	return bounds.get_surface_area() * face_count - havran_cost;
}


int	kd_tree_dynamic::classify_face(const face& f, int axis, float offset)
// Return -1 if the face is entirely behind the plane [axis]=offset
// Return 0 if the face spans the plane.
// Return 1 if the face is entirely in front of the plane.
//
// "behind" means on the negative side, "in front" means on the
// positive side.
{
	assert(axis >= 0 && axis < 3);

	bool	has_front_vert = false;
	bool	has_back_vert = false;

	for (int i = 0; i < 3; i++)
	{
		float	coord = m_verts[f.m_vi[i]].get(axis);

		if (coord < offset - EPSILON)
		{
			has_back_vert = true;
		}
		else if (coord > offset + EPSILON)
		{
			has_front_vert = true;
		}
	}

	if (has_front_vert && has_back_vert)
	{
		return 0;	// crossing.
	}
	else if (has_front_vert)
	{
		return 1;	// all verts in front.
	}
	else if (has_back_vert)
	{
		return -1;	// all verts in back.
	}
	else
	{
		// Face is ON the plane.
		return 0;	// call it "crossing".
	}
}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
