// image_filters.cpp	-- Original code by Dale Schumacher, public domain 1991

// See _Graphics Gems III_ "General Filtered Image Rescaling", Dale A. Schumacher

// Modifications by Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A series of image rescaling functions.  tulrich: Mostly I just
// converted from K&R C to C-like C++, changed the interfaces a bit,
// etc.


#include "engine/image_filters.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <SDL/SDL.h>


Pixel
get_pixel(image, x, y)
Image *image;
int x, y;
{
	static Image *im = NULL;
	static int yy = -1;
	static Pixel *p = NULL;

	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) {
		return(0);
	}
	if((im != image) || (yy != y)) {
		im = image;
		yy = y;
		p = image->data + (y * image->span);
	}
	return(p[x]);
}

void
get_row(row, image, y)
Pixel *row;
Image *image;
int y;
{
	if((y < 0) || (y >= image->ysize)) {
		return;
	}
	memcpy(row,
		image->data + (y * image->span),
		(sizeof(Pixel) * image->xsize));
}

void
get_column(column, image, x)
Pixel *column;
Image *image;
int x;
{
	int i, d;
	Pixel *p;

	if((x < 0) || (x >= image->xsize)) {
		return;
	}
	d = image->span;
	for(i = image->ysize, p = image->data + x; i-- > 0; p += d) {
		*column++ = *p;
	}
}

Pixel
put_pixel(image, x, y, data)
Image *image;
int x, y;
Pixel data;
{
	static Image *im = NULL;
	static int yy = -1;
	static Pixel *p = NULL;

	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) {
		return(0);
	}
	if((im != image) || (yy != y)) {
		im = image;
		yy = y;
		p = image->data + (y * image->span);
	}
	return(p[x] = data);
}


/*
 *	filter function definitions
 */

#define	filter_support		(1.0)

float	filter(float t)
// tulrich: looks like Hermite basis.
{
	/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
	if(t < 0.0f) t = -t;
	if(t < 1.0f) return((2.0f * t - 3.0f) * t * t + 1.0f);
	return(0.0f);
}

#define	box_support		(0.5)

float	box_filter(float t)
{
	if((t > -0.5) && (t <= 0.5)) return(1.0);
	return(0.0);
}

#define	triangle_support	(1.0)

float	triangle_filter(float t)
{
	if(t < 0.0) t = -t;
	if(t < 1.0) return(1.0 - t);
	return(0.0);
}

#define	bell_support		(1.5)

float	bell_filter(float t)
/* box (*) box (*) box */
{
	if(t < 0) t = -t;
	if(t < .5) return(.75 - (t * t));
	if(t < 1.5) {
		t = (t - 1.5);
		return(.5 * (t * t));
	}
	return(0.0);
}

#define	B_spline_support	(2.0f)

float	B_spline_filter(float t)
/* box (*) box (*) box (*) box */
{
	double tt;

	if(t < 0) t = -t;
	if(t < 1) {
		tt = t * t;
		return((.5f * tt * t) - tt + (2.0f / 3.0f));
	} else if(t < 2) {
		t = 2 - t;
		return((1.0f / 6.0f) * (t * t * t));
	}
	return(0.0);
}

float	sinc(float x)
{
	x *= M_PI;
	if(x != 0) return(sinf(x) / x);
	return(1.0f);
}

#define	Lanczos3_support	(3.0f)

float	Lanczos3_filter(float t)
{
	if(t < 0) t = -t;
	if(t < 3.0f) return(sinc(t) * sinc(t/3.0f));
	return(0.0f);
}

#define	Mitchell_support	(2.0)

#define	B	(1.0 / 3.0)
#define	C	(1.0 / 3.0)

float	Mitchell_filter(float t)
{
	float tt;

	tt = t * t;
	if(t < 0) t = -t;
	if(t < 1.0f) {
		t = (((12.0f - 9.0f * B - 6.0f * C) * (t * tt))
		   + ((-18.0f + 12.0f * B + 6.0f * C) * tt)
		   + (6.0f - 2 * B));
		return(t / 6.0f);
	} else if(t < 2.0f) {
		t = (((-1.0f * B - 6.0f * C) * (t * tt))
		   + ((6.0f * B + 30.0f * C) * tt)
		   + ((-12.0f * B - 48.0f * C) * t)
		   + (8.0f * B + 24 * C));
		return(t / 6.0f);
	}
	return(0.0f);
}

/*
 *	image rescaling routine
 */

struct CONTRIB {
	int	pixel;
	float	weight;

	CONTRIB()
		: pixel(0), weight(0.f)
	{
	}

	CONTRIB(int p, float w)
		: pixel(p), weight(w)
	{
	}
};

struct CLIST {
	int	n;		/* number of contributors */
	CONTRIB	*p;		/* pointer to list of contributions */
};

//CLIST	*contrib;		/* array of contribution lists */
array< array<CLIST> >	contrib;

