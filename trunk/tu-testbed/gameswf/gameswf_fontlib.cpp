// gameswf_fontlib.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A module to take care of all of gameswf's loaded fonts.


#ifndef GAMESWF_FONTLIB_CPP


#include "base/container.h"
#include "base/tu_file.h"
#include "gameswf_font.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_styles.h"
#include "gameswf_tesselate.h"
#include "gameswf_render.h"


namespace gameswf
{
namespace fontlib
{
	array<font*>	s_fonts;
	array<bitmap_info*>	s_bitmaps_used;	// keep these so we can delete them during shutdown.

	// Size (in TWIPS) of the box that the glyph should
	// stay within.
	static float	s_rendering_box = 1536.0f;	// this *should* be 1024, but some glyphs in some fonts exceed it!

	// The nominal size of the final antialiased glyphs stored in
	// the texture.  This parameter controls how large the very
	// largest glyphs will be in the texture; most glyphs will be
	// considerably smaller.  This is also the parameter that
	// controls the tradeoff between texture RAM usage and
	// sharpness of large text.
	static const int	GLYPH_FINAL_SIZE = 96;

	static const int	OVERSAMPLE_BITS = 2;
	static const int	OVERSAMPLE_FACTOR = (1 << OVERSAMPLE_BITS);

	// The raw non-antialiased render size for glyphs.
	static const int	GLYPH_RENDER_SIZE = (GLYPH_FINAL_SIZE << OVERSAMPLE_BITS);

	// The dimensions of the textures that the glyphs get packed into.
	static const int	GLYPH_CACHE_TEXTURE_SIZE = 256;

	// How much space to leave around the individual glyph image.
	// This should be at least 1.  The bigger it is, the smoother
	// the boundaries of minified text will be, but the more
	// texture space is wasted.
	const int PAD_PIXELS = 3;

	//
	// State for the glyph packer.
	//

	static Uint8*	s_render_buffer = NULL;
	static matrix	s_render_matrix;

	static Uint8*	s_current_cache_image = NULL;
	static bitmap_info*	s_current_bitmap_info = NULL;

	// Integer-bounded 2D rectangle.
	struct recti
	{
		int	m_x_min, m_x_max, m_y_min, m_y_max;

		recti(int x0 = 0, int x1 = 0, int y0 = 0, int y1 = 0)
			:
			m_x_min(x0),
			m_x_max(x1),
			m_y_min(y0),
			m_y_max(y1)
		{
		}

		bool	is_valid() const
		{
			return m_x_min <= m_x_max
				&& m_y_min <= m_y_max;
		}

		bool	intersects(const recti& r) const
		// Return true if r touches *this.
		{
			if (m_x_min >= r.m_x_max
			    || m_x_max <= r.m_x_min
			    || m_y_min >= r.m_y_max
			    || m_y_max <= r.m_y_min)
			{
				// disjoint.
				return false;
			}
			return true;
		}

		bool	contains(int x, int y) const
		// Return true if (x,y) is inside *this.
		{
			return x >= m_x_min
				&& x < m_x_max
				&& y >= m_y_min
				&& y < m_y_max;
		}
	};
	// Rects already on the texture.
	static array<recti>	s_covered_rects;

	// 2d integer point.
	struct pointi
	{
		int	m_x, m_y;

		pointi(int x = 0, int y = 0)
			:
			m_x(x),
			m_y(y)
		{
		}

		bool	operator<(const pointi& p) const
		// For sorting anchor points.
		{
			return imin(m_x, m_y) < imin(p.m_x, p.m_y);
		}
	};
	// Candidates for upper-left corner of a new rectangle.  Use
	// lower-left and upper-right of previously placed rects.
	static array<pointi>	s_anchor_points;


	static bool	s_saving = false;
	static tu_file*	s_file = NULL;


	static void	ensure_cache_image_available()
	{
		if (s_current_bitmap_info == NULL)
		{
			// Set up a cache.
			s_current_bitmap_info = render::create_bitmap_info_blank();
			s_bitmaps_used.push_back(s_current_bitmap_info);

			if (s_current_cache_image == NULL)
			{
				s_current_cache_image = new Uint8[GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE];
			}
			memset(s_current_cache_image, 0, GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE);

			// Initialize the coverage data.
			s_covered_rects.resize(0);
			s_anchor_points.resize(0);
			s_anchor_points.push_back(pointi(0, 0));	// seed w/ upper-left of texture.
		}
	}


