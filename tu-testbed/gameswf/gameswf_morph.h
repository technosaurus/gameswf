// gameswf_morph.h -- Mike Shaver <shaver@off.net> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.
//
// Morph directives for shape tweening.

#ifndef GAMESWF_MORPH_H
#define GAMESWF_MORPH_H

#include "gameswf_shape.h"
#include "gameswf_styles.h"

namespace gameswf
{
	struct morph_fill_style
	{
		morph_fill_style();
		morph_fill_style(stream* in, movie_definition_sub* m);
		~morph_fill_style();
		
		void read(stream* in, movie_definition_sub* m);
		rgba sample_gradient(int ratio, float morph);
		bitmap_info* create_gradient_bitmap(float morph) const;
		void apply(int fill_side, float morph);
		rgba get_color(float morph) const;
		void set_colors(rgba new_color_orig, rgba new_color_target);
	private:
		int m_type;
		rgba m_color[2];
		matrix m_gradient_matrix[2];
		array<gradient_record> m_gradients[2];
		bitmap_info* m_gradient_bitmap_info[2];
		bitmap_character_def* m_bitmap_character;
		matrix m_bitmap_matrix[2];
	};

	struct morph_line_style
	{
		morph_line_style();
		morph_line_style(stream* in);

		void read(stream* in);
		void apply(float morph);
		
	private:
		Uint16 m_width[2];
		rgba   m_color[2];
	};

	struct morph_path
	{
		morph_path();
		morph_path(float ax, float ay, int fill0, int fill1, int line);
		bool is_empty() const { return m_edges[0].size() == 0; }

		int m_fill0, m_fill1, m_line;
		float m_ax[2], m_ay[2];
		array<edge> m_edges[2];
		bool m_new_shape;
	};

        struct shape_morph_def : public character_def
        {
                shape_morph_def();
                virtual ~shape_morph_def();
                virtual void display(character *instance_info);
                void read(stream* in, int tag_type, bool with_style,
			  movie_definition_sub* m);

        private:
		void read_edge(stream* in, edge& e, float& x, float& y);
		int read_shape_record(stream* in, movie_definition_sub* m,
				      bool start);

		rect	m_bound_orig, m_bound_target;
		array<morph_fill_style> m_fill_styles;
		array<morph_line_style> m_line_styles;
		array<morph_path> m_paths;
        };
}

#endif /* GAMESWF_MORPH_H */
// Local Variables:
// mode: C++
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
