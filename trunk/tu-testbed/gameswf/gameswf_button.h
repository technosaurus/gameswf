// gameswf_button.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF buttons.  Mouse-sensitive update/display, actions, etc.


#ifndef GAMESWF_BUTTON_H
#define GAMESWF_BUTTON_H


#include "gameswf_impl.h"


namespace gameswf
{
	struct button_record
	{
		bool	m_hit_test;
		bool	m_down;
		bool	m_over;
		bool	m_up;
		character*	m_character;
		int	m_button_layer;
		matrix	m_button_matrix;
		cxform	m_button_cxform;

		bool	read(stream* in, int tag_type, movie* m);
	};
	

	struct button_action
	{
		enum condition
		{
			IDLE_TO_OVER_UP = 1 << 0,
			OVER_UP_TO_IDLE = 1 << 1,
			OVER_UP_TO_OVER_DOWN = 1 << 2,
			OVER_DOWN_TO_OVER_UP = 1 << 3,
			OVER_DOWN_TO_OUT_DOWN = 1 << 4,
			OUT_DOWN_TO_OVER_DOWN = 1 << 5,
			OUT_DOWN_TO_IDLE = 1 << 6,
			IDLE_TO_OVER_DOWN = 1 << 7,
			OVER_DOWN_TO_IDLE = 1 << 8,
		};
		int	m_conditions;
		array<action_buffer>	m_actions;

		void	read(stream* in, int tag_type);
	};


	struct button_character_definition : public character
	{
		bool m_menu;
		array<button_record>	m_button_records;
		array<button_action>	m_button_actions;

		button_character_definition();
		bool	is_definition() const;
		character*	create_instance();
		void	read(stream* in, int tag_type, movie* m);
	};

};	// end namespace gameswf


#endif // GAMESWF_BUTTON_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