	void	finish_current_texture()
	{
		if (s_current_bitmap_info == NULL)
		{
			return;
		}

#if 0
		//xxxxxx debug hack -- dump image data to a file
		static int	s_seq = 0;
		char buffer[100];
		sprintf(buffer, "dump%02d.ppm", s_seq);
		s_seq++;
		FILE*	fp = fopen(buffer, "wb");
		if (fp)
		{
			fprintf(fp, "P6\n%d %d\n255\n", GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE);
			for (int i = 0; i < GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE; i++)
			{
				fputc(s_current_cache_image[i], fp);
				fputc(s_current_cache_image[i], fp);
				fputc(s_current_cache_image[i], fp);
			}
			fclose(fp);
		}
		//xxxxxx
#endif // 0

		if (s_saving)		// HACK!!!
		{
			int w = GLYPH_CACHE_TEXTURE_SIZE;
			int h = GLYPH_CACHE_TEXTURE_SIZE;

			// save bitmap size
			s_file->write_le16(w);
			s_file->write_le16(h);

			// save bitmap contents
			s_file->write_bytes(s_current_cache_image, w*h);
		}
		else
		{
			render::set_alpha_image(s_current_bitmap_info,
						GLYPH_CACHE_TEXTURE_SIZE,
						GLYPH_CACHE_TEXTURE_SIZE,
						s_current_cache_image);
		}

		s_current_bitmap_info = NULL;
	}


	bool	is_rect_available(const recti& r)
	// Return true if the given rect can be packed into the
	// currently active texture.
	{
		assert(r.is_valid());
		assert(r.m_x_min >= 0);
		assert(r.m_y_min >= 0);

		if (r.m_x_max > GLYPH_CACHE_TEXTURE_SIZE
		    || r.m_y_max > GLYPH_CACHE_TEXTURE_SIZE)
		{
			// Rect overflows the texture bounds.
			return false;
		}

		// Check against existing rects.
		for (int i = 0, n = s_covered_rects.size(); i < n; i++)
		{
			if (r.intersects(s_covered_rects[i]))
			{
				return false;
			}
		}

		// Spot appears to be open.
		return true;
	}


	void	add_cover_rect(const recti& r)
	// Add the given rect to our list.  Eliminate any anchor
	// points that are disqualified by this new rect.
	{
		s_covered_rects.push_back(r);

		for (int i = 0; i < s_anchor_points.size(); i++)
		{
			const pointi&	p = s_anchor_points[i];
			if (r.contains(p.m_x, p.m_y))
			{
				// Eliminate this point from consideration.
				s_anchor_points.remove(i);
				i--;
			}
		}
	}


	void	add_anchor_point(const pointi& p)
	// Add point to our list of anchors.  Keep the list sorted.
	{
		// Add it to end, since we expect new points to be
		// relatively greater than existing points.
		s_anchor_points.push_back(p);

		// Insertion sort -- bubble down into correct spot.
		for (int i = s_anchor_points.size() - 2; i >= 0; i--)
		{
			if (s_anchor_points[i + 1] < s_anchor_points[i])
			{
				swap(&(s_anchor_points[i]), &(s_anchor_points[i + 1]));
			}
			else
			{
				// Done bubbling down.
				break;
			}
		}
	}