void	zoom(Image* dst, Image* src, float (*filterf)(float t), float fwidth)
/* dst --> destination image structure */
/* src --> source image structure */
/* filterf --> filter function */
/* fwidth --> filter width (support) */
{
	Image *tmp;			/* intermediate image */
	float	xscale, yscale;		/* zoom scale factors */
	int i, j, k;			/* loop variables */
	int n;				/* pixel number */
	float center, left, right;	/* filter calculation variables */
	float width, fscale, weight;	/* filter calculation variables */
	Pixel *raster;			/* a row or column of pixels */

	/* create intermediate image to hold horizontal zoom */
//	tmp = new_image(dst->xsize, src->ysize);
	tmp = image::create_rgb(dst->w, dst->h);
	xscale = (float) dst->w / (float) src->w;
	yscale = (float) dst->h / (float) src->h;

	/* pre-calculate filter contributions for a row */
//	contrib = (CLIST *)calloc(dst->xsize, sizeof(CLIST));
	contrib.resize(dst->w);
	if(xscale < 1.0f) {
		width = fwidth / xscale;
		fscale = 1.0f / xscale;
		for (i = 0; i < dst->w; ++i) {
//			contrib[i].n = 0;
//			contrib[i].p = (CONTRIB *)calloc((int) (width * 2 + 1),
//					sizeof(CONTRIB));
			contrib[i].resize(0);

			center = (float) i / xscale;
			left = ceilf(center - width);
			right = floorf(center + width);
			for (j = left; j <= right; ++j) {
				weight = center - (float) j;
				weight = (*filterf)(weight / fscale) / fscale;
				if(j < 0) {
					n = -j;
				} else if(j >= src->w) {
					n = (src->w - j) + src->w - 1;
				} else {
					n = j;
				}
//				k = contrib[i].n++;
//				contrib[i].p[k].pixel = n;
//				contrib[i].p[k].weight = weight;
				contrib[i].push_back(CONTRIB(n, weight));
			}
		}
	} else {
		for (i = 0; i < dst->xsize; ++i) {
//			contrib[i].n = 0;
//			contrib[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
//					sizeof(CONTRIB));
			contrib[i].resize(0);
			center = (float) i / xscale;
			left = ceilf(center - fwidth);
			right = floorf(center + fwidth);
			for(j = left; j <= right; ++j) {
				weight = center - (float) j;
				weight = (*filterf)(weight);
				if(j < 0) {
					n = -j;
				} else if(j >= src->xsize) {
					n = (src->xsize - j) + src->xsize - 1;
				} else {
					n = j;
				}
//				k = contrib[i].n++;
//				contrib[i].p[k].pixel = n;
//				contrib[i].p[k].weight = weight;
				contrib[i].push_back(CONTRIB(n, weight));
			}
		}
	}

	/* apply filter to zoom horizontally from src to tmp */
	raster = (Pixel *)calloc(src->w, sizeof(Pixel));
	for (k = 0; k < tmp->h; ++k) {
		get_row(raster, src, k);
		for (i = 0; i < tmp->w; ++i) {
			float	red = 0.0f;
			float	green = 0.0f;
			float	blue = 0.0f;
			for(j = 0; j < contrib[i].size(); ++j) {
				red += raster[contrib[i].p[j].pixel].r
					* contrib[i].p[j].weight;
				green += raster[contrib[i].p[j].pixel].g
					* contrib[i].p[j].weight;
				blue += raster[contrib[i].p[j].pixel].b
					* contrib[i].p[j].weight;
			}
			red = fclamp(red, 0, 255);
			green = fclamp(green, 0, 255);
			blue = fclamp(blue, 0, 255);
			image::set_pixel(tmp, i, k, red, green, blue);
		}
	}
	free(raster);

//	/* free the memory allocated for horizontal filter weights */
//	for(i = 0; i < tmp->xsize; ++i) {
//		free(contrib[i].p);
//	}
//	free(contrib);

	/* pre-calculate filter contributions for a column */
//	contrib = (CLIST *)calloc(dst->ysize, sizeof(CLIST));
	contrib.resize(dst->h);
	if(yscale < 1.0) {
		width = fwidth / yscale;
		fscale = 1.0f / yscale;
		for (i = 0; i < dst->w; ++i) {
//			contrib[i].n = 0;
//			contrib[i].p = (CONTRIB *)calloc((int) (width * 2 + 1),
//					sizeof(CONTRIB));
			contrib[i].resize(0);

			center = (float) i / yscale;
			left = ceilf(center - width);
			right = floorf(center + width);
			for (j = left; j <= right; ++j) {
				weight = center - (float) j;
				weight = (*filterf)(weight / fscale) / fscale;
				if(j < 0) {
					n = -j;
				} else if(j >= tmp->h) {
					n = (tmp->h - j) + tmp->h - 1;
				} else {
					n = j;
				}
//				k = contrib[i].n++;
//				contrib[i].p[k].pixel = n;
//				contrib[i].p[k].weight = weight;
				contrib[i].push_back(CONTRIB(n, weight));
			}
		}
	} else {
		for (i = 0; i < dst->ysize; ++i) {
//			contrib[i].n = 0;
//			contrib[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
//					sizeof(CONTRIB));
			contrib[i].resize(0);
			center = (float) i / yscale;
			left = ceilf(center - fwidth);
			right = floorf(center + fwidth);
			for(j = left; j <= right; ++j) {
				weight = center - (float) j;
				weight = (*filterf)(weight);
				if(j < 0) {
					n = -j;
				} else if(j >= tmp->h) {
					n = (tmp-> - j) + tmp->h - 1;
				} else {
					n = j;
				}
//				k = contrib[i].n++;
//				contrib[i].p[k].pixel = n;
//				contrib[i].p[k].weight = weight;
				contrib[i].push_back(CONTRIB(n, weight));
			}
		}
	}

	/* apply filter to zoom vertically from tmp to dst */
	raster = (Pixel *)calloc(tmp->h, sizeof(Pixel));
	for (k = 0; k < dst->xsize; ++k) {
		get_column(raster, tmp, k);
		for (i = 0; i < dst->h; ++i) {
			float	red = 0.0f;
			float	green = 0.0f;
			float	blue = 0.0f;
			for(j = 0; j < contrib[i].size(); ++j) {
//				weight += raster[contrib[i].p[j].pixel]
//					* contrib[i].p[j].weight;
				red += raster[contrib[i].p[j].pixel].r
					* contrib[i].p[j].weight;
				green += raster[contrib[i].p[j].pixel].g
					* contrib[i].p[j].weight;
				blue += raster[contrib[i].p[j].pixel].b
					* contrib[i].p[j].weight;
			}
			red = fclamp(red, 0, 255);
			green = fclamp(green, 0, 255);
			blue = fclamp(blue, 0, 255);
			image::set_pixel(tmp, i, k, red, green, blue);
		}
	}
	free(raster);

//	/* free the memory allocated for vertical filter weights */
//	for(i = 0; i < dst->ysize; ++i) {
//		free(contrib[i].p);
//	}
//	free(contrib);
	contrib.resize(0);

//	free_image(tmp);
	SDL_ImageFree(tmp);
}



