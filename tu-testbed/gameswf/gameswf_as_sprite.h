// gameswf_as_sprite.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#ifndef GAMESWF_AS_SPRITE_H
#define GAMESWF_AS_SPRITE_H

namespace gameswf
{

	//
	// sprite built-in ActionScript methods
	//

	void	sprite_hit_test(const fn_call& fn);
	void	sprite_play(const fn_call& fn);
	void	sprite_stop(const fn_call& fn);
	void	sprite_goto_and_play(const fn_call& fn);
	void	sprite_goto_and_stop(const fn_call& fn);
	void	sprite_next_frame(const fn_call& fn);
	void	sprite_prev_frame(const fn_call& fn);
	void	sprite_get_bytes_loaded(const fn_call& fn);
	void	sprite_get_bytes_total(const fn_call& fn);
	void sprite_swap_depths(const fn_call& fn) ;
	void sprite_duplicate_movieclip(const fn_call& fn);
	void sprite_get_depth(const fn_call& fn);
	void sprite_create_empty_movieclip(const fn_call& fn);
	void sprite_remove_movieclip(const fn_call& fn);
	void sprite_loadmovie(const fn_call& fn);
	void sprite_unloadmovie(const fn_call& fn);
	void sprite_getnexthighestdepth(const fn_call& fn);
	void sprite_create_text_field(const fn_call& fn);
	void sprite_attach_movie(const fn_call& fn);

	void sprite_set_fps(const fn_call& fn);
}

#endif