	bool	pack_rectangle(int* px, int* py, int width, int height)
	// Find a spot for the rectangle in the current cache image.
	// Return true if there's a spot; false if there's no room.
	{
		// Nice algo, due to JARE:
		//
		// * keep a list of "candidate points"; initialize it with {0,0}
		//
		// * each time we add a rect, add its lower-left and
		// upper-right as candidate points.
		//
		// * search the candidate points only, when looking
		// for a good spot.  If we find a good one, also try
		// scanning left or up as well; sometimes this can
		// close some open space.
		//
		// * when we use a candidate point, remove it from the list.

		// Consider candidate spots.
		for (int i = 0, n = s_anchor_points.size(); i < n; i++)
		{
			const pointi&	p = s_anchor_points[i];
			recti	r(p.m_x, p.m_x + width, p.m_y, p.m_y + height);

			// Is this spot any good?
			if (is_rect_available(r))
			{
				// Good spot.  Scan left to see if we can tighten it up.
				while (r.m_x_min > 0)
				{
					recti	r2(r.m_x_min - 1, r.m_x_min - 1 + width, r.m_y_min, r.m_y_min + height);
					if (is_rect_available(r2))
					{
						// Shift left.
						r = r2;
					}
					else
					{
						// Not clear; stop scanning.
						break;
					}
				}

				// Mark our covered rect; remove newly covered anchors.
				add_cover_rect(r);

				// Found our desired spot.  Add new
				// candidate points to the anchor list.
				add_anchor_point(pointi(r.m_x_min, r.m_y_max));	// lower-left
				add_anchor_point(pointi(r.m_x_max, r.m_y_min));	// upper-right

				*px = r.m_x_min;
				*py = r.m_y_min;

				return true;
			}
		}

		// Couldn't find a good spot.
		return false;
	}


	// This is for keeping track of our rendered glyphs, before
	// packing them into textures and registering with the font.
	struct rendered_glyph_info
	{
		font*	m_source_font;
		int	m_glyph_index;
		image::alpha*	m_image;
		unsigned int	m_image_hash;
		float	m_offset_x;
		float	m_offset_y;

		rendered_glyph_info()
			:
			m_source_font(0),
			m_glyph_index(0),
			m_image(0),
			m_image_hash(0),
			m_offset_x(0),
			m_offset_y(0)
		{
		}

		~rendered_glyph_info()
		{
			delete m_image;
		}
	};


	static void	software_trapezoid(
		float y0, float y1,
		float xl0, float xl1,
		float xr0, float xr1)
	// Fill the specified trapezoid in the software output buffer.
	{
		assert(s_render_buffer);

		int	iy0 = (int) ceilf(y0);
		int	iy1 = (int) ceilf(y1);
		float	dy = y1 - y0;

		for (int y = iy0; y < iy1; y++)
		{
			if (y < 0) continue;
			if (y >= GLYPH_RENDER_SIZE) return;

			float	f = (y - y0) / dy;
			int	xl = (int) ceilf(flerp(xl0, xl1, f));
			int	xr = (int) ceilf(flerp(xr0, xr1, f));
			
			xl = iclamp(xl, 0, GLYPH_RENDER_SIZE - 1);
			xr = iclamp(xr, 0, GLYPH_RENDER_SIZE - 1);

			if (xr > xl)
			{
				memset(s_render_buffer + y * GLYPH_RENDER_SIZE + xl,
				       255,
				       xr - xl);
			}
		}
	}


	struct draw_into_software_buffer : tesselate::trapezoid_accepter
	// A trapezoid accepter that does B&W rendering into our
	// software buffer.
	{
		// Overrides from trapezoid_accepter
		virtual void	accept_trapezoid(int style, const tesselate::trapezoid& tr)
		{
			// Transform the coords.
			float	x_scale = s_render_matrix.m_[0][0];
			float	y_scale = s_render_matrix.m_[1][1];
			float	x_offset = s_render_matrix.m_[0][2];
			float	y_offset = s_render_matrix.m_[1][2];

			float	y0 = tr.m_y0 * y_scale + y_offset;
			float	y1 = tr.m_y1 * y_scale + y_offset;
			float	lx0 = tr.m_lx0 * x_scale + x_offset;
			float	lx1 = tr.m_lx1 * x_scale + x_offset;
			float	rx0 = tr.m_rx0 * x_scale + x_offset;
			float	rx1 = tr.m_rx1 * x_scale + x_offset;

			// Draw into the software buffer.
			software_trapezoid(y0, y1, lx0, lx1, rx0, rx1);
		}

		virtual void	accept_line_strip(int style, const point coords[], int coord_count)
		{
			assert(0);	// Shape glyphs should not contain lines.
		}
	};


