// test_kd_tree.cpp	-- Thatcher Ulrich <http://tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test program for kd-tree mesh collision.


#include "geometry/kd_tree_packed.h"
#include "geometry/kd_tree_dynamic.h"
#include "geometry/axial_box.h"
#include "base/tu_file.h"
#include "base/tu_timer.h"


void	print_usage()
{
	printf(
		"kd-tree code tester\n"
		"\n"
		"specify the desired .txt filename containing mesh info\n"
		"\n"
		"options\n"
		"  -d    (default) dump tree diagram to stdout, in PostScript format\n"
		"  -x    dump mesh diagram projected along x axis, in PostScript format\n"
		"  -y    dump mesh diagram projected along y axis, in PostScript format\n"
		"  -z    dump mesh diagram projected along z axis, in PostScript format\n"
		"  -r    shoot random rays at the model and print timings\n"
		);
}


static void	make_kd_trees(array<kd_tree_dynamic*>* treelist, const char* filename);
static void	test_cast_against_tree(const array<kd_tree_dynamic*>& treelist);


int main(int argc, const char** argv)
{
	const char*	infile = NULL;
	bool	do_dump = true;
	bool	do_ray_test = false;
	bool	do_mesh_dump = false;
	int	mesh_axis = 0;

	// Parse args.
	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Switches.
			switch (argv[arg][1])
			{
			default:
				printf("unknown option '%s'\n", argv[arg]);
				print_usage();
				exit(1);
				break;

			case 'h':	// help
				print_usage();
				exit(1);
				break;

			case 'd':
				do_dump = true;
				do_ray_test = false;
				do_mesh_dump = false;
				break;

			case 'r':
				do_dump = false;
				do_ray_test = true;
				do_mesh_dump = false;
				break;

			case 'x':
				do_dump = false;
				do_ray_test = false;
				do_mesh_dump = true;
				mesh_axis = 0;
				break;
			case 'y':
				do_dump = false;
				do_ray_test = false;
				do_mesh_dump = true;
				mesh_axis = 1;
				break;
			case 'z':
				do_dump = false;
				do_ray_test = false;
				do_mesh_dump = true;
				mesh_axis = 2;
				break;
			}
		}
		else
		{
			// Must be a filename.
			if (infile == NULL)
			{
				infile = argv[arg];
			}
			else
			{
				printf("error: more than one filename specified.\n");
				print_usage();
				exit(1);
			}
		}
	}

	if (infile == NULL)
	{
		print_usage();
		exit(1);
	}

	uint64	start_ticks = tu_timer::get_profile_ticks();
	array<kd_tree_dynamic*>	treelist;
	make_kd_trees(&treelist, infile);
	uint64	end_ticks = tu_timer::get_profile_ticks();
	if (do_ray_test)
	{
		// Print timings for building.
		printf("read file and built kd_tree_dynamic in %3.3f seconds\n",
		       tu_timer::profile_ticks_to_seconds(end_ticks - start_ticks));
	}

	if (do_dump && treelist.size())
	{
		treelist[0]->diagram_dump(&tu_file(stdout, false));
	}

	if (do_mesh_dump && treelist.size())
	{
		treelist[0]->mesh_diagram_dump(&tu_file(stdout, false), mesh_axis);
	}

	if (do_ray_test && treelist.size())
	{
		test_cast_against_tree(treelist);
	}

	{for (int i = 0; i < treelist.size(); i++)
	{
		delete treelist[i];
	}}

	return 0;
}


