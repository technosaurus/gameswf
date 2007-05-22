// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf/gameswf.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_types.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_render.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>


namespace jpeg { struct input; }


namespace gameswf
{
	struct action_buffer;
	struct bitmap_character_def;
	struct bitmap_info;
	struct character;
	struct character_def;
	struct display_info;
	struct execute_tag;
	struct font;
	struct movie_root;
	struct movie_definition_sub;

	struct sound_sample : public resource //virtual public ref_counted
	{
		virtual sound_sample*	cast_to_sound_sample() { return this; }
	};

	struct stream;
	struct swf_event;

	void save_extern_movie(movie_interface* m);


	// For internal use.
	movie_definition_sub*	create_movie_sub(const char* filename);
	movie_definition_sub*	create_library_movie_sub(const char* filename);
	movie_interface*	create_library_movie_inst_sub(movie_definition_sub* md);

	// for extern movies
	movie_interface*	create_library_movie_inst(movie_definition* md);
	movie_interface*	get_current_root();
	void set_current_root(movie_interface* m);
	const char* get_workdir();
	void set_workdir(const char* dir);

	struct swf_event
	{
		// NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
		// array<>!  They cannot be moved!  The private
		// operator=(const swf_event&) should help guard
		// against that.

		event_id	m_event;
		action_buffer	m_action_buffer;
		as_value	m_method;

		swf_event()
		{
		}

		void	attach_to(character* ch) const
		{
			ch->set_event_handler(m_event, m_method);
		}

	private:
		// DON'T USE THESE
		swf_event(const swf_event& s) { assert(0); }
		void	operator=(const swf_event& s) { assert(0); }
	};


	// For characters that don't store unusual state in their instances.
	struct generic_character : public character
	{
		character_def*	m_def;

		generic_character(character_def* def, character* parent, int id)
			:
		character(parent, id),
			m_def(def)
		{
			assert(m_def);
		}

		virtual void	display()
		{
			m_def->display(this);	// pass in transform info
			do_display_callback();
		}

		// @@ tulrich: these are used for finding bounds; TODO
		// need to do this using enclose_transformed_rect(),
		// not by scaling the local height/width!

		virtual float	get_height()
		{
			matrix	m = get_world_matrix();
			float	h = m_def->get_height_local();
			return h;
		}

		virtual float	get_width()
		{
			matrix	m = get_world_matrix();
			float	w = m_def->get_width_local();
			return w;
		}

		virtual character*	get_topmost_mouse_entity(float x, float y)
		{
			assert(get_visible());	// caller should check this.

			matrix	m = get_matrix();
			point	p;
			m.transform_by_inverse(&p, point(x, y));

			if (m_def->point_test_local(p.m_x, p.m_y))
			{
				// The mouse is inside the shape.
				return this;
			}
			return NULL;
		}
	};


	struct bitmap_character_def : public character_def
	{
		virtual gameswf::bitmap_info*	get_bitmap_info() = 0;
	};

	// Bitmap character
	struct bitmap_character : public bitmap_character_def
	{
		bitmap_character(bitmap_info* bi) :	m_bitmap_info(bi)
		{
		}

		virtual float	get_width_local()
		{
			return PIXELS_TO_TWIPS(m_bitmap_info->m_original_width);
		}

		virtual float	get_height_local()
		{
			return PIXELS_TO_TWIPS(m_bitmap_info->m_original_height);
		}

		bool	point_test_local(float x, float y)
		// Return true if the specified point is on the interior of our shape.
		// Incoming coords are local coords.
		{
			rect coords;
			coords.m_x_min  = 0.0f;
			coords.m_x_max  = PIXELS_TO_TWIPS(m_bitmap_info->m_original_width);
			coords.m_y_min  = 0.0f;
			coords.m_y_max  = PIXELS_TO_TWIPS(m_bitmap_info->m_original_height);
			if (coords.point_test(x, y))
			{
				return true;
			}
			return false;
		}

		virtual void	display(character* ch)
		{
			rect coords;
			coords.m_x_min  = 0.0f;
			coords.m_x_max  = PIXELS_TO_TWIPS(m_bitmap_info->m_original_width);
			coords.m_y_min  = 0.0f;
			coords.m_y_max  = PIXELS_TO_TWIPS(m_bitmap_info->m_original_height);

			// show whole picture
			rect uv_coords;
			uv_coords.m_x_min  = 0.0f;
			uv_coords.m_x_max  = 1.0f;
			uv_coords.m_y_min  = 0.0f;
			uv_coords.m_y_max  = 1.0f;

			cxform cx = ch->get_world_cxform();
			rgba color = cx.transform(gameswf::rgba());
			matrix m = ch->get_world_matrix();
			render::draw_bitmap(m, m_bitmap_info.get_ptr(), coords,	uv_coords, color);
		}

		gameswf::bitmap_info*	get_bitmap_info()
		{
			return m_bitmap_info.get_ptr();
		}

		private:
			smart_ptr<gameswf::bitmap_info>	m_bitmap_info;
	};

	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(character* m) {}
		virtual void	execute_state(character* m) {}
		virtual void	execute_state_reverse(character* m, int frame) { execute_state(m); }
		virtual bool	is_remove_tag() const { return false; }
		virtual bool	is_action_tag() const { return false; }
		virtual uint32	get_depth_id_of_replace_or_add_tag() const { return static_cast<uint32>(-1); }
	};

	//
	// Loader callbacks.
	//

	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	void	register_tag_loader(int tag_type, loader_function lf);


	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	end_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	export_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	import_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	start_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_video_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	video_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sound_stream_head_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sound_stream_block_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_file_attribute_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_alignzones(stream* in, int tag_type, movie_definition_sub* m);
	void	define_csm_textsetting_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_metadata_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_enable_debugger_loader(stream* in, int tag_type, movie_definition_sub* m);

}	// end namespace gameswf


#endif // GAMESWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
