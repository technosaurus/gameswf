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

		// Label the tri.
		ps.gray(0.9f);
		float	cent_x = (x[0] + x[1] + x[2]) / 3;
		float	cent_y = (y[0] + y[1] + y[2]) / 3;
		ps.printf(TRANSFORM(cent_x, cent_y), "%d", trilabel);
		trilabel++;

		ps.line(TRANSFORM(cent_x, cent_y), TRANSFORM(x[1], y[1]));	// line to the ear
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


int	main()
{
	array<float>	result;
	array<array<float> >	paths;

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

// 	// Make a circle inside the circle.
// 	paths.resize(paths.size() + 1);
// 	make_star(&paths.back(), 50, 50, 8);
// 	offset_path(&paths.back(), 300, 300);
#endif

#if 1
	// Make a star.
 	static const int	STAR_POINTS = 6;
 	static const float	INNER_RADIUS = 1000;
 	static const float	OUTER_RADIUS = 3000;
//  	static const int	STAR_POINTS = 5;
//  	static const float	INNER_RADIUS = 1000;
//  	static const float	OUTER_RADIUS = 3000;
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), INNER_RADIUS, OUTER_RADIUS, STAR_POINTS);

	// Make a star island.
	paths.resize(paths.size() + 1);
	make_star(&paths.back(), 100, 500, 3);
	reverse_path(&paths.back());
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

