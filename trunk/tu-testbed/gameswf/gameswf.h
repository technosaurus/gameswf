// gameswf.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) player library.  The info for this came from
// http://www.openswf.org, the flashsource project, and swfparse.cpp


#ifndef GAMESWF_H
#define GAMESWF_H


class tu_file;
class render_handler;

#include "gameswf_types.h"
#include "base/image.h"

namespace gameswf
{
	struct character;
	struct execute_tag;
	struct rgba;
	
	//
	// This is the client program's interface to a movie.
	//
	struct movie_interface
	{
		virtual ~movie_interface() {}
		
		virtual int	get_width() = 0;
		virtual int	get_height() = 0;
		virtual int	get_current_frame() const = 0;
		virtual int	get_frame_count() const = 0;
		
		// Play control.
		virtual void	restart() = 0;
		virtual void	advance(float delta_time) = 0;
		virtual void	goto_frame(int frame_number) = 0;
		virtual void	display() = 0;

		enum play_state
		{
			PLAY,
			STOP
		};
		virtual void	set_play_state(play_state s) = 0;
		virtual play_state	get_play_state() const = 0;
		
		virtual void	set_background_color(const rgba& bg_color) = 0;
		
		// Set to 0 if you don't want the movie to render its
		// background at all.  1 == full opacity.
		virtual void	set_background_alpha(float alpha) = 0;
		virtual float	get_background_alpha() const = 0;
		
		// move/scale the movie...
		virtual void	set_display_viewport(int x0, int y0, int w, int h) = 0;
		
		virtual void	notify_mouse_state(int x, int y, int buttons) = 0;
		
		// Need an event queue API here, for extracting user and movie events.
		// ...
		
		// Push replacement text into an edit-text field.  Returns
		// true if the field was found and could accept text.
		virtual bool	set_edit_text(const char* var_name, const char* new_text) = 0;

		// For caching precomputed stuff.  Generally of
		// interest to gameswf_processor and programs like it.
		virtual void	output_cached_data(tu_file* out) = 0;
		virtual void	input_cached_data(tu_file* in) = 0;
	};
	
	// Create a swf::movie_interface from the given input stream.
	movie_interface*	create_movie(tu_file* input);
	
	
	//
	// Library management
	//
	
	// Release any library movies we've cached.  Do this when you want
	// maximum cleanup.
	void	clear_library();
	
	// Register a callback to the host, for providing a file, given a
	// "URL" (i.e. a path name).  This is for supporting SWF
	// "libraries", where movies can share resources (fonts and
	// sprites) from a source movie.
	//
	// gameswf will call this when it needs to create a library movie.
	typedef tu_file* (*file_opener_function)(const char* url_or_path);
	void	register_file_opener_callback(file_opener_function opener);
	
	
	//
	// Loader callbacks.
	//
	
	// @@ move this into a more private header
	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	struct stream;
	struct movie;
	typedef void (*loader_function)(stream* input, int tag_type, movie* m);
	void	register_tag_loader(int tag_type, loader_function lf);
	
	
	//
	// Log & error reporting control.
	//
	
	// Supply a function pointer to receive log & error messages.
	void	set_log_callback(void (*callback)(bool error, const char* message));
	
	// Control verbosity of specific categories.
	void	set_verbose_action(bool verbose);
	void	set_verbose_parse(bool verbose);
	
	//
	// Font library control; gameswf shares its fonts among all loaded
	// movies!
	//
	
	struct font;
	namespace fontlib
	{
		// Builds cached glyph textures from shape info.
		void	generate_font_bitmaps();
		
		// Save cached glyph textures to a stream.  This might be used by an
		// offline tool, which loads in all movies, calls
		// generate_font_bitmaps(), and then calls save_cached_font_data()
		// so that a run-time app can call load_cached_font_data().
		void	save_cached_font_data(tu_file* out);
		
		// Load a stream containing previously-saved font glyph textures.
		bool	load_cached_font_data(tu_file* in);
		
		// For accessing the fonts in the library.
		int	get_font_count();
		font*	get_font(int index);
		font*	get_font(const char* name);
		const char*	get_font_name(const font* f);
		
		// @@ also need to add color controls (or just set the diffuse color
		// in the API?), perhaps matrix xform, and maybe spacing, etc.
		//
		//		// For direct text rendering from the host app.
		void	draw_string(const font* f, float x, float y, float size, const char* text);
		//		void	draw_string(const font* f, float x, float y, float size, const wchar* text);	// wide-char version
	}
	
	
	//
	// Sound callback handler.
	//
	
	// You may define a subclass of this, and pass an instance to
	// set_sound_handler().
	struct sound_handler
	{
		enum format_type
		{
			FORMAT_RAW = 0,		// unspecified format.  Useful for 8-bit sounds???
				FORMAT_ADPCM = 1,	// gameswf doesn't pass this through; it uncompresses and sends FORMAT_NATIVE16
				FORMAT_MP3 = 2,
				FORMAT_UNCOMPRESSED = 3,	// 16 bits/sample, little-endian
				FORMAT_NELLYMOSER = 6,	// Mystery proprietary format; see nellymoser.com
				
				// gameswf tries to convert data to this format when possible:
				FORMAT_NATIVE16 = 7	// gameswf extension: 16 bits/sample, native-endian
		};
		// If stereo is true, samples are interleaved w/ left sample first.
		
