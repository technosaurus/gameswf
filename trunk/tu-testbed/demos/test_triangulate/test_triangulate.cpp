// test_triangulate.cpp	-- Thatcher Ulrich <http://tulrich.com> 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Test program for tu_testbed triangulation code.


#include "base/triangulate.h"
#include "base/postscript.h"
#include "base/tu_file.h"
#include <float.h>


void	compute_bounding_box(float* x0, float* y0, float* x1, float* y1, const array<float>& coord_list)
// Compute bounding box around given coord_list of (x,y) pairs.
{
	assert((coord_list.size() & 1) == 0);

	*x0 = *y0 = FLT_MAX;
	*x1 = *y1 = -FLT_MAX;

	for (int i = 0; i < coord_list.size(); )
	{
		if (coord_list[i] > *x1) *x1 = coord_list[i];
		if (coord_list[i] < *x0) *x0 = coord_list[i];
		i++;
		if (coord_list[i] > *y1) *y1 = coord_list[i];
		if (coord_list[i] < *y0) *y0 = coord_list[i];
		i++;
	}
}


void	output_diagram(array<float>& trilist, const array<array<float> >& input_paths)
// Emit a Postscript diagram of the given info.  trilist is the output
// of the triangulation algo; input_paths is the input.
{
	// Bounding box, so we know how to scale the output.
	float	x0, y0, x1, y1;
	compute_bounding_box(&x0, &y0, &x1, &y1, trilist);

	// Compute a uniform scale that makes a nice-sized diagram.
	const float	DIMENSION = 72 * 7.5f;	// 72 Postscript units per inch
	const float	OFFSET = 36;

	float	scale = 1.0f;
	if (x1 - x0 > 0)
	{
		float	xscale = DIMENSION / (x1 - x0);
		scale = xscale;
	}
	if (y1 - y0 > 0)
	{
		float	yscale = DIMENSION / (y1 - y0);
		if (yscale < scale)
		{
			scale = yscale;
		}
	}

	// Setup postscript output.
	tu_file	out("output.ps", "wb");
	if (out.get_error())
	{
		fprintf(stderr, "can't open output.ps, error = %d\n", out.get_error());
		exit(1);
	}

	postscript	ps(&out, "Triangulation test diagram", true /* encapsulated */);

	int	tricount = trilist.size() / 6;
	int	trilabel = 0;

#define DRAW_AS_MESH
//#define DRAW_AS_POLY

#ifdef DRAW_AS_MESH
	for (int i = 0; i < trilist.size(); )
	{
		float	x[3], y[3];
		x[0] = trilist[i++];
		y[0] = trilist[i++];
		x[1] = trilist[i++];
		y[1] = trilist[i++];
		x[2] = trilist[i++];
		y[2] = trilist[i++];

		#define TRANSFORM(x, y)  (((x) - x0) * scale) + OFFSET, (((y) - y0) * scale) + OFFSET

//#define WIREFRAME
#ifdef WIREFRAME
		ps.gray(0);
		ps.line(TRANSFORM(x[0], y[0]), TRANSFORM(x[1], y[1]));
		ps.line(TRANSFORM(x[1], y[1]), TRANSFORM(x[2], y[2]));
		ps.line(TRANSFORM(x[2], y[2]), TRANSFORM(x[0], y[0]));
#else	// not WIREFRAME
		ps.gray(((i/6) / float(tricount) + 0.25f) * 0.5f);
		ps.moveto(TRANSFORM(x[0], y[0]));
		ps.lineto(TRANSFORM(x[1], y[1]));
		ps.lineto(TRANSFORM(x[2], y[2]));
		ps.fill();
#endif // not WIREFRAME

		ps.disk(TRANSFORM(x[1], y[1]), 2);	// this should be the apex of the original ear

#if 0
		// Label the tri.
		ps.gray(0.9f);
		float	cent_x = (x[0] + x[1] + x[2]) / 3;
		float	cent_y = (y[0] + y[1] + y[2]) / 3;
		ps.printf(TRANSFORM(cent_x, cent_y), "%d", trilabel);
		trilabel++;

		ps.line(TRANSFORM(cent_x, cent_y), TRANSFORM(x[1], y[1]));	// line to the ear
#endif // 0

	}
#endif // DRAW_AS_MESH

#ifdef DRAW_AS_POLY
	ps.gray(0);
	// xxxx debug only, draw the coords as if they form a poly.
	for (int i = 0; i < trilist.size() - 2; i += 2)
	{
		#define TRANSFORM(x, y)  (((x) - x0) * scale) + OFFSET, (((y) - y0) * scale) + OFFSET
		float	xv0 = trilist[i];
		float	yv0 = trilist[i + 1];
		float	xv1 = trilist[i + 2];
		float	yv1 = trilist[i + 3];
		ps.line(TRANSFORM(xv0, yv0), TRANSFORM(xv1, yv1));
	}
#endif // DRAW_AS_POLY
}


