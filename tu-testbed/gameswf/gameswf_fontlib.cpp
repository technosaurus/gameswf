// gameswf_fontlib.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A module to take care of all of gameswf's loaded fonts.


#ifndef GAMESWF_FONTLIB_CPP


#include "engine/container.h"
#include "engine/tu_file.h"
#include "gameswf_font.h"
#include "gameswf_impl.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_styles.h"
#include "gameswf_tesselate.h"


namespace gameswf
{
namespace fontlib
{
	array<font*>	s_fonts;
	array<render::bitmap_info*>	s_bitmaps_used;	// keep these so we can delete them during shutdown.

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

	// Align packed glyphs on pixel boundaries of multiples of (1
	// << GLYPH_PACK_ROUNDING_BITS).  I.e. if
	// GLYPH_PACK_ROUNDING_BITS == 2, then align the packed glyphs
	// on 4-pixel boundaries.  The idea here is to avoid bleeding
	// of packed glyphs into their neighbors, when drawing scaled
	// glyphs with mip-mapping.  Some bleed is inevitable, but we
	// can cover a pretty decent range of scales using modest pack
	// rounding.
	//
	// Note that this is much better than just padding all around
	// the outside of an arbitrarily sized glyph, since
	// mip-mapping naturally observes power-of-two boundaries.
	static const int	GLYPH_PACK_ROUNDING_BITS = 3;

	// We make a little bitmap coverage thingy, to help pack our
	// glyphs into the cache texture.  Basically we play Tetris
	// with the glyph rects.
	static const int	GLYPH_COVERAGE_BITMAP_SIZE = (GLYPH_CACHE_TEXTURE_SIZE >> GLYPH_PACK_ROUNDING_BITS);

	//
	// State for the glyph packer.
	//

	static Uint8*	s_render_buffer = NULL;
	static matrix	s_render_matrix;

	static Uint8*	s_current_cache_image = NULL;
	static render::bitmap_info*	s_current_bitmap_info = NULL;
	static Uint8*	s_coverage_image = NULL;

	static bool	s_saving = false;
	static tu_file*	s_file = NULL;