		// gameswf calls at load-time with sound data, to be
		// played later.  You should create a sample with the
		// data, and return a handle that can be used to play
		// it later.  If the data is in a format you can't
		// deal with, then you can return 0 (for example), and
		// then ignore 0's in play_sound() and delete_sound().
		//
		// Assign handles however you like.
		virtual int	create_sound(
			void* data,
			int data_bytes,
			int sample_count,
			format_type format,
			int sample_rate,	/* one of 5512, 11025, 22050, 44100 */
			bool stereo
			) = 0;
		
		// gameswf calls this when it wants you to play the defined sound.
		//
		// loop_count == 0 means play the sound once (1 means play it twice, etc)
		virtual void	play_sound(int sound_handle, int loop_count /* , volume, pan, etc? */) = 0;
		
		// Stop the specified sound if it's playing.
		// (Normally a full-featured sound API would take a
		// handle specifying the *instance* of a playing
		// sample, but SWF is not expressive that way.)
		virtual void	stop_sound(int sound_handle) = 0;
		
		// gameswf calls this when it's done with a particular sound.
		virtual void	delete_sound(int sound_handle) = 0;
		
		virtual ~sound_handler(){};
	};
	
	// Pass in a sound handler, so you can handle audio on behalf of
	// gameswf.  This is optional; if you don't set a handler, or set
	// NULL, then sounds will be ignored.
	//
	// If you want sound support, you should set this at startup,
	// before loading or playing any movies!
	void	set_sound_handler(sound_handler* s);
	

	//
	// matrix type, used by render handler
	//

	struct point;
	struct matrix
	{
		float	m_[2][3];

		static matrix	identity;

		matrix();
		void	set_identity();
		void	concatenate(const matrix& m);
		void	concatenate_translation(float tx, float ty);
		void	concatenate_scale(float s);
		void	read(stream* in);
		void	print() const;
		void	transform(point* result, const point& p) const;
		void	transform_vector(point* result, const point& p) const;
		void	transform_by_inverse(point* result, const point& p) const;
		void	set_inverse(const matrix& m);
		bool	does_flip() const;	// return true if we flip handedness
		float	get_max_scale() const;	// return the maximum scale factor that this transform applies
	};
	

	//
	// cxform: color transform type, used by render handler
	//
	struct cxform
	{
		float	m_[4][2];	// [RGBA][mult, add]

		cxform();
		void	concatenate(const cxform& c);
		rgba	transform(const rgba in) const;
		void	read_rgb(stream* in);
		void	read_rgba(stream* in);
	};


	//
	// texture and render callback handler.
	//
	
	// You must define a subclass of bitmap info and render_handler, and pass an instance to
	// set_render_handler().
	struct bitmap_info
	{
		unsigned int	m_texture_id;
		int	m_original_width;
		int	m_original_height;
		
		bitmap_info() 
			:
		m_texture_id(0),
			m_original_width(0),
			m_original_height(0){}
		
		virtual void	set_alpha_image(int width, int height, Uint8* data) = 0;
		
		enum create_empty
		{
			empty
		};
		
		virtual ~bitmap_info(){};
	};
	
	struct render_handler
	{
		virtual bitmap_info*	create_bitmap_info(image::rgb* im) = 0;
		virtual bitmap_info*	create_bitmap_info(image::rgba* im) = 0;
		virtual bitmap_info*	create_bitmap_info_blank() = 0;
		virtual void	set_alpha_image(bitmap_info* bi, int w, int h, Uint8* data) = 0;	// @@ munges *data!!!
		virtual void	delete_bitmap_info(bitmap_info* bi) = 0;
		
		// Bracket the displaying of a frame from a movie.
		// Fill the background color, and set up default
		// transforms, etc.
		virtual void	begin_display(
			rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1) = 0;
		virtual void	end_display() = 0;
		
		// Geometric and color transforms for mesh and line_strip rendering.
		virtual void	set_matrix(const matrix& m) = 0;
		virtual void	set_cxform(const cxform& cx) = 0;
		
		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-list order.  The type of the array should
		// be float[vertex_count*2]
		virtual void	draw_mesh(const float coords[], int vertex_count) = 0;
		
		// Draw a line-strip using the current line style.
		// Clear the style list after rendering.
		//
		// Coords is a list of (x,y) coordinate pairs, in
		// sequence.
		virtual void	draw_line_strip(const float coords[], int vertex_count) = 0;
		
		// Set line and fill styles for mesh & line_strip
		// rendering.
		enum bitmap_wrap_mode
		{
			WRAP_REPEAT,
				WRAP_CLAMP
		};
		virtual void	fill_style_disable(int fill_side) = 0;
		virtual void	fill_style_color(int fill_side, rgba color) = 0;
		virtual void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm) = 0;
		
		virtual void	line_style_disable() = 0;
		virtual void	line_style_color(rgba color) = 0;
		virtual void	line_style_width(float width) = 0;
		
		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Ignores
		// current transforms.
		virtual void	draw_bitmap(const matrix& m, const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color) = 0;
		
		virtual void	set_antialiased(bool enable) = 0;
		
		
		virtual void begin_submit_mask() = 0;
		virtual void end_submit_mask() = 0;
		virtual void disable_mask() = 0;
	};
	
	// Get and set the render handler. this is one of the first things you should do to initialise
	// the player.
	extern void set_render_handler(render_handler* s);
	
}	// namespace gameswf


#endif // GAMESWF_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
