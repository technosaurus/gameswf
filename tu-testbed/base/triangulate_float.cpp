// triangulate_float.cpp	-- Thatcher Ulrich 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to triangulate arbitrary 2D polygonal regions.
//
// Instantiate our templated algo from triangulate_inst.h

#include "base/triangulate_impl.h"


namespace triangulate
{
	// Version using float coords
	void	compute(
		array<float>* result,	// trilist
		int path_count,
		const array<float> paths[])
	{
		compute_triangulation<float>(result, path_count, paths);
	}
}



#ifdef TEST_TRIANGULATE_FLOAT

// Compile test with something like:
//
// gcc -o triangulate_test -I../ triangulate_float.cpp tu_random.cpp -DTEST_TRIANGULATE_FLOAT -lstdc++
//
// or
//
// cl -Od -Zi -o triangulate_test.exe -I../ triangulate_float.cpp tu_random.cpp -DTEST_TRIANGULATE_FLOAT

int	main()
{
	// test linkage.
	array<float>	result;
	array<array<float> >	paths;

	paths.resize(1);
	paths[0].push_back(0);
	paths[0].push_back(0);
	paths[0].push_back(1);
	paths[0].push_back(0);
	paths[0].push_back(1);
	paths[0].push_back(1);
	paths[0].push_back(0);
	paths[0].push_back(1);

	triangulate::compute(&result, paths.size(), &paths[0]);

	for (int i = 0; i < result.size(); i++)
	{
		printf("%f\n", result[i]);
	}

	return 0;
}


#endif // TEST_TRIANGULATE_FLOAT