	void	finish_current_texture()
	{
		if (s_current_bitmap_info == NULL)
		{
			return;
		}

// 		//xxxxxx debug hack -- dump image data to a file
// 		static int	s_seq = 0;
// 		char buffer[100];
// 		sprintf(buffer, "dump%02d.ppm", s_seq);
// 		s_seq++;
// 		FILE*	fp = fopen(buffer, "wb");
// 		if (fp)
// 		{
// 			fprintf(fp, "P6\n%d %d\n255\n", GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE);
// 			for (int i = 0; i < GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE; i++)
// 			{
// 				fputc(s_current_cache_image[i], fp);
// 				fputc(s_current_cache_image[i], fp);
// 				fputc(s_current_cache_image[i], fp);
// 			}
// 			fclose(fp);
// 		}
// 		//xxxxxx

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


	bool	pack_rectangle(int* px, int* py, int width, int height)
	// Find a spot for the rectangle in the current cache image.
	// Return true if there's a spot; false if there's no room.
	{
		// Really dumb implementation.  Just search for a fit.

		// Width/height coords, scaled down to the coverage map.
		int	round_up_add = (1 << GLYPH_PACK_ROUNDING_BITS);
		int	cw = (width + round_up_add) >> GLYPH_PACK_ROUNDING_BITS;
		int	ch = (height + round_up_add) >> GLYPH_PACK_ROUNDING_BITS;

		for (int j = 0; j < GLYPH_COVERAGE_BITMAP_SIZE - ch + 1; j++)
		{
			for (int i = 0; i < GLYPH_COVERAGE_BITMAP_SIZE - cw + 1; i++)
			{
				// Fit?
				bool	fail = false;
				for (int jj = 0; jj < ch && fail == false; jj++)
				{
					for (int ii = 0; ii < cw; ii++)
					{
						if (s_coverage_image[(j + jj) * GLYPH_COVERAGE_BITMAP_SIZE + (i + ii)])
						{
							fail = true;
							break;
						}
					}
				}
				if (fail == false)
				{
					// Found a spot.

					// Mark the coverage map as filled.
					for (int jj = 0; jj < ch; jj++)
					{
						memset(s_coverage_image + (j + jj) * GLYPH_COVERAGE_BITMAP_SIZE + i,
						       0xFF,
						       cw);
					}

					*px = i << GLYPH_PACK_ROUNDING_BITS;
					*py = j << GLYPH_PACK_ROUNDING_BITS;
					
					return true;
				}
			}
		}

		return false;
	}


	texture_glyph*	texture_pack(
		Uint8* image_data,
		int min_x,
		int min_y,
		int max_x,
		int max_y,
		float offset_x,
		float offset_y
		)
	// Pack the given image data into an available texture, and
	// return a new texture_glyph structure containging info for
	// rendering the cached glyph.
	{
		int	raw_width = (max_x - min_x + 1);
		int	raw_height = (max_y - min_y + 1);

		// Round up to nearest rounding boundary.  We also
		// need to leave 1 texel worth of blank space all
		// around the glyph image.
		static const int	mask = (1 << GLYPH_PACK_ROUNDING_BITS) - 1;
		int	width = (raw_width + 2 + mask) & ~mask;
		int	height = (raw_height + 2 + mask) & ~mask;

		assert(width < GLYPH_CACHE_TEXTURE_SIZE);
		assert(height < GLYPH_CACHE_TEXTURE_SIZE);

		for (int iteration_count = 0; iteration_count < 2; iteration_count++)
		{
			// Make sure cache image is available.
			if (s_current_bitmap_info == NULL)
			{
				// Set up a cache.
				s_current_bitmap_info = render::create_bitmap_info_blank();
				s_bitmaps_used.push_back(s_current_bitmap_info);

				if (s_current_cache_image == NULL)
				{
					s_current_cache_image = new Uint8[GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE];

					assert(s_coverage_image == NULL);
					s_coverage_image = new Uint8[GLYPH_COVERAGE_BITMAP_SIZE * GLYPH_COVERAGE_BITMAP_SIZE];
				}
				memset(s_current_cache_image, 0, GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE);
				memset(s_coverage_image, 0, GLYPH_COVERAGE_BITMAP_SIZE * GLYPH_COVERAGE_BITMAP_SIZE);
			}
		
			// Can we fit the image data into the current cache?
			int	pack_x = 0, pack_y = 0;
			if (pack_rectangle(&pack_x, &pack_y, width, height))
			{
				// Blit the output image into its new spot.
				for (int j = 0; j < raw_height; j++)
				{
					memcpy(s_current_cache_image
					       + (pack_y + 1 + j) * GLYPH_CACHE_TEXTURE_SIZE
					       + pack_x + 1,
					       image_data + (min_y + j) * GLYPH_FINAL_SIZE + min_x,
					       raw_width);
				}

				// Fill out the glyph info.
				texture_glyph*	tg = new texture_glyph;
				tg->m_bitmap_info = s_current_bitmap_info;
				tg->m_uv_origin.m_x = (float(pack_x) - min_x + offset_x) / (GLYPH_CACHE_TEXTURE_SIZE);
				tg->m_uv_origin.m_y = (float(pack_y) - min_y + offset_y) / (GLYPH_CACHE_TEXTURE_SIZE);
				tg->m_uv_bounds.m_x_min = float(pack_x + 0.5f) / (GLYPH_CACHE_TEXTURE_SIZE);
				tg->m_uv_bounds.m_x_max = float(pack_x + 1.5f + raw_width) / (GLYPH_CACHE_TEXTURE_SIZE);
				tg->m_uv_bounds.m_y_min = float(pack_y + 0.5f) / (GLYPH_CACHE_TEXTURE_SIZE);
				tg->m_uv_bounds.m_y_max = float(pack_y + 1.5f + raw_height) / (GLYPH_CACHE_TEXTURE_SIZE);

				return tg;
			}
			else
			{
				// If we couldn't fit our glyph into a
				// blank cache texture, then we have a
				// bug!
				assert(iteration_count == 0);

				finish_current_texture();
				// go around again.
			}
		}

		assert(0);
		return NULL;
	}


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


	static texture_glyph*	make_texture_glyph(const shape_character* sh)
	// Render the given outline shape into a cached font texture.
	// Return a new texture_glyph struct that gives enough info to
	// render the cached glyph as a textured quad.
	{
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

		// Pack into an available texture.
		return texture_pack(output, min_x, min_y, max_x, max_y,
				    offset_x / s_rendering_box * GLYPH_FINAL_SIZE,
				    offset_y / s_rendering_box * GLYPH_FINAL_SIZE);
	}

	static int compare_glyphs( const void * a, const void * b ) {
		return *(int *)b - *(int *)a;
	}
	
	static void	generate_font_bitmaps(font* f)
	// Build cached textured versions of the font's glyphs, and
	// store them in the font.
	{
		assert(f);

		struct sorted_s {
			int size;
			int i;
		} * sorted_array;

		int	count = f->get_glyph_count();
		sorted_array = new sorted_s[count];
		
		int i, n=0;
		for (i = 0; i < count; i++)
		{
			if (f->get_texture_glyph(i) == NULL)
			{
				shape_character*	sh = f->get_glyph(i);
				if (sh)
				{
					// get a rough estimation of glyph size
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
						sorted_array[n].size = w>h ? w : h;
						sorted_array[n].i = i;
						n++;
					}
				}
			}
		}
		
		qsort( sorted_array, n, sizeof(sorted_s), compare_glyphs );
		
		for (i = 0; i < n; i++)
		{
			shape_character*	sh = f->get_glyph(sorted_array[i].i);
			texture_glyph*	tg = make_texture_glyph(sh);
			if (tg)
			{
				f->add_texture_glyph(sorted_array[i].i, tg);
			}
		}
		
		delete [] sorted_array;
	}


