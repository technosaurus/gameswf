// kd_tree_packed.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// packed kd-tree structure for accelerated collision queries
// vs. static triangle soup


#include "engine/kd_tree_packed.h"

#include "engine/axial_box.h"


struct ray_test_result;
struct ray_test_query;


struct kd_tree_packed::node_chunk
{
	void	ray_test(
		ray_test_result* result,
		const ray_test_query& query,
		const axial_box& node_bound,
		int node_idx = 0) const;

	void	get_child(const node_chunk** child, int* child_idx, int idx, int which_child) const;

private:
	struct leaf;

	// Kudos to Christer Ericson for the sweet packed node union.
	union node
	{
		float	m_plane;	// axis is packed into low 2 bits.  01==x, 02==y, 03==z
		int	m_leaf_offset;	// if low 2 bits == 00 then this is an offset into the leaf array

		// @@ can unions have member functions?  MSVC says yes...

		bool	is_leaf() const { return (m_leaf_offset & 3) == 00; }
		const leaf*	get_leaf() const { return (const leaf*) (((const char*) this) + m_leaf_offset); }

		int	get_axis() const
		{
			assert(is_leaf() == false);
			return (m_leaf_offset & 3) - 1;
		}

		float	get_plane_offset() const
		{
			// @@ should we mask off bottom bits?  Or leave them alone?
			return m_plane;
		}
	};

	node	m_nodes[13];		// interior nodes
	int	m_first_child_offset;	// offset of first child chunk, relative to the head of this struct
	int	m_child_offset_bits;	// 1 bit for each existing breadth-first child
};


void	kd_tree_packed::node_chunk::ray_test(
	ray_test_result* result,
	const ray_test_query& query,
	const axial_box& node_bound,
	int node_idx /* = 0 */) const
// Ray test against the node_idx'th node in this chunk.
{
	assert(node_idx >= 0 && node_idx < 13);

	const node&	n = m_nodes[node_idx];
	if (n.is_leaf())
	{
//x		n.get_leaf()->ray_test(result, query);
	}
	else
	{
		int	axis = n.get_axis();
		float	plane_offset = n.get_plane_offset();

		const node_chunk*	chunk;
		int	child_idx;

		// crossing node always gets queried.
		get_child(&chunk, &child_idx, node_idx, 0);
		chunk->ray_test(result, query, node_bound, child_idx);

		// cut the ray at plane_offset

		axial_box	child_box(node_bound);

		// if query reaches front child
		child_box.set_axis_min(axis, plane_offset);
		get_child(&chunk, &child_idx, node_idx, 1);
		chunk->ray_test(result, query, child_box, child_idx);
		
		// if query reaches back child
		child_box = node_bound;
		child_box.set_axis_max(axis, plane_offset);
		get_child(&chunk, &child_idx, node_idx, 2);
		chunk->ray_test(result, query, child_box, child_idx);
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