	static void	render_glyph(rendered_glyph_info* rgi, const shape_character* sh)
	// Render the given outline shape into a cached font texture.
	// 
	// Return fill in the image and offset members of the given
	// rgi.
	{
		assert(rgi);
		assert(sh);
		assert(s_render_buffer);

		//
		// Tesselate and render the shape into a software buffer.
		//

		// Clear the render output to 0.
		memset(s_render_buffer, 0, GLYPH_RENDER_SIZE * GLYPH_RENDER_SIZE);

		// Look at glyph bounds; adjust origin to make sure
		// the shape will fit in our output.
		float	offset_x = 0.f;
		float	offset_y = s_rendering_box;
		rect	glyph_bounds;
		sh->compute_bound(&glyph_bounds);
		if (glyph_bounds.m_x_min < 0)
		{
			offset_x = - glyph_bounds.m_x_min;
		}
		if (glyph_bounds.m_y_max > 0)
		{
			offset_y = s_rendering_box - glyph_bounds.m_y_max;
		}

		s_render_matrix.set_identity();
		s_render_matrix.concatenate_scale(GLYPH_RENDER_SIZE / s_rendering_box);
		s_render_matrix.concatenate_translation(offset_x, offset_y);

		// Tesselate & draw the shape.
		draw_into_software_buffer	accepter;
		sh->tesselate(s_rendering_box / GLYPH_RENDER_SIZE * 0.5f, &accepter);

		//
		// Process the results of rendering.
		//

		// Shrink the results down by a factor of 4x, to get
		// antialiasing.  Also, analyze the data boundaries.
		int	min_x = GLYPH_FINAL_SIZE;
		int	max_x = 0;
		int	min_y = GLYPH_FINAL_SIZE;
		int	max_y = 0;
		Uint8	output[GLYPH_FINAL_SIZE * GLYPH_FINAL_SIZE];
		for (int j = 0; j < GLYPH_FINAL_SIZE; j++)
		{
			for (int i = 0; i < GLYPH_FINAL_SIZE; i++)
			{
				// Sum up the contribution to this output texel.
				int	sum = 0;
				for (int jj = 0; jj < OVERSAMPLE_FACTOR; jj++)
				{
					for (int ii = 0; ii < OVERSAMPLE_FACTOR; ii++)
					{
						Uint8	texel = s_render_buffer[
							((j << OVERSAMPLE_BITS) + jj) * GLYPH_RENDER_SIZE
							+ ((i << OVERSAMPLE_BITS) + ii)];
						sum += texel;
					}
				}
				sum >>= OVERSAMPLE_BITS;
				sum >>= OVERSAMPLE_BITS;
				if (sum > 0)
				{
					min_x = imin(min_x, i);
					max_x = imax(max_x, i);
					min_y = imin(min_y, j);
					max_y = imax(max_y, j);
				}
				output[j * GLYPH_FINAL_SIZE + i] = (Uint8) sum;
			}
		}

		// Fill in rendered_glyph_info.
		rgi->m_image = new image::alpha(max_x - min_x + 1, max_y - min_y + 1);
		rgi->m_offset_x = offset_x / s_rendering_box * GLYPH_FINAL_SIZE - min_x;
		rgi->m_offset_y = offset_y / s_rendering_box * GLYPH_FINAL_SIZE - min_y;

		// Copy the rendered glyph into the new image.
		{for (int j = 0, n = rgi->m_image->m_height; j < n; j++)
		{
			memcpy(
				image::scanline(rgi->m_image, j),
				output + (min_y + j) * GLYPH_FINAL_SIZE + min_x,
				rgi->m_image->m_width);
		}}

		rgi->m_image_hash = rgi->m_image->compute_hash();
	}



	bool	try_to_reuse_previous_image(
		const rendered_glyph_info& rgi,
		const hash<unsigned int, const rendered_glyph_info*>& image_hash)
	// See if we've already packed an identical glyph image for
	// another glyph.  If so, then reuse it, and return true.
	// If no reusable image, return false.
	//
	// Reusing identical images can be a huge win, especially for
	// fonts that use the same dummy glyph for many undefined
	// characters.
	{
		const rendered_glyph_info*	identical_image = NULL;
		if (image_hash.get(rgi.m_image_hash, &identical_image))
		{
			// Found a match.  But is it *really* a match?  Do a
			// bitwise compare.
			if (*(rgi.m_image) == *(identical_image->m_image))
			{
				// Yes, a real bitwise match.  Use the previous
				// image's texture data.
				const texture_glyph*	identical_tg = identical_image->m_source_font->get_texture_glyph(
					identical_image->m_glyph_index);
				if (identical_tg)
				{
					texture_glyph*	tg = new texture_glyph;
					*tg = *identical_tg;

					// Use our own offset, in case it's different.
					tg->m_uv_origin.m_x = identical_tg->m_uv_bounds.m_x_min
						+ rgi.m_offset_x / GLYPH_CACHE_TEXTURE_SIZE;
					tg->m_uv_origin.m_y = identical_tg->m_uv_bounds.m_y_min
						+ rgi.m_offset_y / GLYPH_CACHE_TEXTURE_SIZE;

					rgi.m_source_font->add_texture_glyph(rgi.m_glyph_index, tg);

					return true;
				}
			}
			// else hash matched, but images didn't.
		}

		return false;
	}


