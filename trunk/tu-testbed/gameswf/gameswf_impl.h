// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"


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
	struct sound_sample { virtual ~sound_sample() {} };
	struct stream;


	// Extra internal interfaces added to movie_definition
	struct movie_definition_sub : public movie_definition
	{
		virtual const array<execute_tag*>&	get_playlist(int frame_number) = 0;
		virtual resource*	get_exported_resource(const tu_string& symbol) = 0;
		virtual character_def*	get_character_def(int id) = 0;

		virtual bool	get_labeled_frame(const char* label, int* frame_number) = 0;


		// For use during creation.
		virtual int	get_loading_frame() const = 0;
		virtual void	add_character(int id, character_def* ch) = 0;
		virtual void	add_font(int id, font* ch) = 0;
		virtual font*	get_font(int id) = 0;
		virtual void	add_execute_tag(execute_tag* c) = 0;
		virtual void	add_frame_name(const char* name) = 0;
		virtual void	set_jpeg_loader(jpeg::input* j_in) = 0;
		virtual jpeg::input*	get_jpeg_loader() = 0;
		virtual bitmap_character_def*	get_bitmap_character(int character_id) = 0;
		virtual void	add_bitmap_character(int character_id, bitmap_character_def* ch) = 0;
		virtual sound_sample*	get_sound_sample(int character_id) = 0;
		virtual void	add_sound_sample(int character_id, sound_sample* sam) = 0;
		virtual void	export_resource(const tu_string& symbol, resource* res) = 0;

		virtual void	generate_font_bitmaps() = 0;
	};


	// For internal use.
	movie_definition_sub*	create_movie_sub(const char* filename);
	movie_definition_sub*	create_library_movie_sub(const char* filename);


	struct movie : public movie_interface, public as_object_interface
	{
		virtual movie_definition*	get_movie_definition() { return NULL; }
		virtual movie_root*	get_root() { return NULL; }
		virtual movie*	get_root_movie() { return NULL; }

		virtual float	get_pixel_scale() const { return 1.0f; }
		virtual character*	get_character(int id) { return NULL; }

		virtual matrix	get_world_matrix() const { return matrix::identity; }
		virtual cxform	get_world_cxform() const { return cxform::identity; }

		//
		// display-list management.
		//

		virtual void	add_display_object(Uint16 character_id,
						   const char* name,
						   Uint16 depth,
						   const cxform& color_transform,
						   const matrix& mat,
						   float ratio,
                                                   Uint16 clip_depth)
		{
		}

		virtual void	move_display_object(
			Uint16 depth,
			bool use_cxform,
			const cxform& color_transform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth)
		{
		}

		virtual void	replace_display_object(
			Uint16 character_id,
			const char* name,
			Uint16 depth,
			bool use_cxform,
			const cxform& color_transform,
			bool use_matrix,
			const matrix& mat,
			float ratio,
			Uint16 clip_depth)
		{
		}

		virtual void	remove_display_object(Uint16 depth)
		{
		}

		virtual void	set_background_color(const rgba& color) {}
		virtual void	set_background_alpha(float alpha) {}
		virtual float	get_background_alpha() const { return 1.0f; }
		virtual void	set_display_viewport(int x0, int y0, int width, int height) {}

		virtual void	add_action_buffer(action_buffer* a) { assert(0); }

		virtual void	goto_frame(int target_frame_number) { assert(0); }
		virtual void	goto_labeled_frame(const char* label) { assert(0); }

		virtual void	set_play_state(play_state s) {}
		virtual play_state	get_play_state() const { assert(0); return STOP; }

		virtual void	notify_mouse_state(int x, int y, int buttons)
		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		{
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		// Use this to retrieve the last state of the mouse, as set via
		// notify_mouse_state().
		{
			assert(0);
		}

		virtual int	get_mouse_capture() { assert(0); return -1; }
		virtual void	set_mouse_capture(int id) { assert(0); }

		virtual bool	set_edit_text(const char* var_name, const char* new_text)
		{
			// @@ this should be implemented more directly, to return
			// a proper true/false result on success/failure.
			set_member(var_name, new_text);
			return true;
		}

		virtual bool	has_looped() const { return true; }

		virtual void	start_drag(
			movie* target,
			bool lock_center,
			bool rect_bound,
			float x0,
			float y0,
			float x1,
			float y1)
		{
			assert(0);
		}
		virtual void	stop_drag() { assert(0); }


		//
		// ActionScript.
		//


// use get/set_member...
#if 0
		virtual bool	set_value(const char* var_name, const as_value& val)
		{
			assert(0);
			return false;
		}

		virtual as_value	get_value(const char* var_name)
		{
			assert(0);
			return as_value(as_value::UNDEFINED);
		}

		virtual bool	get_character_value(const char* var_name, as_value* val)
		{
			assert(0);
			return false;
		}
#endif // 0

		virtual movie*	get_relative_target(const tu_string& name)
		{
			assert(0);	
			return NULL;
		}

#if 0
		virtual as_value	get_property(int prop_number)
		{
			assert(0);
			return as_value(as_value::UNDEFINED);
		}

		virtual void	set_property(int prop_number, const as_value& new_val)
		{
			assert(0);
		}
#endif // 0

		// as_object_interface stuff
		virtual bool	set_self_value(const as_value& val) { assert(0); return false; }
		virtual bool	get_self_value(as_value* val) { assert(0); return false; }
		virtual void	set_member(const tu_string& name, const as_value& val) { assert(0); }
		virtual bool	get_member(const tu_string& name, as_value* val) { assert(0); return false; }
		virtual as_value	call_method(const tu_string& name, as_environment* env, int nargs, int arg0)
		{
			assert(0);
			return as_value();
		}

		virtual float	get_timer() const { return 0.0f; }
		virtual movie*	to_movie() { return this; }
	};


	// A character_def is the immutable data representing the template of a
	// movie element.
	struct character_def
	{
	private:
		int	m_id;
		
	public:
		character_def()
			:
			m_id(-1)
		{
		}

		virtual ~character_def() {}

		void	set_id(int id) { m_id = id; }
		int	get_id() const { return m_id; }

		virtual void	display(character* instance_info) {}
		virtual bool	point_test_local(float x, float y) { return false; }

		virtual character*	create_character_instance(movie* parent);	// default is to make a generic_character

		//
		// Caching.
		//

		virtual void	output_cached_data(tu_file* out) {}
		virtual void	input_cached_data(tu_file* in) {}
	};


	// Information about how to display a character.
	struct display_info
	{
		movie*	m_parent;
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
                Uint16 	m_clip_depth;

		display_info()
			:
			m_parent(NULL),
			m_depth(0),
			m_ratio(0.0f),
                        m_clip_depth(0)
		{
		}

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			m_depth = di.m_depth;
			m_color_transform.concatenate(di.m_color_transform);
			m_matrix.concatenate(di.m_matrix);
			m_ratio = di.m_ratio;
                        m_clip_depth = di.m_clip_depth;
		}
	};


	// character is a live, stateful instance of a character_def.
	// It represents a single active element in a movie.
	struct character : public movie
	{
		// @@ stuff from display_info
		movie*	m_parent;
		tu_string	m_name;
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
                Uint16 	m_clip_depth;

		character(movie* parent)
			:
			m_parent(parent),
			m_depth(-1),
			m_ratio(0.0f),
			m_clip_depth(0)
		{
		}

		// Accessors for basic display info.
		movie*	get_parent() const { return m_parent; }
		int	get_depth() const { return m_depth; }
		void	set_depth(int d) { m_depth = d; }
		const matrix&	get_matrix() const { return m_matrix; }
		void	set_matrix(const matrix& m) { m_matrix = m; }
		const cxform&	get_cxform() const { return m_color_transform; }
		void	set_cxform(const cxform& cx) { m_color_transform = cx; }
		void	concatenate_cxform(const cxform& cx) { m_color_transform.concatenate(cx); }
		void	concatenate_matrix(const matrix& m) { m_matrix.concatenate(m); }
		float	get_ratio() const { return m_ratio; }
		void	set_ratio(float f) { m_ratio = f; }
		Uint16	get_clip_depth() const { return m_clip_depth; }
		void	set_clip_depth(Uint16 d) { m_clip_depth = d; }

		virtual int	get_id() const = 0;

		void	set_name(const char* name) { m_name = name; }
		const tu_string&	get_name() const { return m_name; }

		virtual const char*	get_variable_name() const { return get_name().c_str(); }

		virtual matrix	get_world_matrix() const
		// Get our concatenated matrix (all our ancestor transforms, times our matrix).  Maps
		// from our local space into "world" space (i.e. root movie space).
		{
			matrix	m;
			if (m_parent)
			{
				m = m_parent->get_world_matrix();
			}
			m.concatenate(get_matrix());

			return m;
		}

		virtual cxform	get_world_cxform() const
		// Get our concatenated color transform (all our ancestor transforms,
		// times our cxform).  Maps from our local space into normal color space.
		{
			cxform	m;
			if (m_parent)
			{
				m = m_parent->get_world_cxform();
			}
			m.concatenate(get_cxform());

			return m;
		}

		// Movie interfaces.  By default do nothing.  sprite_instance and some others override these.
		virtual void	display() {}
		virtual bool	point_test(float x, float y) { return false; }	// return true if the point is inside our shape.

		virtual int	get_current_frame() const { assert(0); return 0; }
		virtual bool	has_looped() const { assert(0); return false; }
		virtual void	restart() { /*assert(0);*/ }
		virtual void	advance(float delta_time) {}	// for buttons and sprites
		virtual void	goto_frame(int target_frame) {}
	};


	// For characters that don't store unusual state in their instances.
	struct generic_character : public character
	{
		character_def*	m_def;

		generic_character(character_def* def, movie* parent)
			:
			character(parent),
			m_def(def)
		{
			assert(m_def);
		}

		virtual int	get_id() const { return m_def->get_id(); }
//		virtual const char*	get_name() const { return m_def->get_name(); }
		virtual void	display() { m_def->display(this); }	// pass in transform info
		virtual bool	point_test(float x, float y)
		{
			matrix	m = get_world_matrix();
			point	p;
			m.transform_by_inverse(&p, point(x, y));

			return m_def->point_test_local(p.m_x, p.m_y);
		}
	};



	struct bitmap_character_def : public character_def
	{
		virtual gameswf::bitmap_info*	get_bitmap_info() = 0;
	};


	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
		virtual void	execute_state(movie* m) {}
	};


	//
	// Loader callbacks.
	//
	
	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	typedef void (*loader_function)(stream* input, int tag_type, movie_definition_sub* m);
	void	register_tag_loader(int tag_type, loader_function lf);
	
	
	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m);
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
	// sound_stream_loader();	// head, head2, block


	struct texture_glyph;
	namespace fontlib
	{
		// For adding fonts.
		void	add_font(font* f);

		// For drawing a textured glyph w/ current render transforms.
		void	draw_glyph(const matrix& m, const texture_glyph* g, rgba color);

		// Return the pixel height of text, such that the
		// texture glyphs are sampled 1-to-1 texels-to-pixels.
		// I.e. the height of the glyph box, in texels.
		float	get_nominal_texture_glyph_height();
	}


}	// end namespace gameswf


#endif // GAMESWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
