// gameswf_dlist.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A list of active characters.


#ifndef GAMESWF_DLIST_H
#define GAMESWF_DLIST_H


#include "base/container.h"
#include "gameswf_types.h"
#include "gameswf_impl.h"


namespace gameswf
{

	// A struct to serve as an entry in the display list.
	struct display_object_info : display_info	// private inheritance?
	{
		bool	m_ref;
		character*	m_character;

		static int compare(const void* _a, const void* _b)
		// For qsort().
		{
			display_object_info*	a = (display_object_info*) _a;
			display_object_info*	b = (display_object_info*) _b;

			if (a->m_depth < b->m_depth)
			{
				return -1;
			}
			else if (a->m_depth == b->m_depth)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	};


	// A list of active characters.
	struct display_list
	{

		// TODO use better names!
		int	find_display_index(int depth);
		int	get_display_index(int depth);
		
		void	add_display_object(movie* root_movie, character* ch, Uint16 depth, const cxform& color_xform, const matrix& mat, float ratio);
		void	move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio);
		void	replace_display_object(character* ch, Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio);
		void	remove_display_object(Uint16 depth);

		// clear the display list.
		void	clear();

		// reset the references to the display list.
		void	reset();

		// remove unreferenced objects.
		void	update();

		// advance referenced characters.
		void	advance(float delta_time, movie* m);
		void	advance(float delta_time, movie* m, const matrix& mat);

		// display the referenced characters.
		void	display(int m_total_display_count);
		void	display(const display_info & di);

		inline const display_object_info&	get_display_object(int idx) const
		// get the display object at the given position.
		{
			return m_display_object_array[idx];
		}


	private:

		array<display_object_info> m_display_object_array;

	};


}


#endif // GAMESWF_DLIST_H
