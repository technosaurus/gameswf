// gameswf_dlist.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A list of active characters.



#include "gameswf_dlist.h"
#include "gameswf_log.h"
#include "gameswf_render.h"
#include "gameswf.h"
#include <typeinfo>



namespace gameswf
{
	
	int	display_list::find_display_index(int depth)
		// Find the index in the display list matching the given
		// depth.  Failing that, return the index of the first object
		// with a larger depth.
	{
		int	size = m_display_object_array.size();
		if (size == 0)
		{
			return 0;
		}
		
		// Binary search.
		int	jump = size >> 1;
		int	index = jump;
		for (;;)
		{
			jump >>= 1;
			if (jump < 1) jump = 1;
			
			if (depth > m_display_object_array[index].m_depth) {
				if (index == size - 1)
				{
					index = size;
					break;
				}
				index += jump;
			}
			else if (depth < m_display_object_array[index].m_depth)
			{
				if (index == 0
					|| depth > m_display_object_array[index - 1].m_depth)
				{
					break;
				}
				index -= jump;
			}
			else
			{
				// match -- return this index.
				break;
			}
		}
		
		assert(index >= 0 && index <= size);
		
		return index;
	}
	
	
	int	display_list::get_display_index(int depth)
		// Like the above, but looks for an exact match, and returns -1 if failed.
	{
		int	index = find_display_index(depth);
		if (index >= m_display_object_array.size()
			|| get_display_object(index).m_depth != depth)
		{
			// No object at that depth.
			return -1;
		}
		return index;
	}
	
	
	void	display_list::add_display_object(
		movie* root_movie,
		character* ch, 
		Uint16 depth, 
		const cxform& color_xform, 
		const matrix& mat, 
		float ratio,
		Uint16 clip_depth)
	{
		assert(ch);
		
		// Try to move an existing character before creating a new one.
		int index = find_display_index(depth);
		
		if (index < m_display_object_array.size()
			&& get_display_object(index).m_depth == depth
			&& get_display_object(index).m_character->get_id() == ch->get_id())
		{
			display_object_info&	di = m_display_object_array[index];
			di.m_ref = true;
			di.m_color_transform = color_xform;
			di.m_matrix = mat;
			di.m_ratio = ratio;
			di.m_clip_depth = clip_depth;
			return;
		}
		
		// If the character needs per-instance state, then
		// create the instance here, and substitute it for the
		// definition.
		if (ch->is_definition())
		{
			ch = ch->create_instance();
			ch->restart();
		}
		
		display_object_info	di;
		di.m_movie = root_movie;
		di.m_ref = true;
		di.m_character = ch;
		di.m_depth = depth;
		di.m_color_transform = color_xform;
		di.m_matrix = mat;
		di.m_ratio = ratio;
		di.m_clip_depth = clip_depth;
		
		// Insert into the display list...
		index = find_display_index(di.m_depth);
		
		m_display_object_array.insert(index, di);
	}
	
	
	void	display_list::move_display_object(Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth)
		// Updates the transform properties of the object at
		// the specified depth.
	{
		int	size = m_display_object_array.size();
		if (size <= 0)
		{
			// error.
			log_error("error: move_display_object() -- no objects on display list\n");
			//			assert(0);
			return;
		}
		
		int	index = find_display_index(depth);
		if (index < 0 || index >= size)
		{
			// error.
			log_error("error: move_display_object() -- can't find object at depth %d\n", depth);
			//			assert(0);
			return;
		}
		
		display_object_info&	di = m_display_object_array[index];
		if (di.m_depth != depth)
		{
			// error
			log_error("error: move_display_object() -- no object at depth %d\n", depth);
			//			assert(0);
			return;
		}
		
		if (use_cxform)
		{
			di.m_color_transform = color_xform;
		}
		if (use_matrix)
		{
			di.m_matrix = mat;
		}
		di.m_ratio = ratio;
		di.m_clip_depth = clip_depth;
	}
	
	
	void	display_list::replace_display_object(character* ch, Uint16 depth, bool use_cxform, const cxform& color_xform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth)
		// Puts a new character at the specified depth, replacing any
		// existing character.  If use_cxform or use_matrix are false,
		// then keep those respective properties from the existing
		// character.
	{
		int	size = m_display_object_array.size();
		if (size <= 0)
		{
			// error.
			assert(0);
			return;
		}
		
		int	index = find_display_index(depth);
		if (index < 0 || index >= size)
		{
			// error.
			assert(0);
			return;
		}
		
		display_object_info&	di = m_display_object_array[index];
		if (di.m_depth != depth)
		{
			// error
			IF_DEBUG(log_msg("warning: replace_display_object() -- no object at depth %d\n", depth));
			//			assert(0);
			return;
		}
		
		// If the old character is an instance, then delete it.
		if (di.m_character->is_instance())
		{
			delete di.m_character;
			di.m_character = 0;
		}
		
		// Put the new character in its place.
		assert(ch);
		
		// If the character needs per-instance state, then
		// create the instance here, and substitute it for the
		// definition.
		if (ch->is_definition())
		{
			ch = ch->create_instance();
			ch->restart();
		}
		
		// Set the display properties.
		di.m_ref = true;
		di.m_character = ch;
		if (use_cxform)
		{
			di.m_color_transform = color_xform;
		}
		if (use_matrix)
		{
			di.m_matrix = mat;
		}
		di.m_ratio = ratio;
		di.m_clip_depth = clip_depth;
	}
	
	
	void	display_list::remove_display_object(Uint16 depth)
		// Removes the object at the specified depth.
	{
		int	size = m_display_object_array.size();
		if (size <= 0)
		{
			// error.
			log_error("remove_display_object: no characters in display list\n");
			return;
		}
		
		int	index = find_display_index(depth);
		if (index < 0 || index >= size)
		{
			// error -- no character at the given depth.
			log_error("remove_display_object: no character at depth %d\n", depth);
			return;
		}
		
		// Removing the character at get_display_object(index).
		display_object_info&	di = m_display_object_array[index];
		
		// Remove reference only.
		di.m_ref = false;
	}
	
	
	void	display_list::clear()
		// clear the display list.
	{
		int i, n = m_display_object_array.size();
		for (i = 0; i < n; i++)
		{
			character*	ch = m_display_object_array[i].m_character;
			if (ch->is_instance())
			{
				delete ch;
			}
		}
		
		m_display_object_array.clear();
	}
	
	
	void	display_list::reset()
		// reset the references to the display list.
	{
		//printf("### reset the display list!\n");
		int i, n = m_display_object_array.size();
		for (i = 0; i < n; i++)
		{
			m_display_object_array[i].m_ref = false;
		}
	}
	
	
	void	display_list::update()
		// remove unreferenced objects.
	{
		//printf("### update the display list!\n");
		
		int r = 0;
		int i, n = m_display_object_array.size();
		for (i = n-1; i >= 0; i--)
		{
			display_object_info & dobj = m_display_object_array[i];
			
			if (dobj.m_ref == false)
			{
				if (dobj.m_character->is_instance())
				{
					delete dobj.m_character;
					dobj.m_character = NULL;
				}
				m_display_object_array.remove(i);
				r++;
			}
		}
		
		//printf("### removed characters: %4d active characters: %4d\n", r, m_display_object_array.size());
	}
	
	
	void	display_list::advance(float delta_time, movie* m)
		// advance referenced characters.
	{
		int i, n = m_display_object_array.size();
		for (i = 0; i < n; i++)
		{
			display_object_info & dobj = m_display_object_array[i];
			
			if (dobj.m_ref == true)
			{
				dobj.m_character->advance(delta_time, m, dobj.m_matrix);
			}
		}
	}
	
	
	void	display_list::advance(float delta_time, movie* m, const matrix& mat)
		// advance referenced characters.
	{
		int i, n = m_display_object_array.size();
		for (i = 0; i < n; i++)
		{
			display_object_info & dobj = m_display_object_array[i];
			
			if (dobj.m_ref == true)
			{
				matrix	sub_matrix = mat;
				sub_matrix.concatenate(dobj.m_matrix);
				dobj.m_character->advance(delta_time, m, sub_matrix);
			}
		}
	}
	
