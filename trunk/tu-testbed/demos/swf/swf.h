// swf.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) file parser.  The info for this came from
// http://www.openswf.org


#ifndef SWF_H
#define SWF_H


#include "SDL.h"


namespace swf
{
	struct character;
	struct execute_tag;
	struct rgba;

	// This the client program's interface to a movie.
	struct movie_interface
	{
		virtual int	get_width() = 0;
		virtual int	get_height() = 0;
		virtual int	get_current_frame() const = 0;

		virtual void	restart() = 0;
		virtual void	advance(float delta_time) = 0;
		virtual void	goto_frame(int frame_number) = 0;
		virtual void	display() = 0;
		virtual void	set_background_color(const rgba& bg_color) = 0;
	};

	// Create a swf::movie_interface from the given input stream.
	movie_interface*	create_movie(SDL_RWops* input);

	struct stream;
	struct movie;

	typedef void (*loader_function)(stream* input, int tag_type, movie* m);
	// Register a loader function for a certain tag type.
	void	register_tag_loader(int tag_type, loader_function lf);

};	// namespace swf


#endif // SWF_H

