// gameswf.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) player library.  The info for this came from
// http://www.openswf.org, the flashsource project, and swfparse.cpp


#ifndef GAMESWF_H
#define GAMESWF_H


class tu_file;


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
	// Render control.
	//

	void	set_antialiased(bool enable);


	//
	// Font library control; gameswf shares its fonts among all loaded
	// movies!
	//

	struct font;
	namespace fontlib
	{
		// Builds cached glyph textures from shape info.
		void	generate_font_bitmaps();

		// Save cached glyph textures to a file.  This might be used by an
		// offline tool, which loads in all movies, calls
		// generate_font_bitmaps(), and then calls save_cached_font_data()
		// so that a run-time app can call load_cached_font_data().
		void	save_cached_font_data(const char* filename);

		// Load a file containing previously-saved font glyph textures.
		bool	load_cached_font_data(const char* filename);

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
			FORMAT_ADPCM = 1,
			FORMAT_MP3 = 2,
			FORMAT_UNCOMPRESSED = 3,	// 16 bits/sample, little-endian
			FORMAT_NELLYMOSER = 6,	// Mystery proprietary format; see nellymoser.com
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
		virtual void	play_sound(int sound_handle /* , volume, pan, etc? */) = 0;

		// gameswf calls this when it's done with a particular sound.
		virtual void	delete_sound(int sound_handle) = 0;

	};

	// Pass in a sound handler, so you can handle audio on behalf of
	// gameswf.  This is optional; if you don't set a handler, or set
	// NULL, then sounds will be ignored.
	//
	// If you want sound support, you should set this at startup,
	// before loading or playing any movies!
	void	set_sound_handler(sound_handler* s);

	// Utility function to uncompress ADPCM.  Output data is
	// actually SIGNED 16-BIT INTEGER, native endianness.
	// data_out must have enough room for sample_count*2 bytes in
	// mono, or sample_count*4 bytes in stereo.
	void	adpcm_expand(
		void* data_out,
		const void* data_in,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo);

}	// namespace gameswf


#endif // GAMESWF_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
