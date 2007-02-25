// as_key.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_KEY_H
#define GAMESWF_AS_KEY_H

#include "../gameswf_action.h"	// for as_object

namespace gameswf
{
	void	key_add_listener(const fn_call& fn);
	// Add a listener (first arg is object reference) to our list.
	// Listeners will have "onKeyDown" and "onKeyUp" methods
	// called on them when a key changes state.

	void	key_get_ascii(const fn_call& fn);
	// Return the ascii value of the last key pressed.

	void	key_get_code(const fn_call& fn);
	// Returns the keycode of the last key pressed.

	void	key_is_down(const fn_call& fn);
	// Return true if the specified (first arg keycode) key is pressed.

	void	key_is_toggled(const fn_call& fn);
	// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
	// the associated state is on.

	void	key_remove_listener(const fn_call& fn);
	// Remove a previously-added listener.

	struct as_key : public as_object
	{
		Uint8	m_keymap[key::KEYCOUNT / 8 + 1];	// bit-array
		array<weak_ptr<as_object_interface> >	m_listeners;
		int	m_last_key_pressed;

		as_key();

		bool	is_key_down(int code);
		void	set_key_down(int code);
		void	set_key_up(int code);
		void	cleanup_listeners();
		void	add_listener(as_object_interface* listener);
		void	remove_listener(as_object_interface* listener);
		int	get_last_key_pressed() const;
	};

	as_key* key_init();
	// Create built-in key object.

}	// namespace gameswf

#endif	// GAMESWF_AS_KEY_H