	void	pack_and_assign_glyphs(array<rendered_glyph_info>* glyph_info)
	// Pack the given glyphs into textures, and push the
	// texture_glyph info into the source fonts.
	//
	// Re-arranges the glyphs (i.e. sorts them by size) but
	// otherwise doesn't munge the array.
	{
		// Sort the glyphs by size (biggest first).
		struct sorter
		{
			static int	sort_by_size(const void* a, const void* b)
			// For qsort.
			{
				const rendered_glyph_info*	ga = (const rendered_glyph_info*) a;
				const rendered_glyph_info*	gb = (const rendered_glyph_info*) b;

				int	a_size = ga->m_image->m_width + ga->m_image->m_height;
				int	b_size = gb->m_image->m_width + gb->m_image->m_height;

				return b_size - a_size;
			}
		};
		if (glyph_info->size())
		{
			qsort(&(*glyph_info)[0], glyph_info->size(), sizeof((*glyph_info)[0]), sorter::sort_by_size);
		}

		// Flag for whether we've processed this glyph yet.
		array<bool>	packed;
		packed.resize(glyph_info->size());
		for (int i = 0, n = packed.size(); i < n; i++)
		{
			packed[i] = false;
		}

		// Share identical texture data where possible, by
		// doing glyph image comparisons.
		hash<unsigned int, const rendered_glyph_info*>	image_hash;

		// Pack the glyphs.
		{for (int i = 0, n = glyph_info->size(); i < n; )
		{
			int	index = i;

			// Try to pack a glyph into the existing texture.
			for (;;)
			{
				const rendered_glyph_info&	rgi = (*glyph_info)[index];

				// First things first: are we identical to a glyph that has
				// already been packed?
				if (try_to_reuse_previous_image(rgi, image_hash))
				{
					packed[index] = true;
					break;
				}

				int	raw_width = rgi.m_image->m_width;
				int	raw_height = rgi.m_image->m_height;

				// Need to pad around the outside.
				int	width = raw_width + (PAD_PIXELS * 2);
				int	height = raw_height + (PAD_PIXELS * 2);

				assert(width < GLYPH_CACHE_TEXTURE_SIZE);
				assert(height < GLYPH_CACHE_TEXTURE_SIZE);

				// Does this glyph fit?
				int	pack_x = 0, pack_y = 0;
				ensure_cache_image_available();
				if (pack_rectangle(&pack_x, &pack_y, width, height))
				{
					// Fits!
					// Blit the output image into its new spot.
					for (int j = 0; j < raw_height; j++)
					{
						memcpy(s_current_cache_image
						       + (pack_y + PAD_PIXELS + j) * GLYPH_CACHE_TEXTURE_SIZE
						       + pack_x + PAD_PIXELS,
						       image::scanline(rgi.m_image, j),
						       raw_width);
					}

					// Fill out the glyph info.
					texture_glyph*	tg = new texture_glyph;
					tg->m_bitmap_info = s_current_bitmap_info;
					tg->m_uv_origin.m_x = (pack_x + rgi.m_offset_x) / (GLYPH_CACHE_TEXTURE_SIZE);
					tg->m_uv_origin.m_y = (pack_y + rgi.m_offset_y) / (GLYPH_CACHE_TEXTURE_SIZE);
					tg->m_uv_bounds.m_x_min = float(pack_x) / (GLYPH_CACHE_TEXTURE_SIZE);
					tg->m_uv_bounds.m_x_max = float(pack_x + width) / (GLYPH_CACHE_TEXTURE_SIZE);
					tg->m_uv_bounds.m_y_min = float(pack_y) / (GLYPH_CACHE_TEXTURE_SIZE);
					tg->m_uv_bounds.m_y_max = float(pack_y + height) / (GLYPH_CACHE_TEXTURE_SIZE);

					rgi.m_source_font->add_texture_glyph(rgi.m_glyph_index, tg);

					// Add this into the hash so it can possibly be reused.
					if (image_hash.get(rgi.m_image_hash, NULL) == false)
					{
						image_hash.add(rgi.m_image_hash, &rgi);
					}

					packed[index] = true;

					break;
				}
				else
				{
					// Try the next unpacked glyph.
					index++;
					while (index < n && packed[index]) index++;

					if (index >= n)
					{
						// None of the glyphs will fit.  Finish off this texture.
						finish_current_texture();

						// And go around again.
						index = i;
					}
				}
			}

			// Skip to the next unpacked glyph.
			while (i < n && packed[i]) i++;
		}}
	}