	float	get_nominal_texture_glyph_height()
	{
		return 1024.0f / s_rendering_box * GLYPH_FINAL_SIZE;
	}


	//
	// Public interface
	//


	void	generate_font_bitmaps()
	// Build cached textures from glyph outlines.
	{
		assert(s_render_buffer == NULL);
		s_render_buffer = new Uint8[GLYPH_RENDER_SIZE * GLYPH_RENDER_SIZE];

		for (int i = 0; i < s_fonts.size(); i++)
		{
			generate_font_bitmaps(s_fonts[i]);
		}

		// Finish off any pending cache texture.
		finish_current_texture();

		// Clean up our buffers.
		if (s_current_cache_image)
		{
			delete [] s_current_cache_image;
			s_current_cache_image = NULL;

			assert(s_coverage_image);
			delete [] s_coverage_image;
			s_coverage_image = NULL;
		}

		// Clean up the render buffer that we just used.
		assert(s_render_buffer);
		delete [] s_render_buffer;
		s_render_buffer = NULL;
	}


	void	save_cached_font_data(const char* filename)
	// Save cached glyph textures to a file.  This might be used by an
	// offline tool, which loads in all movies, calls
	// generate_font_bitmaps(), and then calls save_cached_font_data()
	// so that a run-time app can call load_cached_font_data().
	{
		s_file = new tu_file(filename, "wb");
		if (s_file->get_error() == TU_FILE_NO_ERROR)
		{
			// save header identifier.
			s_file->write_bytes( "gswf", 4 );

			// save version number.
			s_file->write_le16( 0x0100 );
			
			// skip number of bitmaps.
			s_file->write_le16( 0 );
			
			// save bitmaps
			s_saving = true;		// HACK!!!
			generate_font_bitmaps();
			s_saving = false;
			
			// save number of bitmaps.
			int restore = s_file->get_position();
			s_file->set_position(6);
			s_file->write_le16( s_bitmaps_used.size() );
			s_file->set_position(restore);
			
			// save number of fonts.
			s_file->write_le16( s_fonts.size() );
			
			// for each font:
			for (int f=0; f < s_fonts.size(); f++)
			{
				// skip number of glyphs.
				int ng = s_fonts[f]->get_glyph_count();
				int ng_position = s_file->get_position();
				s_file->write_le32(0);
				
				int n = 0;
				
				// save glyphs:
				for (int g=0; g<ng; g++)
				{
					const texture_glyph * tg = s_fonts[f]->get_texture_glyph(g);
					if (tg!=NULL)
					{
						// save glyph index.
						s_file->write_le32(g);

						// save bitmap index.
						int bi;
						for (bi=0; bi<s_bitmaps_used.size(); bi++)
						{
							if (tg->m_bitmap_info==s_bitmaps_used[bi])
							{
								break;
							}
						}
						assert(bi!=s_bitmaps_used.size());
						s_file->write_le16((Uint16)bi);

						// save rect, position.
						s_file->write_le32((Uint32&)tg->m_uv_bounds.m_x_min);
						s_file->write_le32((Uint32&)tg->m_uv_bounds.m_y_min);
						s_file->write_le32((Uint32&)tg->m_uv_bounds.m_x_max);
						s_file->write_le32((Uint32&)tg->m_uv_bounds.m_y_max);
						s_file->write_le32((Uint32&)tg->m_uv_origin.m_x);
						s_file->write_le32((Uint32&)tg->m_uv_origin.m_y);
						n++;
					}
				}
				
				restore = s_file->get_position();
				s_file->set_position(ng_position);
				s_file->write_le32(n);
				s_file->set_position(restore);
			}
		}
		else
		{
			printf("cannot open '%s'\n", filename);
		}
		delete s_file;
		s_file = NULL;
	}
	

