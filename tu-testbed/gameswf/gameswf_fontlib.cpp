// gameswf_fontlib.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A module to take care of all of gameswf's loaded fonts.


#ifndef GAMESWF_FONTLIB_CPP


#include "engine/container.h"
#include "gameswf_impl.h"
#include "gameswf_font.h"
#include "gameswf_render.h"


namespace gameswf
{
namespace fontlib
{
	array<font*>	s_fonts;
	array<render::bitmap_info*>	s_bitmaps_used;	// keep these so we can delete them during shutdown.

	// The nominal size of the final antialiased glyphs stored in the texture.
	static const int	GLYPH_FINAL_SIZE = 32;

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
	static const int	GLYPH_PACK_ROUNDING_BITS = 2;

	// We make a little bitmap coverage thingy, to help pack our
	// glyphs into the cache texture.  Basically we play Tetris
	// with the glyph rects.
	static const int	GLYPH_COVERAGE_BITMAP_SIZE = (GLYPH_CACHE_TEXTURE_SIZE >> GLYPH_PACK_ROUNDING_BITS);

	//
	// State for the glyph packer.
	//

	static Uint8*	s_current_cache_image = NULL;
	static render::bitmap_info*	s_current_bitmap_info = NULL;
	static Uint8*	s_coverage_image = NULL;


	void	finish_current_texture()
	{
		if (s_current_bitmap_info == NULL)
		{
			return;
		}

			//xxxxxx debug hack -- dump image data to a file
			FILE*	fp = fopen("dump.ppm", "wb");
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


		render::set_alpha_image(s_current_bitmap_info,
					GLYPH_CACHE_TEXTURE_SIZE,
					GLYPH_CACHE_TEXTURE_SIZE,
					s_current_cache_image);

		s_current_bitmap_info = NULL;
	}


	bool	pack_rectangle(int* px, int* py, int width, int height)
	// Find a spot for the rectangle in the current cache image.
	// Return true if there's a spot; false if there's no room.
	{
		// Really dumb implementation.  Just search for a fit.

		// Width/height coords, scaled down to the coverage map.
		int	round_up_add = (1 << GLYPH_PACK_ROUNDING_BITS) - 1;
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

		// Round up to nearest rounding boundary.
		static const int	mask = (1 << GLYPH_PACK_ROUNDING_BITS) - 1;
		int	width = (raw_width + mask) & ~mask;
		int	height = (raw_height + mask) & ~mask;

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
				for (int j = 0; j < height; j++)
				{
					memcpy(s_current_cache_image + (pack_y + j) * GLYPH_CACHE_TEXTURE_SIZE + pack_x,
					       image_data + (min_y + j) * GLYPH_FINAL_SIZE + min_x,
					       width);
				}

				// Fill out the glyph info.
				texture_glyph*	tg = new texture_glyph;
				tg->m_bitmap_info = s_current_bitmap_info;
				tg->m_uv_origin.m_x = (float(pack_x) - min_x + offset_x) / (GLYPH_CACHE_TEXTURE_SIZE - 1);
				tg->m_uv_origin.m_y = (float(pack_y) - min_y + offset_y) / (GLYPH_CACHE_TEXTURE_SIZE - 1);
				tg->m_uv_bounds.m_x_min = float(pack_x) / (GLYPH_CACHE_TEXTURE_SIZE - 1);
				tg->m_uv_bounds.m_x_max = float(pack_x + raw_width - 1) / (GLYPH_CACHE_TEXTURE_SIZE - 1);
				tg->m_uv_bounds.m_y_min = float(pack_y) / (GLYPH_CACHE_TEXTURE_SIZE - 1);
				tg->m_uv_bounds.m_y_max = float(pack_y + raw_height - 1) / (GLYPH_CACHE_TEXTURE_SIZE - 1);

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


	static texture_glyph*	make_texture_glyph(const shape_character* sh)
	// Render the given outline shape into a cached font texture.
	// Return a new texture_glyph struct that gives enough info to
	// render the cached glyph as a textured quad.
	{
		assert(sh);

		//
		// Render the shape.
		//

		// Clear the render output to 0.
		Uint8*	render_buffer = gameswf::render::get_software_mode_buffer();
		memset(render_buffer, 0, GLYPH_RENDER_SIZE * GLYPH_RENDER_SIZE);

		// Look at glyph bounds; adjust origin to make sure
		// the shape will fit in our output.
		float	offset_x = 0.f;
		float	offset_y = 1024.f;
		rect	glyph_bounds;
		get_shape_bounds(&glyph_bounds, sh);
		if (glyph_bounds.m_x_min < 0)
		{
			offset_x = - glyph_bounds.m_x_min;
		}
		if (glyph_bounds.m_y_max > 0)
		{
			offset_y = 1024 - glyph_bounds.m_y_max;
		}

		matrix	mat;
		mat.set_identity();
		mat.concatenate_scale(GLYPH_RENDER_SIZE / 1024.0f);
		mat.concatenate_translation(offset_x, offset_y);

		// Draw the shape.
		render::push_apply_matrix(mat);
		display_shape_character_in_white(sh);
		render::pop_matrix();

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
						Uint8	texel = render_buffer[
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
				    offset_x / 1024.0f * GLYPH_FINAL_SIZE,
				    offset_y / 1024.0f * GLYPH_FINAL_SIZE);
	}


	static void	generate_font_bitmaps(font* f)
	// Build cached textured versions of the font's glyphs, and
	// store them in the font.
	{
		assert(f);

		int	count = f->get_glyph_count();
		for (int i = 0; i < count; i++)
		{
			if (f->get_texture_glyph(i) == NULL)
			{
				// Create a texture for this glyph.

				shape_character*	sh = f->get_glyph(i);
				if (sh)
				{
					texture_glyph*	tg = make_texture_glyph(sh);
					f->add_texture_glyph(i, tg);
				}
			}
		}
	}


	//
	// Public interface
	//


	void	generate_font_bitmaps()
	// Build cached textures from glyph outlines.
	{
		gameswf::render::software_mode_enable(GLYPH_RENDER_SIZE, GLYPH_RENDER_SIZE);

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

		// Clean up the rendering context that we just used.
		gameswf::render::software_mode_disable();
	}


	void	save_cached_font_data(const char* filename)
	{
	}
	

	void	load_cached_font_data(const char* filename)
	{
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


	void	draw_glyph(const texture_glyph* tg, rgba color)
	// Draw the given texture glyph using current render
	// transforms, in the given color.
	{
		assert(tg);
		assert(tg->m_bitmap_info);

		rect	bounds = tg->m_uv_bounds;
		bounds.m_x_min -= tg->m_uv_origin.m_x;
		bounds.m_x_max -= tg->m_uv_origin.m_x;
		bounds.m_y_min -= tg->m_uv_origin.m_y;
		bounds.m_y_max -= tg->m_uv_origin.m_y;

		// Scale from uv coords to the 1024x1024 glyph square.
		float	scale = 256.0f * 1024.0f / GLYPH_FINAL_SIZE;

		bounds.m_x_min *= scale;
		bounds.m_x_max *= scale;
		bounds.m_y_min *= scale;
		bounds.m_y_max *= scale;
		
		render::draw_bitmap(tg->m_bitmap_info, bounds, tg->m_uv_bounds, color);
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