	void	display_list::display(int m_total_display_count)
		// Display the referenced characters. Lower depths
		// are obscured by higher depths.
	{
		bool masked = false;
		int highest_masked_layer = 0;
		
		log_msg("number of objects to be drawn %i\n", m_display_object_array.size());
		
		for (int i = 0; i < m_display_object_array.size(); i++)
		{                        
			display_object_info&	dobj = m_display_object_array[i];
			
			log_msg("depth %i, clip_depth %i\n", dobj.m_depth, dobj.m_clip_depth);
			

			// check whether a previous mask should be disables
			if(masked)
			{
				if(dobj.m_depth > highest_masked_layer)
				{
					log_msg("disabled mask before drawing depth %i\n", dobj.m_depth);
					masked = false;
					// turn off mask
					render::disable_mask();
				}
			}

			// check whether this object should become mask
			if (dobj.m_clip_depth > 0)
			{
				log_msg("begin submit mask\n");
				render::begin_submit_mask();
			}     
			
			dobj.m_display_number = m_total_display_count;                             
			dobj.m_character->display(dobj);
			log_msg("object drawn\n");
			
			// if this object should have become a mask, inform the renderer that it now has all information about it
			if(dobj.m_clip_depth > 0)
			{
				log_msg("end submit mask\n");
				render::end_submit_mask();                            
				highest_masked_layer = dobj.m_clip_depth;
				masked = true;                                                    
			}
		}
		
		if(masked)
		{
			// if a mask masks the scene all the way upto the highest layer, it will not be disabled at the end
			// of drawing the display list, so disable it manually
			render::disable_mask();
		}
	}
	
	void	display_list::display(const display_info & di)
		// Display the referenced characters.
	{    
		log_msg("drawing object at depth %i\n",di.m_depth);
		for (int i = 0; i < m_display_object_array.size(); i++)
		{
			display_object_info&	dobj = m_display_object_array[i];                                       
			display_info	sub_di = di;
			sub_di.concatenate(dobj);
			dobj.m_character->display(sub_di);
		}
	}
}


