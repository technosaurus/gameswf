// gameswf_as_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "gameswf/gameswf_action.h"	// for as_object
#include "gameswf/gameswf_sprite.h"

namespace gameswf
{

	//
	// sprite built-in ActionScript methods
	//

	void	sprite_hit_test(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.result->set_bool(false);
		if (fn.nargs == 1) 
		{
			// Evaluates the bounding boxes of the target and specified instance,
			// and returns true if they overlap or intersect at any point.
			character* ch = fn.env->find_target(fn.arg(0))->cast_to_character();
			if (ch)
			{
				fn.result->set_bool(sprite->hit_test(ch));
				return;
			}
			log_error("hiTest: can't find target\n");
			return;
		}
		log_error("hiTest(x,y,flag) is't implemented yet\n");
	}

	void	sprite_play(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		sprite->set_play_state(movie_interface::PLAY);
	}

	void	sprite_stop(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_goto_and_play(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		if (fn.nargs < 1)
		{
			log_error("error: sprite_goto_and_play needs one arg\n");
			return;
		}

		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		sprite->goto_frame(target_frame);
		sprite->set_play_state(movie_interface::PLAY);
	}

	void	sprite_goto_and_stop(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		if (fn.nargs < 1)
		{
			log_error("error: sprite_goto_and_stop needs one arg\n");
			return;
		}

		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		sprite->goto_frame(target_frame);
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_next_frame(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		int frame_count = sprite->get_frame_count();
		int current_frame = sprite->get_current_frame();
		if (current_frame < frame_count)
		{
			sprite->goto_frame(current_frame + 1);
		}
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_prev_frame(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		int current_frame = sprite->get_current_frame();
		if (current_frame > 0)
		{
			sprite->goto_frame(current_frame - 1);
		}
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_get_bytes_loaded(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.result->set_int(sprite->get_loaded_bytes());
	}

	void	sprite_get_bytes_total(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.result->set_int(sprite->get_file_bytes());
	}

	//swapDepths(target:Object) : Void
	void sprite_swap_depths(const fn_call& fn) 
	{ 
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		if (fn.nargs != 1) 
		{ 
			log_error("swapDepths needs one arg\n"); 
			return; 
		} 

		sprite_instance* target = NULL; 
		if (fn.arg(0).get_type() == as_value::OBJECT) 
		{ 
			target = fn.arg(0).to_object()->cast_to_sprite(); 
		} 
		else 
			if (fn.arg(0).get_type() == as_value::NUMBER) 
			{ 
				int target_depth = int(fn.arg(0).to_number()); 
				sprite_instance* parent = sprite->get_parent()->cast_to_sprite(); 
				
				character* ch = parent->m_display_list.get_character_at_depth(target_depth);
				if (ch)
				{
					target = parent->m_display_list.get_character_at_depth(target_depth)->cast_to_sprite();
				}
				else
				{
					log_error("swapDepths: no character in depth #%d\n", target_depth); 
				}
			} 
			else 
			{ 
				log_error("swapDepths has received invalid arg\n"); 
				return; 
			} 

			if (sprite == NULL || target == NULL) 
			{ 
				log_error("It is impossible to swap NULL character\n"); 
				return; 
			} 

			if (sprite->get_parent() == target->get_parent() && sprite->get_parent() != NULL) 
			{ 
				int target_depth = target->get_depth(); 
				target->set_depth(sprite->get_depth()); 
				sprite->set_depth(target_depth); 

				sprite_instance* parent = sprite->get_parent()->cast_to_sprite(); 
				parent->m_display_list.swap_characters(sprite, target); 
			} 
			else 
			{ 
				log_error("MovieClips should have the same parent\n"); 
			} 
	} 

	//duplicateMovieClip(name:String, depth:Number, [initObject:Object]) : MovieClip 
	void sprite_duplicate_movieclip(const fn_call& fn) 
	{ 
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		if (fn.nargs < 2) 
		{ 
			log_error("duplicateMovieClip needs 2 or 3 args\n"); 
			return; 
		} 

		as_object* init_object = NULL;
		if (fn.nargs == 3) 
		{ 
			init_object = fn.arg(2).to_object()->cast_to_as_object(); 
		} 

		character* ch = sprite->clone_display_object(
			fn.arg(0).to_tu_string(), 
			(int) fn.arg(1).to_number(),
			init_object);

		fn.result->set_as_object_interface(ch); 
	} 

	void sprite_get_depth(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.result->set_int(sprite->get_depth());
	}

	//createEmptyMovieClip(name:String, depth:Number) : MovieClip
	void sprite_create_empty_movieclip(const fn_call& fn)
	{
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		if (fn.nargs != 2)
		{
			log_error("createEmptyMovieClip needs 2 args\n");
			return;
		}

		character* ch = sprite->add_empty_movieclip(fn.arg(0).to_string(), int(fn.arg(1).to_number()));
		fn.result->set_as_object_interface(ch);
	}

	// removeMovieClip() : Void 
	void sprite_remove_movieclip(const fn_call& fn) 
	{ 
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		sprite_instance* parent = sprite->get_parent()->cast_to_sprite(); 
		if (parent) 
		{ 
			parent->remove_display_object(sprite->get_depth(), -1); 
		} 
	} 

	// loadMovie(url:String, [method:String]) : Void
	// TODO: implement [method:String]
	void sprite_loadmovie(const fn_call& fn) 
	{ 
		if (fn.nargs >= 1)
		{
			fn.env->load_file(fn.arg(0).to_string(), as_value(fn.this_ptr));
		}

	} 

	// public unloadMovie() : Void
	void sprite_unloadmovie(const fn_call& fn) 
	{ 
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.env->load_file("", as_value(fn.this_ptr));
	} 

	// getNextHighestDepth() : Number
	// Determines a depth value
	// The value returned is 0 or larger (that is, negative numbers are not returned). 
	void sprite_getnexthighestdepth(const fn_call& fn) 
	{ 
		sprite_instance* sprite = NULL;
		if (fn.this_ptr)
		{
			sprite = fn.this_ptr->cast_to_sprite();
		}
		if (sprite == NULL)
		{
			sprite = fn.env->get_target()->cast_to_sprite();
		}
		assert(sprite);

		fn.result->set_int(sprite->get_highest_depth());
	} 

}
