// gameswf.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) player library.  The info for this came from
// http://www.openswf.org, the flashsource project, and swfparse.cpp


#ifndef GAMESWF_H
#define GAMESWF_H


#include "SDL.h"


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

		virtual void	notify_mouse_state(int x, int y, int buttons) = 0;

		// Need an event queue API here, for extracting user and movie events.
	};

	// Create a swf::movie_interface from the given input stream.
	movie_interface*	create_movie(SDL_RWops* input);

	//
	// Loader callbacks.
	//

	struct stream;
	struct movie;
	typedef void (*loader_function)(stream* input, int tag_type, movie* m);

	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	void	register_tag_loader(int tag_type, loader_function lf);

	//
	// Render control.
	//

	void	set_antialiased(bool enable);
	void	set_pixel_scale(float scale);


	//
	// Font library control; gameswf shares its fonts among all loaded
	// movies!
	//

	namespace fontlib
	{
		struct font;

		// Builds cached glyph textures from shape info.
		void	generate_font_bitmaps();

		// Save cached glyph textures to a file.  This might be used by an
		// offline tool, which loads in all movies, calls
		// generate_font_bitmaps(), and then calls save_cached_font_data()
		// so that a run-time app can call load_cached_font_data().
		void	save_cached_font_data(const char* filename);

		// Load a file containing previously-saved font glyph textures.
		void	load_cached_font_data(const char* filename);

		// For accessing the fonts in the library.
		int	get_font_count();
		font*	get_font(int index);
		font*	get_font(const char* name);
		const char*	get_font_name(const font* f);

// @@ also need to add color controls (or just set the diffuse color
// in the API?), perhaps matrix xform, and maybe spacing, etc.
//
//		// For direct text rendering from the host app.
//		void	draw_string(const font* f, float x, float y, float size, const char* text);
//		void	draw_string(const font* f, float x, float y, float size, const wchar* text);	// wide-char version
	};

};	// namespace gameswf


#endif // GAMESWF_H

