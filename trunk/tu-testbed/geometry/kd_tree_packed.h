// kd_tree_packed.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// packed kd-tree structure for accelerated collision queries
// vs. static triangle soup

#ifndef KD_TREE_PACKED_H
#define KD_TREE_PACKED_H


#include "engine/container.h"
#include "engine/geometry.h"


class tu_file;
struct kd_tree_dynamic;


struct kd_tree_packed
{
	kd_tree_packed();
	~kd_tree_packed();

	void	read(tu_file* in);
	void	write(tu_file* out);

	void	build(kd_tree_dynamic* source_tree);

	// void	ray_test(....);
	// void	lss_test(....);

private:
	struct node_chunk;

	vec3*	m_verts;
	node_chunk*	m_packed_tree;
};


#endif // KD_TREE_PACKED_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