#if 0

/*
 *	command line interface
 */

void usage()
{
	fprintf(stderr, "usage: %s [-options] input.bm output.bm\n", _Program);
	fprintf(stderr, "\
options:\n\
	-x xsize		output x size\n\
	-y ysize		output y size\n\
	-f filter		filter type\n\
{b=box, t=triangle, q=bell, B=B-spline, h=hermite, l=Lanczos3, m=Mitchell}\n\
");
	exit(1);
}

void
banner()
{
	printf("%s v%s -- %s\n", _Program, _Version, _Copyright);
}

main(argc, argv)
int argc;
char *argv[];
{
	register int c;
	extern int optind;
	extern char *optarg;
	int xsize = 0, ysize = 0;
	double (*f)() = filter;
	double s = filter_support;
	char *dstfile, *srcfile;
	Image *dst, *src;
	FILE *fp;

	while((c = getopt(argc, argv, "x:y:f:V")) != EOF) {
		switch(c) {
		case 'x': xsize = atoi(optarg); break;
		case 'y': ysize = atoi(optarg); break;
		case 'f':
			switch(*optarg) {
			case 'b': f=box_filter; s=box_support; break;
			case 't': f=triangle_filter; s=triangle_support; break;
			case 'q': f=bell_filter; s=bell_support; break;
			case 'B': f=B_spline_filter; s=B_spline_support; break;
			case 'h': f=filter; s=filter_support; break;
			case 'l': f=Lanczos3_filter; s=Lanczos3_support; break;
			case 'm': f=Mitchell_filter; s=Mitchell_support; break;
			default: usage();
			}
			break;
		case 'V': banner(); exit(EXIT_SUCCESS);
		case '?': usage();
		default:  usage();
		}
	}
	if((argc - optind) != 2) usage();
	srcfile = argv[optind];
	dstfile = argv[optind + 1];
	if(((fp = fopen(srcfile, "r")) == NULL)
	|| ((src = load_image(fp)) == NULL)) {
		fprintf(stderr, "%s: can't load source image '%s'\n",
			_Program, srcfile);
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	if(xsize <= 0) xsize = src->xsize;
	if(ysize <= 0) ysize = src->ysize;
	dst = new_image(xsize, ysize);
	zoom(dst, src, f, s);
	if(((fp = fopen(dstfile, "w")) == NULL)
	|| (save_image(fp, dst) != 0)) {
		fprintf(stderr, "%s: can't save destination image '%s'\n",
			_Program, dstfile);
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	exit(EXIT_SUCCESS);
}
#endif // 0





// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
