// swf.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) file parser.  The info for this came from
// http://www.openswf.org


#ifndef SWF_H
#define SWF_H


#include "SDL.h"


namespace swf {

	struct character;

	struct movie
	{
		virtual void	execute(float time);
		virtual void	add_character(int id, character* ch);
	};


	struct character
	{
		virtual void	execute(float time);	// OpenGL state contains viewport, transforms, etc.
	};


	struct tag
	{
		virtual void	execute(float time);
	};

	struct stream;

	// Create a swf::movie from the given input stream.
	movie*	create_movie(SDL_RWops* input);

	typedef void (*loader_function)(stream* input, int tag_type, movie* m);
	// Register a loader function for a certain tag type.
	void	register_tag_loader(int tag_type, loader_function lf);

};	// namespace swf


#endif // SWF_H

