// gameswf_styles.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Fill and line style types.


#include "gameswf_styles.h"
#include "gameswf_impl.h"
#include "gameswf_render.h"
#include "gameswf_stream.h"


namespace gameswf
{
	//
	// gradient_record
	//

	gradient_record::gradient_record()
		:
		m_ratio(0)
	{
	}


	void	gradient_record::read(stream* in, int tag_type)
	{
		m_ratio = in->read_u8();
		m_color.read(in, tag_type);
	}


	//
	// fill_style
	//


	fill_style::fill_style()
		:
		m_type(0),
		m_gradient_bitmap_info(0),
		m_bitmap_character(0)
	{
		assert(m_gradients.size() == 0);
	}


	void	fill_style::read(stream* in, int tag_type, movie* m)
	{
		m_type = in->read_u8();

		IF_DEBUG(printf("fsr type = 0x%X\n", m_type));

		if (m_type == 0x00)
		{
			// 0x00: solid fill
			if (tag_type <= 22) {
				m_color.read_rgb(in);
			} else {
				m_color.read_rgba(in);
			}

			IF_DEBUG(printf("fsr color: ");
					 m_color.print(stdout));
		}
		else if (m_type == 0x10 || m_type == 0x12)
		{
			// 0x10: linear gradient fill
			// 0x12: radial gradient fill

			matrix	input_matrix;
			input_matrix.read(in);

			if (m_type == 0x10)
			{
				m_gradient_matrix.set_identity();
				m_gradient_matrix.concatenate_translation(128.f, 0.f);
				m_gradient_matrix.concatenate_scale(1.0f / 128.0f);
			}
			else 
			{
				m_gradient_matrix.set_identity();
				m_gradient_matrix.concatenate_translation(32.f, 32.f);
				m_gradient_matrix.concatenate_scale(1.0f / 512.0f);
			}


			matrix	m;
			m.set_inverse(input_matrix);
			m_gradient_matrix.concatenate(m);
				
			// GRADIENT
			int	num_gradients = in->read_u8();
			assert(num_gradients >= 1 && num_gradients <= 8);
			m_gradients.resize(num_gradients);
			for (int i = 0; i < num_gradients; i++)
			{
				m_gradients[i].read(in, tag_type);
			}

			IF_DEBUG(printf("fsr: num_gradients = %d\n", num_gradients));

			// @@ hack.
			if (num_gradients > 0)
			{
				m_color = m_gradients[0].m_color;
			}
		}
		else if (m_type == 0x40 || m_type == 0x41)
		{
			// 0x40: tiled bitmap fill
			// 0x41: clipped bitmap fill

			int	bitmap_char_id = in->read_u16();
			IF_DEBUG(printf("fsr: bitmap_char = %d\n", bitmap_char_id));

			// Look up the bitmap character.
			m_bitmap_character = m->get_bitmap_character(bitmap_char_id);

			matrix	m;
			m.read(in);

			// For some reason, it looks like they store the inverse of the
			// TWIPS-to-texcoords matrix.
			m_bitmap_matrix.set_inverse(m);
			IF_DEBUG(m_bitmap_matrix.print(stdout));
		}
	}


	rgba	fill_style::sample_gradient(int ratio) const
	// Return the color at the specified ratio into our gradient.
	// Ratio is in [0, 255].
	{
		assert(ratio >= 0 && ratio <= 255);
		assert(m_type == 0x10 || m_type == 0x12);
		assert(m_gradients.size() > 0);

		if (ratio < m_gradients[0].m_ratio)
			return m_gradients[0].m_color;		
		
		for (int i = 1; i < m_gradients.size(); i++)
		{
			if (m_gradients[i].m_ratio >= ratio)
			{
				float	f = (ratio - m_gradients[i - 1].m_ratio) / float(m_gradients[i].m_ratio - m_gradients[i - 1].m_ratio);

				rgba	result;
				result.set_lerp(m_gradients[i - 1].m_color, m_gradients[i].m_color, f);
				return result;
			}
		}
		return m_gradients.back().m_color;
	}

	gameswf::render::bitmap_info*	fill_style::create_gradient_bitmap() const
	// Make a bitmap_info* corresponding to our gradient.
	// We can use this to set the gradient fill style.
	{
		assert(m_type == 0x10 || m_type == 0x12);

		image::rgba*	im = NULL;

		if (m_type == 0x10)
		{
			// Linear gradient.
			im = image::create_rgba(256, 1);

			for (int i = 0; i < im->m_width; i++)
			{
				rgba	sample = sample_gradient(i);
				im->set_pixel(i, 0, sample.m_r, sample.m_g, sample.m_b, sample.m_a);
			}
		}
		else if (m_type == 0x12)
		{
			// Radial gradient.
			im = image::create_rgba(64, 64);

			for (int j = 0; j < im->m_height; j++)
			{
				for (int i = 0; i < im->m_width; i++)
				{
					float	radius = (im->m_height - 1) / 2.0f;
					float	y = (j - radius) / radius;
					float	x = (i - radius) / radius;
					int	ratio = (int) floorf(255.5f * sqrt(x * x + y * y));
					if (ratio > 255)
					{
						ratio = 255;
					}
					rgba	sample = sample_gradient( ratio );
					im->set_pixel(i, j, sample.m_r, sample.m_g, sample.m_b, sample.m_a);
				}
			}
		}

		gameswf::render::bitmap_info*	bi = gameswf::render::create_bitmap_info(im);
		delete im;

		return bi;
	}


	void	fill_style::apply(int fill_side) const
	// Push our style parameters into the renderer.
	{
		if (m_type == 0x00)
		{
			// 0x00: solid fill
			gameswf::render::fill_style_color(fill_side, m_color);
		}
		else if (m_type == 0x10 || m_type == 0x12)
		{
			// 0x10: linear gradient fill
			// 0x12: radial gradient fill

			if (m_gradient_bitmap_info == NULL)
			{
				(const_cast<fill_style*>(this))->m_gradient_bitmap_info = create_gradient_bitmap();
			}

			if (m_gradient_bitmap_info)
			{
				gameswf::render::fill_style_bitmap(
					fill_side,
					m_gradient_bitmap_info,
					m_gradient_matrix,
					gameswf::render::WRAP_CLAMP);
			}
			else
			{
				// Hack.
				gameswf::render::fill_style_color(
					fill_side,
					m_color);
			}
		}
		else if (m_type == 0x40
				 || m_type == 0x41)
		{
			// bitmap fill (either tiled or clipped)
			gameswf::render::bitmap_info*	bi = NULL;
			if (m_bitmap_character != NULL)
			{
				bi = m_bitmap_character->get_bitmap_info();
				if (bi != NULL)
				{
					gameswf::render::bitmap_wrap_mode	wmode = gameswf::render::WRAP_REPEAT;
					if (m_type == 0x41)
					{
						wmode = gameswf::render::WRAP_CLAMP;
					}
					gameswf::render::fill_style_bitmap(
						fill_side,
						bi,
						m_bitmap_matrix,
						wmode);
				}
			}
		}
	}


	//
	// line_style
	//

	
	line_style::line_style()
		:
		m_width(0)
	{
	}


	void	line_style::read(stream* in, int tag_type)
	{
		m_width = in->read_u16();
		m_color.read(in, tag_type);
	}


	void	line_style::apply() const
	{
		gameswf::render::line_style_color(m_color);
	}

}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