void	make_star(array<float>* out, float inner_radius, float outer_radius, int points)
// Generate a star-shaped polygon.  Point coordinates go into *out.
{
	assert(points > 2);
	assert(out);

	for (int i = 0; i < points; i++)
	{
		float	angle = (i * 2) / float(points * 2) * 2 * float(M_PI);
		out->push_back(cosf(angle) * outer_radius);
		out->push_back(sinf(angle) * outer_radius);

		angle = (i * 2 + 1) / float(points * 2) * 2 * float(M_PI);
		out->push_back(cosf(angle) * inner_radius);
		out->push_back(sinf(angle) * inner_radius);
	}
}


void	make_square(array<float>* out, float size)
// Generate a square with the given side size.
{
	assert(out);

	float	radius = size / 2;

	out->push_back(-radius);
	out->push_back(-radius);
	out->push_back( radius);
	out->push_back(-radius);
	out->push_back( radius);
	out->push_back( radius);
	out->push_back(-radius);
	out->push_back( radius);
}


void	make_spiral(array<float>* out, float width, float radians)
// Generate a spiral.
{
	assert(out);

	float	angle = 1;
	
	while (angle < radians)
	{
		float	radius = width * angle / 2 / float(M_PI);

		out->push_back(radius * cosf(angle));
		out->push_back(radius * sinf(angle));

		if (radius < 6)
		{
			angle += 0.2f;
		}
		else
		{
			float	step = 1.0f / radius;
			angle += step;
		}
	}

	// spiral back in towards the center.
	for (int i = 0, n = out->size(); i < n; i += 2)
	{
		float	x = (*out)[(n - 2) - i];
		float	y = (*out)[(n - 2) - i + 1];

		float	dist = sqrt(x * x + y * y);
		float	thickness = width / 5;
		if (dist > thickness)
		{
			out->push_back(x * (dist - width / 5) / dist);
			out->push_back(y * (dist - width / 5) / dist);
		}
		else
		{
			out->push_back(x * 0.8f);
			out->push_back(y * 0.8f);
		}
	}
}


void	reverse_path(array<float>* path)
// Reverse the order of the path coords (i.e. for turning a poly into
// an island).
{
	assert((path->size() & 1) == 0);

	int	coord_ct = path->size() >> 1;
	int	ct = coord_ct >> 1;

	for (int i = 0; i < ct * 2; i += 2)
	{
		swap(&(*path)[i * 2], &(*path)[(coord_ct - 1 - i) * 2]);	// x coord
		swap(&(*path)[i * 2 + 1], &(*path)[(coord_ct - 1 - i) * 2 + 1]);	// y coord
	}
}


void	offset_path(array<float>* path, float x, float y)
// Offset the given path coords.
{
	assert((path->size() & 1) == 0);

	for (int i = 0; i < path->size(); i += 2)
	{
		(*path)[i] += x;
		(*path)[i + 1] += y;
	}
}


void	rotate_coord_order(array<float>* path, int rotate_count)
// Reorder the verts, but preserve the exact same poly shape.
//
// rotate_count specifies how 
{
	assert(rotate_count >= 0);
	assert((path->size() & 1) == 0);

	if (path->size() == 0)
	{
		return;
	}

	// Stupid impl...
	while (rotate_count-- > 0)
	{
		// shift coordinates down, rotate first coord to end of array.
		float	tempx = (*path)[0];
		float	tempy = (*path)[1];

		int	i;
		for (i = 0; i < path->size() - 2; i += 2)
		{
			(*path)[i] = (*path)[i + 2];
			(*path)[i + 1] = (*path)[i + 3];
		}
		(*path)[i] = tempx;
		(*path)[i + 1] = tempy;
	}
}


