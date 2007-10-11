// gameswf_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_sprite.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_render.h"

#if TU_CONFIG_LINK_TO_LIB3DS
	#include "plugins/lib3ds/gameswf_3ds.h"
#endif

#ifdef _WIN32
	#define snprintf _snprintf
	#define strncasecmp strnicmp
#endif // _WIN32

namespace gameswf
{

	// url=="" means that the load_file() works as unloadMovie(target)
	character* as_environment::load_file(const char* url, const as_value& target_value)
	{
		sprite_instance* target = NULL;
		{
			character* ch = find_target(target_value);
			if (ch == NULL)
			{
				IF_VERBOSE_ACTION(log_msg("load_file: target %s is't found\n", target_value.to_string()));
				return NULL;
			}
			target = ch->cast_to_sprite();
		}

		if (target == NULL)
		{
			IF_VERBOSE_ACTION(log_msg("load_file: target %s is't movieclip\n", target_value.to_string()));
			return NULL;
		}

		movie_root* mroot = target->get_root();
		character* parent = target->get_parent();

		// is unloadMovie() ?
		if (strlen(url) == 0)
		{
			if (parent)
			{
				sprite_instance* parent_as_sprite = parent->cast_to_sprite();
				if (parent_as_sprite)
				{
					parent_as_sprite->remove_display_object(target);
				}
			}
			else	// target is _root, unloadMovie(_root)
			{
				target->clear_display_objects();
			}
			return NULL;
		}

		// is path relative ?
		tu_string infile = get_workdir();
		if (strstr(url, ":") || url[0] == '/')
		{
			infile = "";
		}
		infile += url;

		if (infile.size() < 4)	// need at least the file extension 
		{
			IF_VERBOSE_ACTION(log_msg("loadMovie: invalid path '%s'\n", infile.c_str()));
			return NULL;
		}

		// SWF loader

		if (strncasecmp(infile.c_str() + infile.size() - 4, ".swf", 4) == 0)
		{
			movie_definition*	md = create_movie(infile.c_str());
			if (md == NULL)
			{
				IF_VERBOSE_ACTION(log_msg("can't create movie from %s\n", infile.c_str()));
				return NULL;
			}

			// SWF loader into _root

			if (target == target->get_root_movie())
			{
				movie_interface* new_inst = md->create_instance();
				assert(new_inst);

				sprite_instance* new_root = new_inst->get_root_movie()->cast_to_sprite();
				set_current_root(new_inst);
				new_root->on_event(event_id::LOAD);
				return new_root;
			}

			// SWF loader into container

			sprite_instance* new_ch = new sprite_instance(md->cast_to_movie_def_impl(), mroot, parent, -1);

			const char* name = target->get_name();
			Uint16 depth = target->get_depth();
			bool use_cxform = false;
			cxform color_transform =  target->get_cxform();
			bool use_matrix = false;
			const matrix& mat = target->get_matrix();
			float ratio = target->get_ratio();
			Uint16 clip_depth = target->get_clip_depth();

			assert(parent != NULL);
			new_ch->set_parent(parent);


			if (new_ch->cast_to_sprite())
			{
				new_ch->cast_to_sprite()->set_root(mroot);
			}

			parent->replace_display_object(
				new_ch->cast_to_character(),
				name,
				depth,
				use_cxform,
				color_transform,
				use_matrix,
				mat,
				ratio,
				clip_depth);

			return new_ch;

		}
		else

		// JPEG loader

		if (strncasecmp(infile.c_str() + infile.size() - 4, ".jpg", 4) == 0)
		{
#if TU_CONFIG_LINK_TO_JPEGLIB == 0
			log_error("gameswf is not linked to jpeglib -- can't load jpeg image data!\n");
			return NULL;
#else

			image::rgb* im = image::read_jpeg(infile.c_str());
			if (im == NULL)
			{
				return NULL;
			}

			bitmap_info* bi = render::create_bitmap_info_rgb(im);
			delete im;

			bitmap_character*	jpg_def = new bitmap_character(bi);

			// clear target display list
			target->clear_display_objects();

			// add new jpeg character into empty sprite
			cxform color_transform;
			matrix matrix;
			target->m_display_list.add_display_object(
				jpg_def->create_character_instance(target, 1),
				target->get_highest_depth(),
				true,
				color_transform,
				matrix,
				0.0f,
				0); 

			return target;

#endif

		}
		else

		// 3DS loader

		if (strncasecmp(infile.c_str() + infile.size() - 4, ".3ds", 4) == 0)
		{
#if TU_CONFIG_LINK_TO_LIB3DS == 0
			log_error("gameswf is not linked to lib3ds -- can't load 3DS file\n");
			return NULL;
#else
			if (parent == NULL)
			{
				log_error("can't load 3DS as _root\n");
				return NULL;
			}

			x3ds_definition* x3ds = new x3ds_definition(infile.c_str());
			character* new_ch = x3ds->create_character_instance(parent, 1);

			const char* name = target->get_name();
			Uint16 depth = target->get_depth();
			bool use_cxform = false;
			cxform color_transform = target->get_cxform();
			bool use_matrix = false;
			const matrix& mat = target->get_matrix();
			float ratio = target->get_ratio();
			Uint16 clip_depth = target->get_clip_depth();

			new_ch->set_parent(parent);
			parent->replace_display_object(
				new_ch,
				name,
				depth,
				use_cxform,
				color_transform,
				use_matrix,
				mat,
				ratio,
				clip_depth);

			return new_ch;


#endif

		}

		log_error("loadMovie: can't load '%s'\n", infile.c_str());
		return NULL;
	}