	static void	generate_font_bitmaps(array<rendered_glyph_info>* glyph_info, font* f)
	// Render images for each of the font's glyphs, and put the
	// info about them in the given array.
	{
		assert(glyph_info);
		assert(f);

		for (int i = 0, n = f->get_glyph_count(); i < n; i++)
		{
			if (f->get_texture_glyph(i) == NULL)
			{
				shape_character*	sh = f->get_glyph(i);
				if (sh)
				{
					rect	glyph_bounds;
					sh->compute_bound(&glyph_bounds);

					int w = (int) glyph_bounds.width();
					int h = (int) glyph_bounds.height();
					if (glyph_bounds.width() < 0)
					{
						// Invalid width; this must be an empty glyph.
						// Don't bother generating a texture for it.
					}
					else
					{
						// Add a glyph.
						glyph_info->resize(glyph_info->size() + 1);
						rendered_glyph_info&	rgi = glyph_info->back();
						rgi.m_source_font = f;
						rgi.m_glyph_index = i;

						render_glyph(&rgi, sh);
					}
				}
			}
		}
	}


	float	get_nominal_texture_glyph_height()
	{
		return 1024.0f / s_rendering_box * GLYPH_FINAL_SIZE;
	}


	//
	// Public interface
	//


	void	generate_font_bitmaps(const array<font*>& fonts)
	// Build cached textures from glyph outlines.
	{
		assert(s_render_buffer == NULL);
		s_render_buffer = new Uint8[GLYPH_RENDER_SIZE * GLYPH_RENDER_SIZE];

		// Build the glyph images.
		array<rendered_glyph_info>	glyph_info;
		for (int i = 0; i < fonts.size(); i++)
		{
			generate_font_bitmaps(&glyph_info, fonts[i]);
		}

		// Pack all the rendered glyphs and push the info into their fonts.
		pack_and_assign_glyphs(&glyph_info);

		// Finish off any pending cache texture.
		finish_current_texture();

		// Clean up our buffers.
		if (s_current_cache_image)
		{
			delete [] s_current_cache_image;
			s_current_cache_image = NULL;

			s_covered_rects.resize(0);
			s_anchor_points.resize(0);
		}

		// Clean up the render buffer that we just used.
		assert(s_render_buffer);
		delete [] s_render_buffer;
		s_render_buffer = NULL;
	}