int	main()
{
	array<float>	result;
	array<array<float> >	paths;

#if 0
	// Make a square.
	paths.resize(paths.size() + 1);
	make_square(&paths.back(), 1100);

	// Another square.
	paths.resize(paths.size() + 1);
	make_square(&paths.back(), 1100);
	offset_path(&paths.back(), 1200, 100);
#endif

#if 0
	// Make a square.
	paths.resize(paths.size() + 1);
	make_square(&paths.back(), 1100);

	// Make a little square inside.
	paths.resize(paths.size() + 1);
	make_square(&paths.back(), 100);
	reverse_path(&paths.back());

	// Make a little star island.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 50, 250, 3);
	offset_path(&paths.back(), -300, -300);
	reverse_path(&paths.back());

	// Make a circle.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 200, 200, 20);
	offset_path(&paths.back(), 300, 300);
	reverse_path(&paths.back());

	// Make a circle inside the circle.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 50, 50, 5);
	offset_path(&paths.back(), 300, 300);

	// Make a circle outside the big square.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 50, 50, 8);
	offset_path(&paths.back(), 1200, 300);
#endif

#if 0
	// Lots of circles.

	// @@ set this to 100 for a good performance torture test of bridge-finding.
	const int	TEST_DIM = 20;
	{for (int x = 0; x < TEST_DIM; x++)
	{
		for (int y = 0; y < TEST_DIM; y++)
		{
			paths.resize(paths.size() + 1);
			make_star(&paths.back(), 9, 9, 10);
			offset_path(&paths.back(), float(x) * 20, float(y) * 20);
		}
	}}
#endif

#if 0
	// Lots of concentric circles.
	static int	CIRCLE_COUNT = 20;	// CIRCLE_COUNT >= 10 is a good performance test.
	{for (int i = 0; i < CIRCLE_COUNT * 2 + 1; i++)
	{
		paths.resize(paths.size() + 1);
		make_star(&paths.back(), 2 + float(i), 2 + float(i), 10 + i * 6);
		if (i & 1) reverse_path(&paths.back());
	}}
#endif

#if 0
	// test some degenerates.
	paths.resize(paths.size() + 1);
	paths.back().push_back(0);
	paths.back().push_back(0);
	paths.back().push_back(100);
	paths.back().push_back(-50);
	paths.back().push_back(120);
	paths.back().push_back(0);
	paths.back().push_back(100);
	paths.back().push_back(50);
	paths.back().push_back(0);
	paths.back().push_back(0);
	paths.back().push_back(-100);
	paths.back().push_back(-100);
	rotate_coord_order(&paths.back(), 5);
#endif

#if 0
	// Make a star.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 2300, 3000, 20);

	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 1100, 2200, 20);
	reverse_path(&paths.back());

	paths.resize(paths.size() + 1);
	make_star(&paths.back(),  800, 1800, 20);

	// Make a star island.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 100, 500, 3);
	reverse_path(&paths.back());
#endif

#if 0
	// @@ TODO this one has a gnarly twist that puts us into the
	// recovery mode.  See "Case A" in recovery_process() in
	// triangulate_imp.h.

	// Stars with touching verts on different paths.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 2300, 3000, 20);

	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 2300, 1100, 20);
	reverse_path(&paths.back());

 	paths.resize(paths.size() + 1);
 	make_star(&paths.back(), 800, 1100, 20);

	// Make a star island.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 100, 300, 3);
	reverse_path(&paths.back());
#endif

#if 1
	// A direct, simplified expression of the "gnarly twist".
	paths.resize(paths.size() + 1);

	paths.back().push_back(0);
	paths.back().push_back(0);

	paths.back().push_back(3);
	paths.back().push_back(0);

	paths.back().push_back(3);
	paths.back().push_back(2);

	paths.back().push_back(0);
	paths.back().push_back(2);

	paths.back().push_back(0);
	paths.back().push_back(3);

	paths.back().push_back(2);
	paths.back().push_back(3);

	paths.back().push_back(2);
	paths.back().push_back(2);

	paths.back().push_back(2.5f);
	paths.back().push_back(0.5f);

	paths.back().push_back(1.5f);
	paths.back().push_back(0.5f);

	paths.back().push_back(2);
	paths.back().push_back(2);

	paths.back().push_back(2);
	paths.back().push_back(3);

	paths.back().push_back(0);
	paths.back().push_back(3);

	paths.back().push_back(0);
	paths.back().push_back(2);
#endif // 0

#if 0
	// Spiral.
	paths.resize(paths.size() + 1);
	make_spiral(&paths.back(), 10, 20);
#endif

	// Triangulate.
	triangulate::compute(&result, paths.size(), &paths[0]);

	assert((result.size() % 6) == 0);

	// Dump.
	output_diagram(result, paths);
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