	bool	load_cached_font_data(const char* filename)
	// Load a file containing previously-saved font glyph textures.
	{
		bool result = false;
		s_file = new tu_file(filename, "rb");
		if (s_file->get_error() == TU_FILE_NO_ERROR)
		{
			// load header identifier.
			char head[4];
			s_file->read_bytes( head, 4 );
			if (head[0] != 'g' || head[1] != 's' || head[2] != 'w' || head[3] != 'f')
			{
				// Header doesn't look like a gswf cache file.
				assert(0);
				goto error_exit;
			}

			// load version number.
			Uint16 version	= s_file->read_le16();
			if (version != 0x0100)
			{
				// Bad version number.
				assert(0);
				goto error_exit;
			}

			// load number of bitmaps.
			int nb = s_file->read_le16();
			//s_bitmaps_used.resize(nb);

			int pw=0, ph=0;

			// load bitmaps.
			for (int b=0; b<nb; b++)
			{
				s_current_bitmap_info = render::create_bitmap_info_blank();
				s_bitmaps_used.push_back(s_current_bitmap_info);

				// save bitmap size
				int w = s_file->read_le16();
				int h = s_file->read_le16();

				if (s_current_cache_image == NULL || w!=pw || h!=ph)
				{
					delete [] s_current_cache_image;
					s_current_cache_image = new Uint8[w*h];
					pw = w;
					ph = h;
				}

				// save bitmap contents
				s_file->read_bytes(s_current_cache_image, w*h);

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
			int nf = s_file->read_le16();
			assert(s_fonts.size()==nf);		// FIXME: doesn't have to.

			// for each font:
			for (int f=0; f<nf; f++)
			{
				// load number of glyphs.
				int ng = s_file->read_le32();

				// load glyphs:
				for (int g=0; g<ng; g++)
				{
					// load glyph index.
					int glyph_index = s_file->read_le32();

					texture_glyph * tg = new texture_glyph;

					// load bitmap index
					int bi = s_file->read_le16();
					assert(bi<s_bitmaps_used.size());
					tg->m_bitmap_info = s_bitmaps_used[bi];

					// load glyph bounds and origin.
					(Uint32&)tg->m_uv_bounds.m_x_min = s_file->read_le32();
					(Uint32&)tg->m_uv_bounds.m_y_min = s_file->read_le32();
					(Uint32&)tg->m_uv_bounds.m_x_max = s_file->read_le32();
					(Uint32&)tg->m_uv_bounds.m_y_max = s_file->read_le32();
					(Uint32&)tg->m_uv_origin.m_x = s_file->read_le32();
					(Uint32&)tg->m_uv_origin.m_y = s_file->read_le32();

					s_fonts[f]->add_texture_glyph(glyph_index, tg);
				}
			}
			result = true;
		}
	error_exit:
		delete s_file;
		s_file = NULL;
		return result;
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
		assert(tg->m_bitmap_info);

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