void	make_kd_trees(array<kd_tree_dynamic*>* treelist, const char* filename)
// Build a list of kd-trees from the specified text file.  Format is:
//
// tridata
// verts: 1136
// tris: 2144
// 123.456 789.012 345.678
// .... (rest of verts)
// 1 2 3 4 5
// .... (rest of faces)
//
// Faces are 3 vertex indices, a surface type int, and a face-flags
// int.
{
	array<vec3>	verts;
	array<int>	indices;

	FILE*	in = fopen(filename, "r");
	if (in == NULL)
	{
		printf("can't open '%s'\n", filename);
	}

	static const int LINE_MAX = 1000;
	char line[LINE_MAX];

	fgets(line, LINE_MAX, in);
	if (strncmp(line, "tridata", strlen("tridata")) != 0)
	{
		printf("file '%s' does not appear to be tridata\n");
	}

	int	vert_count = 0;
	int	tri_count = 0;

	fgets(line, LINE_MAX, in);
	if (sscanf(line, "verts: %d", &vert_count) != 1)
	{
		printf("can't read vert count\n");
	}

// 	// Can't handle big meshes; TODO split the meshes and make a
// 	// forest of kd_trees in this case.
// 	if (vert_count >= 65536)
// 	{
// 		printf("too many verts; must be <64K\n");
// 		return NULL;
// 	}
	
	fgets(line, LINE_MAX, in);
	if (sscanf(line, "tris: %d", &tri_count) != 1)
	{
		printf("can't read tri count\n");
	}

	if (vert_count <= 0)
	{
		printf("invalid number of verts: %d\n", vert_count);
	}

	// Read verts.
	for (int i = 0; i < vert_count; i++)
	{
		vec3	v;

		fgets(line, LINE_MAX, in);
		if (sscanf(line, "%f %f %f", &v.x, &v.y, &v.z) != 3)
		{
			printf("error reading vert at vertex index %d\n", i);
		}
		
		verts.push_back(v);
	}

	// Read triangles.
	{for (int i = 0; i < tri_count; i++)
	{
		int	vi0, vi1, vi2;

		fgets(line, LINE_MAX, in);
		if (sscanf(line, "%d %d %d", &vi0, &vi1, &vi2) != 3)
		{
			printf("error reading triangle verts at triangle index %d\n", i);
		}
		
		if (vi0 < 0 || vi0 >= vert_count
			|| vi1 < 0 || vi1 >= vert_count
			|| vi2 < 0 || vi2 >= vert_count)
		{
			printf("invalid triangle verts at triangle %d, verts are %d %d %d\n",
				   i, vi0, vi1, vi2);
		}

		indices.push_back(vi0);
		indices.push_back(vi1);
		indices.push_back(vi2);
	}}

	assert(indices.size() == tri_count * 3);

	// Done.
	fclose(in);

	// Make the kd-tree.
	kd_tree_dynamic::build_trees(treelist, vert_count, &verts[0], tri_count, &indices[0]);
//	kd_tree_dynamic*	tree = new kd_tree_dynamic(vert_count, &verts[0], tri_count, &indices[0]);
//	return tree;
}


void	test_cast_against_tree(const array<kd_tree_dynamic*>& treelist)
// Shoot a ton of random rays against the kdtrees.
{
	assert(treelist.size() > 0);

	static const int	RAY_COUNT = 100000;

	printf("building kd_tree_packed...\n");

	uint64	start_build_ticks = tu_timer::get_profile_ticks();

	// Make a list of packed trees, and get an overall bound.
	array<kd_tree_packed*>	kds;
	axial_box	bound(axial_box::INVALID, vec3::flt_max, vec3::minus_flt_max);
	for (int i = 0; i < treelist.size(); i++)
	{
		kd_tree_packed*	kd = kd_tree_packed::build(treelist[i]);
		kds.push_back(kd);

		bound.set_enclosing(kd->get_bound());
	}

	uint64	end_build_ticks = tu_timer::get_profile_ticks();

	printf("built %d trees in %3.3f seconds\n",
	       kds.size(),
	       tu_timer::profile_ticks_to_seconds(end_build_ticks - start_build_ticks));

	printf("starting to cast...\n");

	uint64	start_cast_ticks = tu_timer::get_profile_ticks();

	for (int i = 0; i < RAY_COUNT; i++)
	{
		vec3	start = bound.get_random_point();
		vec3	end = bound.get_random_point();

		// Avoid very short ray tests.
		while ((start - end).sqrmag() < 1e-3f)
		{
			end = bound.get_random_point();
		}

		ray_query	ray(ray_query::start_end, start, end);

		bool	result = false;
		for (int ti = 0, tn = kds.size(); ti < tn; ti++)
		{
			result = kds[ti]->ray_test(ray);
			if (result) break;	// early out on hit
		}
	}

	uint64	end_ticks = tu_timer::get_profile_ticks();
	double	seconds = tu_timer::profile_ticks_to_seconds(end_ticks - start_cast_ticks);
	double	secs_per_ray = seconds / RAY_COUNT;
	printf("%d ray casts took %3.3f seconds, %3.3f micros/ray\n", RAY_COUNT, seconds, secs_per_ray * 1000000);
	printf("tests: %d nodes, %d leaves, %d faces\n",
	       kd_tree_packed::s_ray_test_node_count,
	       kd_tree_packed::s_ray_test_leaf_count,
	       kd_tree_packed::s_ray_test_face_count);
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

