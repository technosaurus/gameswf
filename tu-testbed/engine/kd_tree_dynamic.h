// kd_tree_dynamic.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Data structure for building a kd-tree from a triangle mesh.


#ifndef KD_TREE_DYNAMIC_H
#define KD_TREE_DYNAMIC_H


#include "engine/container.h"
#include "engine/geometry.h"
#include "engine/axial_box.h"


struct kd_tree_dynamic
{
	kd_tree_dynamic(
		int vert_count,
		const vec3 verts[],
		int triangle_count,
		const int indices[]
		);
	~kd_tree_dynamic();

	struct face
	{
		Uint16	m_vi[3];	// indices of verts
		Uint16	m_flags;
	};

	struct leaf
	{
		array<face>	m_faces;
	};

	// Internal node.  Not too tidy; would use unions etc. if it were
	// important.
	struct node
	{
		node*	m_back;
		node*	m_cross;
		node*	m_front;
		leaf*	m_leaf;
		int		m_axis;	// split axis: 0 = x, 1 = y, 2 = z
		float	m_offset;	// where the split occurs

		node();
		~node();
		bool	is_valid() const;
	};

	const array<vec3>&	get_verts() const { return m_verts; }
	const node*	get_root() const { return m_root; }
	const axial_box&	get_bound() const { return m_bound; }

private:
	void	compute_actual_bounds(axial_box* result, int face_count, face faces[]);
	node*	build_tree(int face_count, face faces[], const axial_box& bounds);
	void	do_split(
		int* back_end,
		int* cross_end,
		int* front_end,
		int face_count,
		face faces[],
		int axis,
		float offset
		// for debugging
		, const axial_box& back_bounds
		, const axial_box& bounds
		, const axial_box& front_bounds
		);
	float	evaluate_split(int face_count, face faces[], const axial_box& bounds, int axis, float offset);
	int	classify_face(const face& f, int axis, float offset);

	array<vec3>	m_verts;
	node*	m_root;
	axial_box	m_bound;
};


#endif // KD_TREE_DYNAMIC_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
