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


static kd_tree_dynamic*	make_kd_tree(const char* filename);
static void	test_cast_against_tree(const kd_tree_dynamic* tree);


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
	kd_tree_dynamic*	tree = make_kd_tree(infile);
	uint64	end_ticks = tu_timer::get_profile_ticks();
	if (do_ray_test)
	{
		// Print timings for building.
		printf("read file and built kd_tree_dynamic in %3.3f seconds\n",
		       tu_timer::profile_ticks_to_seconds(end_ticks - start_ticks));
	}

	if (do_dump && tree)
	{
		tree->diagram_dump(&tu_file(stdout, false));
	}

	if (do_mesh_dump && tree)
	{
		tree->mesh_diagram_dump(&tu_file(stdout, false), mesh_axis);
	}

	if (do_ray_test && tree)
	{
		test_cast_against_tree(tree);
	}

	delete tree;

	return 0;
}


kd_tree_dynamic*	make_kd_tree(const char* filename)
// Build a kd-tree from the specified text file.
// Format is:
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
		return NULL;
	}

	static const int LINE_MAX = 1000;
	char line[LINE_MAX];

	fgets(line, LINE_MAX, in);
	if (strncmp(line, "tridata", strlen("tridata")) != 0)
	{
		printf("file '%s' does not appear to be tridata\n");
		return NULL;
	}

	int	vert_count = 0;
	int	tri_count = 0;

	fgets(line, LINE_MAX, in);
	if (sscanf(line, "verts: %d", &vert_count) != 1)
	{
		printf("can't read vert count\n");
		return NULL;
	}

	// Can't handle big meshes; TODO split the meshes and make a
	// forest of kd_trees in this case.
	if (vert_count >= 65536)
	{
		printf("too many verts; must be <64K\n");
		return NULL;
	}
	
	fgets(line, LINE_MAX, in);
	if (sscanf(line, "tris: %d", &tri_count) != 1)
	{
		printf("can't read tri count\n");
		return NULL;
	}

	if (vert_count <= 0)
	{
		printf("invalid number of verts: %d\n", vert_count);
		return NULL;
	}

	// Read verts.
	for (int i = 0; i < vert_count; i++)
	{
		vec3	v;

		fgets(line, LINE_MAX, in);
		if (sscanf(line, "%f %f %f", &v.x, &v.y, &v.z) != 3)
		{
			printf("error reading vert at vertex index %d\n", i);
			return NULL;
		}
		
		verts.push_back(v);
	}

	// Read triangles.
	{for (int i = 0; i < tri_count; i++)
	{
		int	vi0, vi1, vi2, surface_id, flags;

		fgets(line, LINE_MAX, in);
		if (sscanf(line, "%d %d %d %d %d", &vi0, &vi1, &vi2, &surface_id, &flags) != 5)
		{
			printf("error reading triangle verts & flags at triangle index %d\n", i);
			return NULL;
		}
		
		if (vi0 < 0 || vi0 >= vert_count
			|| vi1 < 0 || vi1 >= vert_count
			|| vi2 < 0 || vi2 >= vert_count)
		{
			printf("invalid triangle verts at triangle %d, verts are %d %d %d\n",
				   i, vi0, vi1, vi2);
			return NULL;
		}

		indices.push_back(vi0);
		indices.push_back(vi1);
		indices.push_back(vi2);
	}}

	assert(indices.size() == tri_count * 3);

	// Done.
	fclose(in);

	// Make the kd-tree.
	kd_tree_dynamic*	tree = new kd_tree_dynamic(vert_count, &verts[0], tri_count, &indices[0]);

	return tree;
}


void	test_cast_against_tree(const kd_tree_dynamic* tree)
// Shoot a ton of random rays against the kdtree.
{
	static const int	RAY_COUNT = 100000;

	printf("building kd_tree_packed...\n");

	uint64	start_build_ticks = tu_timer::get_profile_ticks();

	// Make a packed tree.
	kd_tree_packed*	kd = kd_tree_packed::build(tree);

	uint64	end_build_ticks = tu_timer::get_profile_ticks();

	printf("built in %3.3f seconds\n", tu_timer::profile_ticks_to_seconds(end_build_ticks - start_build_ticks));

	const axial_box&	bound = kd->get_bound();

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

		// xxx debugging: force all rays parallel to some axis...
		do
		{
			end = bound.get_random_point();
			start.y = end.y;
			start.z = end.z;
		}
		while ((start - end).sqrmag() < 1e-3f);
		// xxx

		ray_query	ray(ray_query::start_end, start, end);

		bool	result = kd->ray_test(ray);
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

