// kd_tree_dynamic.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility kd-tree structure, for building kd-trees from triangle
// soup.


#include "geometry/kd_tree_dynamic.h"

#include "base/tu_file.h"


static const float	EPSILON = 1e-6f;
static const int	LEAF_FACE_COUNT = 6;

#define TRINARY_KD_TREE
//#define CARVE_OFF_SPACE
#define ADHOC_METRIC
//#define MACDONALD_AND_BOOTH_METRIC


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
	if (face_count <= LEAF_FACE_COUNT)
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

	assert(bounds.encloses(actual_bounds, 1e-3f));

	actual_bounds.set_intersection(bounds);

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

		// We use the implicit node bounds, not the actual bounds of
		// the face sets, for computing split quality etc, since that
		// is what the run-time structures have when they are
		// computing query results.

		axial_box	back_bounds(bounds);
		back_bounds.set_axis_max(best_split_axis, best_split_offset);

		axial_box	front_bounds(bounds);
		front_bounds.set_axis_min(best_split_axis, best_split_offset);

		node*	n = new node;
		n->m_axis = best_split_axis;
		n->m_offset = best_split_offset;

		// Recursively build sub-trees.

#ifndef TRINARY_KD_TREE

		// For a binary kd-tree, we duplicate the crossing
		// faces and then clip them, instead of using a third
		// branch.

		array<face>	faces_copy;
		faces_copy.resize(face_count);
		memcpy(&faces_copy[0], faces, sizeof(faces[0]) * face_count);

		clip_faces(&faces_copy, n->m_axis, n->m_offset);

		do_split(&back_end, &cross_end, &front_end, faces_copy.size(), &faces_copy[0], n->m_axis, n->m_offset);

		int	back_count = cross_end;
		int	cross_count = cross_end - back_end;
		int	front_count = faces_copy.size() - back_end;

		assert(cross_count == 0);	// make sure our clipping worked.

		// Sanity check here, to keep clipping from getting out of control...
		if (/*face_count < LEAF_FACE_COUNT * 3 &&*/
		    (back_count > face_count
			|| front_count > face_count))
		{
			// Faces are proliferating!  Just make a leaf.
			n->m_leaf = new leaf;
			n->m_leaf->m_faces = faces_copy;

			return n;
		}

		if (back_count)
		{
			n->m_back = build_tree(back_count, &faces_copy[0], back_bounds);
		}
		n->m_cross = NULL;
		if (front_count)
		{
			n->m_front = build_tree(front_count, &faces_copy[back_count], front_bounds);
		}

#else // TRINARY_KD_TREE

		do_split(&back_end, &cross_end, &front_end, face_count, faces, best_split_axis, best_split_offset);

		n->m_back = build_tree(back_end, faces + 0, back_bounds);

		n->m_cross = build_tree(cross_end - back_end, faces + back_end, bounds);

		n->m_front = build_tree(front_end - cross_end, faces + cross_end, front_bounds);

#endif // TRINARY_KD_TREE

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