	void	output_cached_data(tu_file* out, const array<font*>& fonts)
	// Save cached font data, includeing glyph textures, to a
	// stream.  This is used by the movie caching code.
	{
		assert(out);

		// skip number of bitmaps.
		int	bitmaps_used_base = s_bitmaps_used.size();
		int	size_pos = out->get_position();
		printf("size_pos = %d\n", size_pos);//xxxxxxxx
		out->write_le16(0);
		
		// save bitmaps
		s_file = out;
		s_saving = true;		// HACK!!!
		generate_font_bitmaps(fonts);
		s_saving = false;
		s_file = NULL;
		
		printf("wrote bitmaps, pos = %d\n", out->get_position());//xxxxxxxx

		// save number of bitmaps.
		out->set_position(size_pos);
		out->write_le16(s_bitmaps_used.size() - bitmaps_used_base);
		printf("bitmap count = %d\n", s_bitmaps_used.size() - bitmaps_used_base);//xxxxxx
		out->go_to_end();
		
		// save number of fonts.
		out->write_le16(fonts.size());
		
		// for each font:
		for (int f = 0; f < fonts.size(); f++)
		{
			// skip number of glyphs.
			int ng = fonts[f]->get_glyph_count();
			int ng_position = out->get_position();
			out->write_le32(0);
				
			int n = 0;
				
			// save glyphs:
			for (int g = 0; g < ng; g++)
			{
				const texture_glyph * tg = fonts[f]->get_texture_glyph(g);
				if (tg != NULL)
				{
					// save glyph index.
					out->write_le32(g);

					// save bitmap index.
					int bi;
					for (bi = bitmaps_used_base; bi < s_bitmaps_used.size(); bi++)
					{
						if (tg->m_bitmap_info == s_bitmaps_used[bi])
						{
							break;
						}
					}
					assert(bi >= bitmaps_used_base
					       && bi < s_bitmaps_used.size());

					out->write_le16((Uint16) (bi - bitmaps_used_base));

					// save rect, position.
					out->write_float32(tg->m_uv_bounds.m_x_min);
					out->write_float32(tg->m_uv_bounds.m_y_min);
					out->write_float32(tg->m_uv_bounds.m_x_max);
					out->write_float32(tg->m_uv_bounds.m_y_max);
					out->write_float32(tg->m_uv_origin.m_x);
					out->write_float32(tg->m_uv_origin.m_y);
					n++;
				}
			}

			out->set_position(ng_position);
			out->write_le32(n);
			out->go_to_end();

			// Output cached shape data.
			fonts[f]->output_cached_data(out);
		}

		if (out->get_error() != TU_FILE_NO_ERROR)
		{
			log_error("gameswf::fontlib::save_cached_font_data(): problem writing to output stream!");
		}
	}
	

	void	input_cached_data(tu_file* in, const array<font*>& fonts)
	// Load a stream containing previously-saved font glyph textures.
	{
		// load number of bitmaps.
		int nb = in->read_le16();

		int pw = 0, ph = 0;

		// load bitmaps.
		int	bitmaps_used_base = s_bitmaps_used.size();
		for (int b = 0; b < nb; b++)
		{
			s_current_bitmap_info = render::create_bitmap_info_blank();
			s_bitmaps_used.push_back(s_current_bitmap_info);

			// save bitmap size
			int w = in->read_le16();
			int h = in->read_le16();

			if (s_current_cache_image == NULL || w != pw || h != ph)
			{
				delete [] s_current_cache_image;
				s_current_cache_image = new Uint8[w*h];
				pw = w;
				ph = h;
			}

			// load bitmap contents
			in->read_bytes(s_current_cache_image, w * h);

			render::set_alpha_image(
				s_current_bitmap_info,
				w, h,
				s_current_cache_image);
		}

		// reset pointers.
		s_current_bitmap_info = NULL;
		delete [] s_current_cache_image;
		s_current_cache_image = NULL;

		// load number of fonts.
		int nf = in->read_le16();
		if (nf != fonts.size())
		{
			// Font counts must match!
			log_error("error: mismatched font count (read %d, expected %d) in cached font data\n", nf, fonts.size());
			in->go_to_end();
			goto error_exit;
		}

		// for each font:
		{for (int f = 0; f < nf; f++)
		{
			if (in->get_error() != TU_FILE_NO_ERROR)
			{
				log_error("error reading cache file (fonts); skipping\n");
				return;
			}
			if (in->get_eof())
			{
				log_error("unexpected eof reading cache file (fonts); skipping\n");
				return;
			}

			// load number of texture glyphs.
			int ng = in->read_le32();

			// load glyphs:
			for (int g=0; g<ng; g++)
			{
				// load glyph index.
				int glyph_index = in->read_le32();

				texture_glyph * tg = new texture_glyph;

				// load bitmap index
				int bi = in->read_le16();
				if (bi + bitmaps_used_base >= s_bitmaps_used.size())
				{
					// Bad data; give up.
					log_error("error: invalid bitmap index %d in cached font data\n", bi);
					in->go_to_end();
					goto error_exit;
				}

				tg->m_bitmap_info = s_bitmaps_used[bi + bitmaps_used_base];

				// load glyph bounds and origin.
				tg->m_uv_bounds.m_x_min = in->read_float32();
				tg->m_uv_bounds.m_y_min = in->read_float32();
				tg->m_uv_bounds.m_x_max = in->read_float32();
				tg->m_uv_bounds.m_y_max = in->read_float32();
				tg->m_uv_origin.m_x = in->read_float32();
				tg->m_uv_origin.m_y = in->read_float32();

				fonts[f]->add_texture_glyph(glyph_index, tg);
			}

			// Load cached shape data.
			fonts[f]->input_cached_data(in);
		}}

	error_exit:
		;

	}


