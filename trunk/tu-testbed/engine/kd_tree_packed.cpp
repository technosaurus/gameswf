// kd_tree_packed.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// packed kd-tree structure for accelerated collision queries
// vs. static triangle soup


#include "engine/kd_tree_packed.h"


struct kd_tree_packed::node_chunk
{
	void	ray_test(ray_test_result* result, const ray_test_query& query, int node_idx = 0) const;

	void	get_child(const node_chunk** child, int* child_idx, int idx, int which_child) const;

private:
	// Kudos to Christer Ericson for the sweet packed node union.
	union node
	{
		float	m_plane;	// axis is packed into low 2 bits.  01==x, 02==y, 03==z
		int	m_leaf_offset;	// if low 2 bits == 00 then this is an offset into the leaf array

		// @@ can unions have member functions?
	};

	node	m_nodes[13];		// interior nodes
	int	m_first_child_offset;	// offset of first child chunk, relative to the head of this struct
	int	m_child_offset_bits;	// 1 bit for each existing breadth-first child
};


void	kd_tree_packed::node_chunk::ray_test(
	ray_test_result* result,
	const ray_test_query& query,
	const axial_box& node_bound,
	int node_idx = 0) const
// Ray test against the node_idx'th node in this chunk.
{
	assert(child >= 0 && child < 13);

	const node&	n = m_node[node_idx];
	if (n.is_leaf())
	{
		n.get_leaf()->ray_test(result, query);
	}
	else
	{
		int	axis = node.get_axis();
		float	plane_offset = node.get_plane_offset();

		const node_chunk*	chunk;
		int	child_idx;

		// crossing node always gets queried.
		get_child(&chunk, &child_idx, node_idx, 0);
		chunk->ray_test(result, query, child_idx);

		// cut the ray at plane_offset

		// if query reaches front child
		get_child(&chunk, &child_idx, node_idx, 1);
		chunk->ray_test(result, query, child_idx);
		
		// if query reaches back child
		get_child(&chunk, &child_idx, node_idx, 2);
		chunk->ray_test(result, query, child_idx);
	}
}