static int	classify_coord(float coord, float offset)
{
	if (coord < offset - EPSILON)
	{
		return -1;
	}
	else if (coord > offset + EPSILON)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void	kd_tree_dynamic::do_split(
	int* back_end,
	int* cross_end,
	int* front_end,
	int face_count,
	face faces[],
	int axis,
	float offset)
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
		}
		else if (result == 0)
		{
#ifdef TRINARY_KD_TREE
			// Crossing.
			cross_faces.push_back(f);
#else	// not TRINARY_KD_TREE
			// This must be an "on" or marginal face;
			// i.e. some verts are on the crossing plane.
			// Push it to one side or the other.
			float	coord[3];
			coord[0] = m_verts[f.m_vi[0]].get(axis);
			coord[1] = m_verts[f.m_vi[1]].get(axis);
			coord[2] = m_verts[f.m_vi[2]].get(axis);
			int	side =
				classify_coord(coord[0], offset)
				+ classify_coord(coord[1], offset)
				+ classify_coord(coord[2], offset);
			if (side < 0)
			{
				back_faces.push_back(f);
			}
			else if (side == 0)
			{
				// Arbitrarily put in back.
				back_faces.push_back(f);
			}
			else
			{
				front_faces.push_back(f);
			}
#endif	// not TRINARY_KD_TREE
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

#ifdef CARVE_OFF_SPACE
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
#endif // CARVE_OFF_SPACE


// My ad-hoc metric
#ifdef ADHOC_METRIC
	// compute a figure for how close to the center this splitting
	// plane is.  Normalize in [0,1].
	float	volume_balance = 1.0f - fabsf(center - offset) / extent;

	// Compute a figure for how well we balance the faces.  0 == bad,
	// 1 == good.
	float	face_balance = 1.0f - (fabsf(front_count - back_count) + float(cross_count)) / face_count;

	float	split_quality = bounds.get_surface_area() * volume_balance * face_balance;

	return split_quality;
#endif // ADHOC_METRIC


#ifdef MACDONALD_AND_BOOTH_METRIC
	// MacDonald and Booth's metric, as quoted by Havran, endorsed by
	// Ville Miettinen and Atman Binstock:

	float	cost_back = back_bounds.get_surface_area() * back_count;
	float	cost_front = front_bounds.get_surface_area() * front_count;
	float	cost_cross = bounds.get_surface_area() * cross_count;

	float	havran_cost = cost_back + cost_front + cost_cross;

	// We need to turn the cost into a quality, so subtract it from a
	// big number.
	return bounds.get_surface_area() * face_count - havran_cost;
#endif // MACDONALD_AND_BOOTH_METRIC
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
		int	cr = classify_coord(coord, offset);

		if (cr == -1)
		{
			has_back_vert = true;
		}
		else if (cr == 1)
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


void	kd_tree_dynamic::clip_faces(array<face>* faces, int axis, float offset)
// Clip the given faces against the plane [axis]=offset.  Update the
// *faces array with the newly clipped faces; add faces and verts as
// necessary.
{
	int	original_face_count = faces->size();

	for (int i = 0; i < original_face_count; i++)
	{
		face	f = (*faces)[i];

		if (classify_face(f, axis, offset) == 0)
		{
			// Crossing face, probably needs to be clipped.

			int	vr[3];
			vr[0] = classify_coord(m_verts[f.m_vi[0]].get(axis), offset);
			vr[1] = classify_coord(m_verts[f.m_vi[1]].get(axis), offset);
			vr[2] = classify_coord(m_verts[f.m_vi[2]].get(axis), offset);

			// Sort...
			if (vr[0] > vr[1])
			{
				swap(&vr[0], &vr[1]);
				swap(&f.m_vi[0], &f.m_vi[1]);
			}
			if (vr[1] > vr[2])
			{
				swap(&vr[1], &vr[2]);
				swap(&f.m_vi[1], &f.m_vi[2]);
			}
			if (vr[0] > vr[1])
			{
				swap(&vr[0], &vr[1]);
				swap(&f.m_vi[0], &f.m_vi[1]);
			}

			if (vr[0] == 0 || vr[2] == 0)
			{
				// Face doesn't actually cross; no need to clip.
				continue;
			}
			
			const vec3	v[3] = {
				m_verts[f.m_vi[0]],
				m_verts[f.m_vi[1]],
				m_verts[f.m_vi[2]]
			};

			// Different cases.
			if (vr[1] == 0)
			{
				// Middle vert is on the plane; make two triangles.

				// One new vert.
				float	lerper = (offset - v[0].get(axis)) / (v[2].get(axis) - v[0].get(axis));
				vec3	new_vert = v[0] * (1 - lerper) + v[2] * lerper;
				new_vert.set(axis, offset);	// make damn sure
				assert(new_vert.checknan() == false);

				int	new_vi = m_verts.size();
				m_verts.push_back(new_vert);

				// New faces.
				face	f0 = f;
				f0.m_vi[2] = new_vi;
				(*faces)[i] = f0;	// replace original face

				assert(classify_face(f0, axis, offset) <= 0);
				
				face	f1 = f;
				f1.m_vi[0] = new_vi;
				faces->push_back(f1);	// add a face

				assert(classify_face(f1, axis, offset) >= 0);
			}
			else if (vr[1] < 0)
			{
				// Middle vert is behind the plane.
				// Make two tris behind, one in front.

				// Two new verts.
				float	lerper0 = (offset - v[0].get(axis)) / (v[2].get(axis) - v[0].get(axis));
				vec3	new_vert0 = v[0] * (1 - lerper0) + v[2] * lerper0;
				new_vert0.set(axis, offset);	// make damn sure
				assert(new_vert0.checknan() == false);
				int	new_vi0 = m_verts.size();
				m_verts.push_back(new_vert0);

				float	lerper1 = (offset - v[1].get(axis)) / (v[2].get(axis) - v[1].get(axis));
				vec3	new_vert1 = v[1] * (1 - lerper1) + v[2] * lerper1;
				new_vert1.set(axis, offset);	// make damn sure
				assert(new_vert1.checknan() == false);
				int	new_vi1 = m_verts.size();
				m_verts.push_back(new_vert1);

				// New faces.
				face	f0 = f;
				f0.m_vi[2] = new_vi0;
				(*faces)[i] = f0;

				assert(classify_face(f0, axis, offset) <= 0);

				face	f1 = f;
				f1.m_vi[0] = new_vi0;
				f1.m_vi[2] = new_vi1;
				faces->push_back(f1);

				assert(classify_face(f1, axis, offset) <= 0);

				face	f2 = f;
				f2.m_vi[0] = new_vi0;
				f2.m_vi[1] = new_vi1;
				faces->push_back(f2);

				assert(classify_face(f2, axis, offset) >= 0);
			}
			else if (vr[1] > 0)
			{
				// Middle vert is in front of the plane.
				// Make on tri behind, two in front.

				// Two new verts.
				float	lerper1 = (offset - v[0].get(axis)) / (v[1].get(axis) - v[0].get(axis));
				vec3	new_vert1 = v[0] * (1 - lerper1) + v[1] * lerper1;
				new_vert1.set(axis, offset);	// make damn sure
				assert(new_vert1.checknan() == false);
				int	new_vi1 = m_verts.size();
				m_verts.push_back(new_vert1);

				float	lerper2 = (offset - v[0].get(axis)) / (v[2].get(axis) - v[0].get(axis));
				vec3	new_vert2 = v[0] * (1 - lerper2) + v[2] * lerper2;
				new_vert2.set(axis, offset);	// make damn sure
				assert(new_vert2.checknan() == false);
				int	new_vi2 = m_verts.size();
				m_verts.push_back(new_vert2);

				// New faces.
				face	f0 = f;
				f0.m_vi[1] = new_vi1;
				f0.m_vi[2] = new_vi2;
				(*faces)[i] = f0;

				assert(classify_face(f0, axis, offset) <= 0);

				face	f1 = f;
				f1.m_vi[0] = new_vi1;
				f1.m_vi[2] = new_vi2;
				faces->push_back(f1);

				assert(classify_face(f1, axis, offset) >= 0);

				face	f2 = f;
				f2.m_vi[0] = new_vi2;
				faces->push_back(f2);

				assert(classify_face(f2, axis, offset) >= 0);
			}
		}
		
	}
}


void	kd_tree_dynamic::dump(tu_file* out) const
// Dump some debug info.
{
	node*	n = m_root;

	if (n) n->dump(out, 0);
}


void	kd_tree_dynamic::node::dump(tu_file* out, int depth) const
{
	for (int i = 0; i < depth; i++) { out->write_byte(' '); }

	if (m_leaf)
	{
		int	face_count = m_leaf->m_faces.size();
		char	c = ("0123456789X")[iclamp(face_count, 0, 10)];
		out->write_byte(c);
		out->write_byte('\n');
	}
	else
	{
		out->write_byte('+');
		out->write_byte('\n');
		if (m_back)
		{
			m_back->dump(out, depth + 1);
		}
#ifdef TRINARY_KD_TREE
		if (m_cross)
		{
			m_cross->dump(out, depth + 1);
		}
#endif // TRINARY_KD_TREE
		if (m_front)
		{
			m_front->dump(out, depth + 1);
		}
	}
}


#include "base/postscript.h"


static const int	X_SIZE = 612;
static const int	Y_SIZE = 792;
static const int	MARGIN = 20;


struct kd_diagram_dump_info
{
	postscript*	m_ps;
	int	m_depth;
	int	m_max_depth;
	array<int>	m_width;	// width of the tree at each level
	array<int>	m_max_width;
	array<int>	m_count;	// width so far, during drawing

	// Some stats.
	int	m_leaf_count;
	int	m_node_count;
	int	m_face_count;
	int	m_max_faces_in_leaf;
	int	m_null_children;
	int	m_depth_times_faces;

	kd_diagram_dump_info()
		:
		m_ps(0),
		m_depth(0),
		m_max_depth(0),
		m_leaf_count(0),
		m_node_count(0),
		m_face_count(0),
		m_max_faces_in_leaf(0),
		m_null_children(0),
		m_depth_times_faces(0)
	{
	}

	void	get_node_coords(int* x, int* y)
	{
		float	h_spacing = (X_SIZE - MARGIN*2) / float(m_max_width.back());
		float	adjust = 1.0f;
		if (m_width[m_depth] > 1) adjust = (m_max_width[m_depth] + 1) / float(m_width[m_depth] + 1);

		*x = int(X_SIZE/2 + (m_count[m_depth] - m_width[m_depth] / 2) * h_spacing * adjust);
		*y = Y_SIZE - MARGIN - m_depth * (Y_SIZE - MARGIN*2) / (m_max_depth + 1);
	}

	void	update_stats(kd_tree_dynamic::node* n)
	// Add this node's stats to our totals.
	{
		if (n == 0)
		{
			m_null_children++;
		}
		else if (n->m_leaf)
		{
			m_leaf_count++;

			assert(n->m_leaf);
			int	faces = n->m_leaf->m_faces.size();
			m_face_count += faces;
			if (faces > m_max_faces_in_leaf) m_max_faces_in_leaf = faces;

			m_depth_times_faces += (m_depth + 1) * faces;
		}
		else
		{
			m_node_count++;
		}
	}

	void	diagram_stats()
	// Print some textual stats to the given Postscript stream.
	{
		float	x = MARGIN;
		float	y = Y_SIZE - MARGIN;
		const float	LINE = 10;
#ifdef TRINARY_KD_TREE
		y -= LINE; m_ps->printf(x, y, "Trinary KD-Tree");
#else	// not TRINARY_KD_TREE
		y -= LINE; m_ps->printf(x, y, "Binary KD-Tree");
#endif	// not TRINARY_KD_TREE
#ifdef MACDONALD_AND_BOOTH_METRIC
		y -= LINE; m_ps->printf(x, y, "using MacDonald and Booth metric");
#endif
#ifdef ADHOC_METRIC
		y -= LINE; m_ps->printf(x, y, "using ad-hoc metric");
#endif
#ifdef CARVE_OFF_SPACE
		y -= LINE; m_ps->printf(x, y, "using carve-off-space heuristic");
#endif
		y -= LINE; m_ps->printf(x, y, "leaf face count limit: %d", LEAF_FACE_COUNT);
		y -= LINE; m_ps->printf(x, y, "face ct: %d", m_face_count);
		y -= LINE; m_ps->printf(x, y, "leaf ct: %d", m_leaf_count);
		y -= LINE; m_ps->printf(x, y, "node ct: %d", m_node_count);
		y -= LINE; m_ps->printf(x, y, "null ct: %d", m_null_children);
		y -= LINE; m_ps->printf(x, y, "worst leaf: %d faces", m_max_faces_in_leaf);
		y -= LINE; m_ps->printf(x, y, "max depth: %d", m_max_depth + 1);
		y -= LINE; m_ps->printf(x, y, "avg face depth: %3.2f", m_depth_times_faces / float(m_face_count));
	}
};


static void	node_traverse(kd_diagram_dump_info* inf, kd_tree_dynamic::node* n)
// Traverse the tree, updating inf->m_width.  That's helpful for
// formatting the diagram.
{
	inf->update_stats(n);

	if (inf->m_depth > inf->m_max_depth)
	{
		inf->m_max_depth = inf->m_depth;
	}

	while (inf->m_width.size() <= inf->m_max_depth)
	{
		inf->m_width.push_back(0);
	}

	inf->m_width[inf->m_depth]++;	// count this node.

	if (n && n->m_leaf == 0)
	{
		// Count children.
		inf->m_depth++;
		node_traverse(inf, n->m_back);
#ifdef TRINARY_KD_TREE
		node_traverse(inf, n->m_cross);
#endif // TRINARY_KD_TREE
		node_traverse(inf, n->m_front);
		inf->m_depth--;

		assert(inf->m_depth >= 0);
	}
}


static void	node_diagram(kd_diagram_dump_info* inf, kd_tree_dynamic::node* n, int parent_x, int parent_y)
// Emit Postscript drawing commands to diagram this node in the tree.
{
	// Diagram this node.
	int	x, y;
	inf->get_node_coords(&x, &y);

	// Line to parent.
	inf->m_ps->line(x, y, parent_x, parent_y);

	if (n == 0)
	{
		// NULL --> show a circle w/ slash
		inf->m_ps->circle(x, y, 1);
		inf->m_ps->line(x + 1, y + 1, x - 1, y - 1);
	}
	else if (n->m_leaf)
	{
		// Leaf.  Draw concentric circles.
		int	face_count = n->m_leaf->m_faces.size();
		for (int i = 0; i < face_count + 1; i++)
		{
			inf->m_ps->circle(x, y, 2 + i * 1.0f);
		}
	}
	else
	{
		// Internal node.

		// draw disk
		inf->m_ps->disk(x, y, 1);

		// draw children.
		inf->m_depth++;
		node_diagram(inf, n->m_back, x, y);
#ifdef TRINARY_KD_TREE
		node_diagram(inf, n->m_cross, x, y);
#endif // TRINARY_KD_TREE
		node_diagram(inf, n->m_front, x, y);
		inf->m_depth--;

		assert(inf->m_depth >= 0);
	}

	// count this node.
	inf->m_count[inf->m_depth]++;
}


void	kd_tree_dynamic::diagram_dump(tu_file* out) const
// Generate a Postscript schematic diagram of the tree.
{
	postscript*	ps = new postscript(out, "kd-tree diagram");
	
	kd_diagram_dump_info	inf;
	inf.m_ps = ps;
	inf.m_depth = 0;

	node_traverse(&inf, m_root);

	while (inf.m_count.size() <= inf.m_max_depth)
	{
		inf.m_count.push_back(0);
	}

	int	max_width = 1;
	for (int i = 0; i <= inf.m_max_depth; i++)
	{
		if (inf.m_width[i] > max_width)
		{
			max_width = inf.m_width[i];
		}
		inf.m_max_width.push_back(max_width);
	}

	inf.diagram_stats();

	int	root_x = 0, root_y = 0;
	inf.get_node_coords(&root_x, &root_y);

	node_diagram(&inf, m_root, root_x, root_y);

	delete ps;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