	int	get_font_count()
	// Return the number of fonts in our library.
	{
		return s_fonts.size();
	}


	font*	get_font(int index)
	// Retrieve one of our fonts, by index.
	{
		if (index < 0 || index >= s_fonts.size())
		{
			return NULL;
		}

		return s_fonts[index];
	}


	font*	get_font(const char* name)
	// Return the named font.
	{
		// Dumb linear search.
		for (int i = 0; i < s_fonts.size(); i++)
		{
			if (strcmp(s_fonts[i]->get_name(), name) == 0)
			{
				return s_fonts[i];
			}
		}
		return NULL;
	}


	const char*	get_font_name(const font* f)
	// Return the name of the given font.
	{
		if (f == NULL)
		{
			return "<null>";
		}
		return f->get_name();
	}


	void	add_font(font* f)
	// Add the given font to our library.
	{
#ifndef NDEBUG
		// Make sure font isn't already in the list.
		for (int i = 0; i < s_fonts.size(); i++)
		{
			assert(s_fonts[i] != f);
		}
#endif // not NDEBUG

		s_fonts.push_back(f);
	}


	void	draw_glyph(const matrix& mat, const texture_glyph* tg, rgba color)
	// Draw the given texture glyph using the given transform, in
	// the given color.
	{
		assert(tg);
//		assert(tg->m_bitmap_info);

		// @@ worth it to precompute these bounds?

		rect	bounds = tg->m_uv_bounds;
		bounds.m_x_min -= tg->m_uv_origin.m_x;
		bounds.m_x_max -= tg->m_uv_origin.m_x;
		bounds.m_y_min -= tg->m_uv_origin.m_y;
		bounds.m_y_max -= tg->m_uv_origin.m_y;

		// Scale from uv coords to the 1024x1024 glyph square.
		static float	s_scale = GLYPH_CACHE_TEXTURE_SIZE * s_rendering_box / GLYPH_FINAL_SIZE;

		bounds.m_x_min *= s_scale;
		bounds.m_x_max *= s_scale;
		bounds.m_y_min *= s_scale;
		bounds.m_y_max *= s_scale;
		
		render::draw_bitmap(mat, tg->m_bitmap_info, bounds, tg->m_uv_bounds, color);
	}


	void	draw_string(const font* f, float x, float y, float size, const char* text)
	// Host-driven text rendering function. This not-tested and unfinished.
	{
		// Dummy arrays with a white fill style.  For passing to shape_character::display().
		static array<fill_style>	s_dummy_style;
		static array<line_style>	s_dummy_line_style;
		static display_info	s_dummy_display_info;
		if (s_dummy_style.size() < 1)
		{
			s_dummy_style.resize(1);
			s_dummy_style.back().set_color(rgba(255, 255, 255, 255));
		}

		// Render each glyph in the string.
		for (int i = 0; text[i]; i++)
		{
			int g = f->get_glyph_index(text[i]);
			if (g == -1) 
			{
				continue;	// FIXME: advance?
			}

			const texture_glyph*	tg = f->get_texture_glyph(g);
			
			matrix m;
			m.concatenate_translation(x, y);
			m.concatenate_scale(size / 1024.0f);

			if (tg)
			{
				// Draw the glyph using the cached texture-map info.
				fontlib::draw_glyph(m, tg, rgba());
			}
			else
			{
				shape_character*	glyph = f->get_glyph(g);

				// Draw the character using the filled outline.
				if (glyph)
				{
					glyph->display(s_dummy_display_info, s_dummy_style, s_dummy_line_style);
				}
			}

			x += f->get_advance(g);
		}

	}

}	// end namespace fontlib
}	// end namespace gameswf


#endif	// GAMESWF_FONTLIB_CPP

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
