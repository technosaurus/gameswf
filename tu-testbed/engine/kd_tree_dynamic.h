// kd_tree_dynamic.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Data structure for building a kd-tree from a triangle mesh.


#ifndef KD_TREE_DYNAMIC_H
#define KD_TREE_DYNAMIC_H


#include "engine/container.h"
#include "engine/geometry.h"


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

		bool	is_valid() const;
	};

	const node*	get_root() const { return m_root; }

private:
	array<vec3>	m_verts;
	node*	m_root;
};


#endif // KD_TREE_DYNAMIC_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