	as_value	as_environment::get_variable(const tu_string& varname, const array<with_stack_entry>& with_stack) const
	// Return the value of the given var, if it's defined.
	{
		// Path lookup rigamarole.
		character*	target = m_target;
		tu_string	path;
		tu_string	var;
		if (parse_path(varname, &path, &var))
		{
			target = find_target(path);	// @@ Use with_stack here too???  Need to test.
			if (target)
			{
				as_value	val;
				target->get_member(var, &val);
				return val;
			}
			else
			{
				IF_VERBOSE_ACTION(log_msg("find_target(\"%s\") failed\n", path.c_str()));
				return as_value();
			}
		}
		else
		{
			return this->get_variable_raw(varname, with_stack);
		}
	}


	as_value	as_environment::get_variable_raw(
		const tu_string& varname,
		const array<with_stack_entry>& with_stack) const
	// varname must be a plain variable name; no path parsing.
	{
		as_value	val;

		if (strchr(varname.c_str(), ':') != NULL ||
			strchr(varname.c_str(), '/') != NULL ||
			strchr(varname.c_str(), '.') != NULL)
		{
			return val;
		}

		// Check the with-stack.
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			as_object_interface*	obj = with_stack[i].m_object.get_ptr();
			if (obj && obj->get_member(varname, &val))
			{
				// Found the var in this context.
				return val;
			}
		}

		// Check locals.
		int	local_index = find_local(varname, true);
		if (local_index >= 0)
		{
			// Get local var.
			return m_local_frames[local_index].m_value;
		}

		// Looking for "this"?
		if (varname == "this")
		{
			val.set_as_object_interface(m_target);
			return val;
		}

		// Check movie members.
		if (m_target)
		{
			if (m_target->get_member(varname, &val))
			{
				return val;
			}
		}


		// Check built-in constants.
		if (m_target)
		{
			if (varname == "_root" || varname == "_level0")
			{
				return as_value(m_target->get_root_movie());
			}
		}

		if (varname == "_global")
		{
			return as_value(get_global());
		}
		if (get_global()->get_member(varname, &val))
		{
			return val;
		}
	
		// Fallback.
		IF_VERBOSE_ACTION(log_msg("get_variable_raw(\"%s\") failed, returning UNDEFINED.\n", varname.c_str()));
		return as_value();
	}

	void as_environment::set_target(as_value& target, character* original_target)
	{
		if (target.get_type() == as_value::STRING)
		{
			tu_string path = target.to_tu_string();
			IF_VERBOSE_ACTION(log_msg("-------------- ActionSetTarget2: %s", path.c_str()));
			if (path.size() > 0)
			{
				character* tar = find_target(path);
				if (tar)
				{
					set_target(tar);
					return;
				}
			}
			else
			{
				set_target(original_target);
				return;
			}
		}
		else
		if (target.get_type() == as_value::OBJECT)
		{
			IF_VERBOSE_ACTION(log_msg("-------------- ActionSetTarget2: %s", target.to_string()));
			character* tar = find_target(target);
			if (tar)
			{
				set_target(tar);
				return;
			}
		}
		IF_VERBOSE_ACTION(log_msg("can't set target %s\n", target.to_string()));
	}

	void	as_environment::set_variable(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
	// Given a path to variable, set its value.
	{
		IF_VERBOSE_ACTION(log_msg("-------------- %s = %s\n", varname.c_str(), val.to_string()));//xxxxxxxxxx

		// Path lookup rigamarole.
		character*	target = m_target;
		tu_string	path;
		tu_string	var;
		if (parse_path(varname, &path, &var))
		{
			target = find_target(path);
			if (target)
			{
				target->set_member(var, val);
			}
		}
		else
		{
			this->set_variable_raw(varname, val, with_stack);
		}
	}


	void	as_environment::set_variable_raw(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
	// No path rigamarole.
	{
		// Check the with-stack.
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			as_object_interface*	obj = with_stack[i].m_object.get_ptr();
			as_value	dummy;
			if (obj && obj->get_member(varname, &dummy))
			{
				// This object has the member; so set it here.
				obj->set_member(varname, val);
				return;
			}
		}

		// Check locals.
		int	local_index = find_local(varname, true);
		if (local_index >= 0)
		{
			// Set local var.
			m_local_frames[local_index].m_value = val;
			return;
		}

		if (m_target)
		{
			m_target->set_member(varname, val);
		}
		else
		{
			log_error("can't set_variable_raw '%s'='%s', target is NULL\n",
				varname.c_str(), val.to_string());
		}
	}


	void	as_environment::set_local(const tu_string& varname, const as_value& val)
	// Set/initialize the value of the local variable.
	{
		// Is it in the current frame already?
		int	index = find_local(varname, false);
		if (index < 0)
		{
			// Not in frame; create a new local var.
			assert(varname.length() > 0);	// null varnames are invalid!
			m_local_frames.push_back(frame_slot(varname, val));
		}
		else
		{
			// In frame already; modify existing var.
			m_local_frames[index].m_value = val;
		}
	}

	
	void	as_environment::add_local(const tu_string& varname, const as_value& val)
	// Add a local var with the given name and value to our
	// current local frame.  Use this when you know the var
	// doesn't exist yet, since it's faster than set_local();
	// e.g. when setting up args for a function.
	{
		assert(varname.length() > 0);
		m_local_frames.push_back(frame_slot(varname, val));
	}


	void	as_environment::declare_local(const tu_string& varname)
	// Create the specified local var if it doesn't exist already.
	{
		// Is it in the current frame already?
		int	index = find_local(varname, false);
		if (index < 0)
		{
			// Not in frame; create a new local var.
			assert(varname.length() > 0);	// null varnames are invalid!
 			m_local_frames.push_back(frame_slot(varname, as_value()));
		}
		else
		{
			// In frame already; don't mess with it.
		}
	}

	
	bool	as_environment::get_member(const tu_stringi& varname, as_value* val) const
	{
		return m_variables.get(varname, val);
	}


	bool	as_environment::set_member(const tu_stringi& varname, const as_value& val)
	{
		m_variables.set(varname, val);
		return true;
	}

	as_value* as_environment::get_register(int reg)
	{
		as_value* val = local_register_ptr(reg);
		IF_VERBOSE_ACTION(log_msg("-------------- get_register(%d): %s at 0x%X\n", 
			reg, val->to_string(), val->to_object()));
		return val;
	}

	void as_environment::set_register(int reg, const as_value& val)
	{
		IF_VERBOSE_ACTION(log_msg("-------------- set_register(%d): %s at 0x%X\n",
			reg, val.to_string(), val.to_object()));
		*local_register_ptr(reg) = val;
	}

	as_value*	as_environment::local_register_ptr(int reg)
	// Return a pointer to the specified local register.
	// Local registers are numbered starting with 1.
	//
	// Return value will never be NULL.  If reg is out of bounds,
	// we log an error, but still return a valid pointer (to
	// global reg[0]).  So the behavior is a bit undefined, but
	// not dangerous.
	{
		// We index the registers from the end of the register
		// array, so we don't have to keep base/frame
		// pointers.

		assert(reg >=0 && reg <= m_local_register.size());

		// Flash 8 can have zero register (-1 for zero)
		return &m_local_register[m_local_register.size() - reg - 1];
	}


	int	as_environment::find_local(const tu_string& varname, bool ignore_barrier) const
	// Search the active frame for the named var; return its index
	// in the m_local_frames stack if found.
	// 
	// Otherwise return -1.
	// set_local should use "ignore_barrier=false"
	// get_variable should use "ignore_barrier=true"
	{
		// Linear search sucks, but is probably fine for
		// typical use of local vars in script.  There could
		// be pathological breakdowns if a function has tons
		// of locals though.  The ActionScript bytecode does
		// not help us much by using strings to index locals.

		for (int i = m_local_frames.size() - 1; i >= 0; i--)
		{
			const frame_slot&	slot = m_local_frames[i];
			if (slot.m_name.length() == 0 && ignore_barrier == false)
			{
				// End of local frame; stop looking.
				return -1;
			}
			else
			if (slot.m_name == varname)
			{
				// Found it.
				return i;
			}
		}
		return -1;
	}


	bool	as_environment::parse_path(const tu_string& var_path, tu_string* path, tu_string* var)
	// See if the given variable name is actually a sprite path
	// followed by a variable name.  These come in the format:
	//
	// 	/path/to/some/sprite/:varname
	//
	// (or same thing, without the last '/')
	//
	// or
	//	path.to.some.var
	//
	// If that's the format, puts the path part (no colon or
	// trailing slash) in *path, and the varname part (no color)
	// in *var and returns true.
	//
	// If no colon, returns false and leaves *path & *var alone.
	{
		// Search for colon.
		int	colon_index = 0;
		int	var_path_length = var_path.length();
		for ( ; colon_index < var_path_length; colon_index++)
		{
			if (var_path[colon_index] == ':')
			{
				// Found it.
				break;
			}
		}

		if (colon_index >= var_path_length)
		{
			// No colon.  Is there a '.'?  Find the last
			// one, if any.
			for (colon_index = var_path_length - 1; colon_index >= 0; colon_index--)
			{
				if (var_path[colon_index] == '.'	||
					var_path[colon_index] == '/')		// ActionScript 1.0
				{
					// Found it.
					break;
				}
			}
			if (colon_index < 0) return false;
		}

		// Make the subparts.

		// Var.
		*var = &var_path[colon_index + 1];

		// Path.
		if (colon_index > 0)
		{
			if (var_path[colon_index - 1] == '/')
			{
				// Trim off the extraneous trailing slash.
				colon_index--;
			}
		}
		// @@ could be better.  This whole usage of tu_string is very flabby...
		*path = var_path;
		path->resize(colon_index);

		return true;
	}


	character*	as_environment::find_target(const as_value& val) const
	// Find the sprite/movie represented by the given value.  The
	// value might be a reference to the object itself, or a
	// string giving a relative path name to the object.
	{
		if (val.get_type() == as_value::OBJECT)
		{
			if (val.to_object() != NULL)
			{
				return val.to_object()->cast_to_character();
			}
			else
			{
				return NULL;
			}
		}
		else if (val.get_type() == as_value::STRING)
		{
			return find_target(val.to_tu_string());
		}
		else
		{
			log_error("error: invalid path; neither string nor object\n");
			return NULL;
		}
	}


	const char*	next_slash_or_dot(const char* word)
	// Search for next '.' or '/' character in this word.  Return
	// a pointer to it, or to NULL if it wasn't found.
	{
		for (const char* p = word; *p; p++)
		{
			if (*p == '.' && p[1] == '.')
			{
				p++;
			}
			else if (*p == '.' || *p == '/')
			{
				return p;
			}
		}

		return NULL;
	}


	character*	as_environment::find_target(const tu_string& path) const
	// Find the sprite/movie referenced by the given path.
	{
		if (path.length() <= 0)
		{
			return m_target;
		}

		assert(path.length() > 0);

		character*	env = m_target;
		assert(env);
		
		const char*	p = path.c_str();
		tu_string	subpart;

		if (*p == '/')
		{
			// Absolute path.  Start at the root.
			env = env->get_relative_target("_level0");
			p++;
		}

		for (;;)
		{
			const char*	next_slash = next_slash_or_dot(p);
			subpart = p;
			if (next_slash == p)
			{
				log_error("error: invalid path '%s'\n", path.c_str());
				break;
			}
			else if (next_slash)
			{
				// Cut off the slash and everything after it.
				subpart.resize(next_slash - p);
			}

			env = env->get_relative_target(subpart);
			//@@   _level0 --> root, .. --> parent, . --> this, other == character

			if (env == NULL || next_slash == NULL)
			{
				break;
			}

			p = next_slash + 1;
		}
		return env;
	}

	void as_environment::dump()
	// for debugging
	// retrieves vars & print them
	{
		printf("\n*** environment 0x%p ***\n", this);

		printf("*** variables\n");
		for (stringi_hash<as_value>::const_iterator it = m_variables.begin(); 
			it != m_variables.end(); ++it)
		{
			printf("%s: %s\n", it->first.c_str(), it->second.to_string());
		}

		printf("*** stack\n");
		for (int i = m_stack.size() - 1; i >= 0; i--)
		{
			printf("%s\n", m_stack[i].to_string());
		}

		printf("***\n");
	}

	void	as_environment::clear_refs(as_object_interface* this_ptr)
	{
		for (stringi_hash<as_value>::iterator it = m_variables.begin();
			it != m_variables.end(); ++it)
		{
			as_object_interface* obj = it->second.to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					it->second.set_undefined();
				}
				else
				{
					obj->clear_refs(this_ptr);
				}
			}
		}
	}

}
