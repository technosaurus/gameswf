// gameswf_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_impl.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_timers.h"
#include "gameswf/gameswf_sprite.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_freetype.h"
#include "base/tu_random.h"
#include "base/tu_timer.h"
#include "base/tu_loadlib.h"

// action script classes
#include "gameswf/gameswf_as_classes/as_array.h"
#include "gameswf/gameswf_as_classes/as_sound.h"
#include "gameswf/gameswf_as_classes/as_key.h"
#include "gameswf/gameswf_as_classes/as_math.h"
#include "gameswf/gameswf_as_classes/as_mcloader.h"
#include "gameswf/gameswf_as_classes/as_netstream.h"
#include "gameswf/gameswf_as_classes/as_netconnection.h"
#include "gameswf/gameswf_as_classes/as_textformat.h"
#include "gameswf/gameswf_as_classes/as_string.h"
#include "gameswf/gameswf_as_classes/as_color.h"
#include "gameswf/gameswf_as_classes/as_date.h"
#include "gameswf/gameswf_as_classes/as_xmlsocket.h"
#include "gameswf/gameswf_as_classes/as_broadcaster.h"
#include "gameswf/gameswf_as_classes/as_db.h"	// mysql plugin

#ifdef _WIN32
#define snprintf _snprintf
#endif // _WIN32

// NOTES:
//
// Buttons
// on (press)                 onPress
// on (release)               onRelease
// on (releaseOutside)        onReleaseOutside
// on (rollOver)              onRollOver
// on (rollOut)               onRollOut
// on (dragOver)              onDragOver
// on (dragOut)               onDragOut
// on (keyPress"...")         onKeyDown, onKeyUp      <----- IMPORTANT
//
// Sprites
// onClipEvent (load)         onLoad
// onClipEvent (unload)       onUnload                Hm.
// onClipEvent (enterFrame)   onEnterFrame
// onClipEvent (mouseDown)    onMouseDown
// onClipEvent (mouseUp)      onMouseUp
// onClipEvent (mouseMove)    onMouseMove
// onClipEvent (keyDown)      onKeyDown
// onClipEvent (keyUp)        onKeyUp
// onClipEvent (data)         onData

// Text fields have event handlers too!

// Sprite built in methods:
// play()
// stop()
// gotoAndStop()
// gotoAndPlay()
// nextFrame()
// startDrag()
// getURL()
// getBytesLoaded()
// getBytesTotal()

// Built-in functions: (do these actually exist in the VM, or are they just opcodes?)
// Number()
// String()


// TODO builtins
//
// Number.toString() -- takes an optional arg that specifies the base
//
// parseInt(), parseFloat()
//
// Boolean() type cast
//
// typeof operator --> "number", "string", "boolean", "object" (also
// for arrays), "null", "movieclip", "function", "undefined"
//
// isNaN()
//
// Number.MAX_VALUE, Number.MIN_VALUE
//
// String.fromCharCode()



namespace gameswf
{
	
	//
	// action stuff
	//

	void	action_init();

	// Statics.
	bool	s_inited = false;
	smart_ptr<as_object>	s_global;
	fscommand_callback	s_fscommand_handler = NULL;
	Uint64 s_start_time = 0;

	static heap s_heap;
	heap* get_heap() { return &s_heap; }

	static tu_string s_gameswf_version("gameSWF");
	const char* get_gameswf_version() {	return s_gameswf_version.c_str(); }
	
	void	register_fscommand_callback(fscommand_callback handler)
	// External interface.
	{
		s_fscommand_handler = handler;
	}

	as_object* get_global()
	{
		return s_global.get_ptr();
	}

	void set_bootup_options(const char* param)
	// gameSWF extension
	// Allow pass the user bootup options to Flash (through _global._bootup)
	{
		action_init();
		s_global->set_member("_bootup", param);
	}

	//
	// Function/method dispatch.
	//


	as_value	call_method(
		const as_value& method,
		as_environment* env,
		as_object_interface* this_ptr,
		int nargs,
		int first_arg_bottom_index)
	// first_arg_bottom_index is the stack index, from the bottom, of the first argument.
	// Subsequent arguments are at *lower* indices.  E.g. if first_arg_bottom_index = 7,
	// then arg1 is at env->bottom(7), arg2 is at env->bottom(6), etc.
	{
		as_value	val;

		as_c_function_ptr	func = method.to_c_function();
		if (func)
		{
			// It's a C function.  Call it.
			(*func)(fn_call(&val, this_ptr, env, nargs, first_arg_bottom_index));
		}
		else if (as_as_function* as_func = method.to_as_function())
		{
			// It's an ActionScript function.  Call it.
			(*as_func)(fn_call(&val, this_ptr, env, nargs, first_arg_bottom_index));
		}
		else
		{
			IF_VERBOSE_ACTION(log_error("error in call_method(): method is not a function\n"));
		}

		return val;
	}

/*	const char*	call_method_parsed(
		as_environment* env,
		as_object_interface* this_ptr,
		const char* method_name,
		const char* method_arg_fmt,
		va_list args)
	// Printf-like vararg interface for calling ActionScript.
	// Handy for external binding.
	{
		log_msg("FIXME(%d): %s\n", __LINE__, __FUNCTION__);

#if 0
		static const int	BUFSIZE = 1000;
		char	buffer[BUFSIZE];
		array<const char*>	tokens;

		// Brutal crap parsing.  Basically null out any
		// delimiter characters, so that the method name and
		// args sit in the buffer as null-terminated C
		// strings.  Leave an intial ' character as the first
		// char in a string argument.
		// Don't verify parens or matching quotes or anything.
		{
			strncpy(buffer, method_call, BUFSIZE);
			buffer[BUFSIZE - 1] = 0;
			char*	p = buffer;

			char	in_quote = 0;
			bool	in_arg = false;
			for (;; p++)
			{
				char	c = *p;
				if (c == 0)
				{
					// End of string.
					break;
				}
				else if (c == in_quote)
				{
					// End of quotation.


					assert(in_arg);
					*p = 0;
					in_quote = 0;
					in_arg = false;
				}
				else if (in_arg)
				{
					if (in_quote == 0)
					{
						if (c == ')' || c == '(' || c == ',' || c == ' ')
						{
							// End of arg.
							*p = 0;
							in_arg = false;
						}
					}
				}
				else
				{
					// Not in arg.  Watch for start of arg.
					assert(in_quote == 0);
					if (c == '\'' || c == '\"')
					{
						// Start of quote.
						in_quote = c;
						in_arg = true;
						*p = '\'';	// ' at the start of the arg, so later we know this is a string.
						tokens.push_back(p);
					}
					else if (c == ' ' || c == ',')
					{
						// Non-arg junk, null it out.
						*p = 0;
					}
					else
					{
						// Must be the start of a numeric arg.
						in_arg = true;
						tokens.push_back(p);
					}
				}
			}
		}
#endif // 0


		// Parse va_list args
		int	starting_index = env->get_top_index();
		const char* p = method_arg_fmt;
		for (;; p++)
		{
			char	c = *p;
			if (c == 0)
			{
				// End of args.
				break;
			}
			else if (c == '%')
			{
				p++;
				c = *p;
				// Here's an arg.
				if (c == 'd')
				{
					// Integer.
					env->push(va_arg(args, int));
				}
				else if (c == 'f')
				{
					// Double
					env->push(va_arg(args, double));
				}
				else if (c == 's')
				{
					// String
					env->push(va_arg(args, const char *));
				}
				else if (c == 'l')
				{
					p++;
					c = *p;
					if (c == 's')
					{
						// Wide string.
						env->push(va_arg(args, const wchar_t *));
					}
					else
					{
						log_error("call_method_parsed('%s','%s') -- invalid fmt '%%l%c'\n",
							  method_name,
							  method_arg_fmt,
							  c);
					}
				}
				else
				{
					// Invalid fmt, warn.
					log_error("call_method_parsed('%s','%s') -- invalid fmt '%%%c'\n",
						  method_name,
						  method_arg_fmt,
						  c);
				}
			}
			else
			{
				// Ignore whitespace and commas.
				if (c == ' ' || c == '\t' || c == ',')
				{
					// OK
				}
				else
				{
					// Invalid arg; warn.
					log_error("call_method_parsed('%s','%s') -- invalid char '%c'\n",
						  method_name,
						  method_arg_fmt,
						  c);
				}
			}
		}

		array<with_stack_entry>	dummy_with_stack;
		as_value	method = env->get_variable(method_name, dummy_with_stack);

		// check method

		// Reverse the order of pushed args
		int	nargs = env->get_top_index() - starting_index;
		for (int i = 0; i < (nargs >> 1); i++)
		{
			int	i0 = starting_index + 1 + i;
			int	i1 = starting_index + nargs - i;
			assert(i0 < i1);

			swap(&(env->bottom(i0)), &(env->bottom(i1)));
		}

		// Do the call.
		as_value	result = call_method(method, env, this_ptr, nargs, env->get_top_index());
		env->drop(nargs);

		// Return pointer to static string for return value.
		static tu_string	s_retval;
		s_retval = result.to_tu_string();
		return s_retval.c_str();
	}
*/

	//
	// Built-in objects
	//

	//
	// global init
	//


	void	as_global_trace(const fn_call& fn)
	{
		assert(fn.nargs >= 1);

		// Log our argument.
		//
		// @@ what if we get extra args?

		const char* val = "";
		if (fn.arg(0).get_type() == as_value::UNDEFINED) {
			val = "undefined";
		} else {
			val = fn.arg(0).call_to_string(fn.env).c_str();
		}
		log_msg("%s\n", val);
	}

	void	as_global_object_ctor(const fn_call& fn)
	// Constructor for ActionScript class Object.
	{
		fn.result->set_as_object_interface(new as_object);
	}


	void	as_global_assetpropflags(const fn_call& fn)
	// Undocumented ASSetPropFlags function
	{
		const int version = fn.env->get_target()->get_movie_definition()->get_version();

		// Check the arguments
		assert(fn.nargs == 3 || fn.nargs == 4);
		assert((version == 5) ? (fn.nargs == 3) : true);

		// object
		as_object_interface* const obj = fn.arg(0).to_object();
		if (obj == NULL)
		{
			log_error("error: assetpropflags for NULL object\n");
			return;
		}

		// The second argument is a list of child names,
		// may be in the form array(like ["abc", "def", "ggggg"]) or in the form a string(like "abc, def, ggggg")
		// the NULL second parameter means that assetpropflags is applied to all children
		as_object_interface* props = fn.arg(1).to_object();

		// a number which represents three bitwise flags which
		// are used to determine whether the list of child names should be hidden,
		// un-hidden, protected from over-write, un-protected from over-write,
		// protected from deletion and un-protected from deletion
		int set_true = int(fn.arg(2).to_number()) & as_prop_flags::as_prop_flags_mask;

		// Is another integer bitmask that works like set_true,
		// except it sets the attributes to false. The
		// set_false bitmask is applied before set_true is applied

		// ASSetPropFlags was exposed in Flash 5, however the fourth argument 'set_false'
		// was not required as it always defaulted to the value '~0'. 
		int set_false = (fn.nargs == 3 ? 
				 (version == 5 ? ~0 : 0) : int(fn.arg(3).to_number()))
			& as_prop_flags::as_prop_flags_mask;

		// Evan: it seems that if set_true == 0 and set_false == 0, this function
		// acts as if the parameters where (object, null, 0x1, 0) ...
		if (set_false == 0 && set_true == 0)
		{
			props = NULL;
			set_false = 0;
			set_true = 0x1;
		}

		if (props == NULL)
		{
			// Take all the members of the object

			as_object* object = obj->cast_to_as_object();
			if (object)
			{
				for (stringi_hash<as_member>::const_iterator it = object->m_members.begin(); 
					it != object->m_members.end(); ++it)
				{
					as_member member = it.get_value();

					as_prop_flags f = member.get_member_flags();
					f.set_flags(set_true, set_false);
					member.set_member_flags(f);

					object->m_members.set(it.get_key(), member);
				}
			}
		}
		else
		{
			as_object* object = obj->cast_to_as_object();
			as_object* object_props = props->cast_to_as_object();
			if (object == NULL || object_props == NULL)
			{
				return;
			}

			stringi_hash<as_member>::iterator it = object_props->m_members.begin();
			while(it != object_props->m_members.end())
			{
				const tu_stringi key = (it.get_value()).get_member_value().to_string();
				stringi_hash<as_member>::iterator it2 = object->m_members.find(key);

				if (it2 != object->m_members.end())
				{
					as_member member = it2.get_value();

					as_prop_flags f = member.get_member_flags();
					f.set_flags(set_true, set_false);
					member.set_member_flags(f);

					object->m_members.set((it.get_value()).get_member_value().to_string(), member);
				}

				++it;
			}
		}
	}

	void	as_global_movieclip_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
		sprite_definition* empty_sprite_def = new sprite_definition(NULL);
		character* ch = new sprite_instance(empty_sprite_def, get_current_root()->get_root(), get_current_root()->get_root_movie(), 0);
		fn.result->set_as_object_interface(ch);
	}

	void as_global_boolean_ctor(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			fn.result->set_bool(fn.arg(0).to_bool());
			return;
		}
		fn.result->set_undefined();
	}

	void as_global_number_ctor(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			fn.result->set_double(fn.arg(0).to_number());
			return;
		}
		fn.result->set_undefined();
	}


	// getVersion() : String
	void	as_global_get_version(const fn_call& fn)
	// Returns a string containing Flash Player version and platform information.
	{
		fn.result->set_tu_string(get_gameswf_version());
	}

	void	action_init()
	// Create/hook built-ins.
	{
		if (s_inited == false)
		{
			s_inited = true;
			s_start_time = tu_timer::get_ticks();

			// @@ s_global should really be a
			// client-visible player object, which
			// contains one or more actual movie
			// instances.  We're currently just hacking it
			// in as an app-global mutable object :(
			assert(s_global == NULL);
			s_global = new as_object;
			get_heap()->set(s_global.get_ptr(), false);
			s_global->set_member("trace", as_value(as_global_trace));
			s_global->set_member("Object", as_value(as_global_object_ctor));
			s_global->set_member("Sound", as_value(as_global_sound_ctor));
			s_global->set_member("Array", as_value(as_global_array_ctor));
			s_global->set_member("MovieClip", as_value(as_global_movieclip_ctor));

			s_global->set_member("TextFormat", as_value(as_global_textformat_ctor));

			//			s_global->set_member("XML", as_value(xml_new));
			s_global->set_member("XMLSocket", as_value(as_global_xmlsock_ctor));

			s_global->set_member("MovieClipLoader", as_value(as_global_mcloader_ctor));
			s_global->set_member("String", as_value(string_ctor));
			s_global->set_member("Number", as_value(as_global_number_ctor));
			s_global->set_member("Boolean", as_value(as_global_boolean_ctor));
			s_global->set_member("Color", as_value(as_global_color_ctor));
			s_global->set_member("Date", as_value(as_global_date_ctor));

			// ASSetPropFlags
			s_global->set_member("ASSetPropFlags", as_value(as_global_assetpropflags));

			// for video
			s_global->set_member("NetStream", as_value(as_global_netstream_ctor));
			s_global->set_member("NetConnection", as_value(as_global_netconnection_ctor));

			// MYSQL extensions
			s_global->set_member("MyDb", as_value(as_global_mysqldb_ctor));

			s_global->set_member("math", math_init());
			s_global->set_member("Key", key_init());
			s_global->set_member("AsBroadcaster", broadcaster_init());

			// global builtins functions
			s_global->set_member("setInterval",  as_value(as_global_setinterval));
			s_global->set_member("clearInterval",  as_value(as_global_clearinterval));
			s_global->set_member("getVersion",  as_value(as_global_get_version));
			s_global->set_member("/:$version",  "gameSWF");
		}
	}


	void	action_clear()
	{
		if (s_inited)
		{
			s_inited = false;
			s_global = NULL;
			tu_freetype::close();
		}
	}

	void notify_key_object(key::code k, bool down)
	{
		static tu_string	key_obj_name("Key");

		action_init();

		as_value	kval;
		s_global->get_member(key_obj_name, &kval);
		if (kval.get_type() == as_value::OBJECT)
		{
			as_key*	ko = kval.to_object()->cast_to_as_key();
			assert(ko);

			if (down) ko->set_key_down(k);
			else ko->set_key_up(k);
		}
		else
		{
			log_error("gameswf::notify_key_event(): no Key built-in\n");
		}
	}

	//
	// properties by number
	//

	static const tu_string	s_property_names[] =
	{
		tu_string("_x"),
		tu_string("_y"),
		tu_string("_xscale"),
		tu_string("_yscale"),
		tu_string("_currentframe"),
		tu_string("_totalframes"),
		tu_string("_alpha"),
		tu_string("_visible"),
		tu_string("_width"),
		tu_string("_height"),
		tu_string("_rotation"),
		tu_string("_target"),
		tu_string("_framesloaded"),
		tu_string("_name"),
		tu_string("_droptarget"),
		tu_string("_url"),
		tu_string("_highquality"),
		tu_string("_focusrect"),
		tu_string("_soundbuftime"),
		tu_string("@@ mystery quality member"),
		tu_string("_xmouse"),
		tu_string("_ymouse"),
	};


	static as_value	get_property(as_object_interface* obj, int prop_number)
	{
		as_value	val;
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->get_member(s_property_names[prop_number], &val);
		}
		else
		{
			log_error("error: invalid property query, property number %d\n", prop_number);
		}
		return val;
	}

	static void	set_property(as_object_interface* obj, int prop_number, const as_value& val)
	{
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->set_member(s_property_names[prop_number], val);
		}
		else
		{
			log_error("error: invalid set_property, property number %d\n", prop_number);
		}
	}


	//
	// do_action
	//


	// Thin wrapper around action_buffer.
	struct do_action : public execute_tag
	{
		action_buffer	m_buf;

		void	read(stream* in)
		{
			m_buf.read(in);
		}

		virtual void	execute(character* m)
		{
			m->add_action_buffer(&m_buf);
		}

		// Don't override because actions should not be replayed when seeking the movie.
		//void	execute_state(character* m) {}

		virtual bool	is_action_tag() const
		// Tell the caller that we are an action tag.
		{
			return true;
		}
	};

	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		IF_VERBOSE_PARSE(log_msg("tag %d: do_action_loader\n", tag_type));

		IF_VERBOSE_ACTION(log_msg("-------------- actions in frame %d\n", m->get_loading_frame()));

		assert(in);
		assert(tag_type == 12);
		assert(m);
		
		do_action*	da = new do_action;
		da->read(in);

		m->add_execute_tag(da);
	}

	
	//
	// do_init_action
	//


	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 59);

		int	sprite_character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("  tag %d: do_init_action_loader\n", tag_type));

		IF_VERBOSE_ACTION(log_msg("  -- init actions for sprite %d\n", sprite_character_id));

		do_action*	da = new do_action;
		da->read(in);
		m->add_init_action(sprite_character_id, da);
	}

	// radix:Number - Specifies the numeric base (from 2 to 36) to use for 
	// the number-to-string conversion. 
	// If you do not specify the radix parameter, the default value is 10.
	static const char s_hex[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZW";
	tu_string number_to_string(int radix, int val)
	{
		tu_string res;
		if (radix >=2 && radix <= strlen(s_hex))
		{
			while (val > 0)
			{
				int k = val % radix;
				val = (int) (val / radix);
				tu_string digit;
				digit += s_hex[k];
				res = digit + res;
			}
		}
		return res;
	}


	//
	// action_buffer
	//

	// Disassemble one instruction to the log.
	static void	log_disasm(const unsigned char* instruction_data);

	action_buffer::action_buffer()
		:
		m_decl_dict_processed_at(-1)
	{
	}


	void	action_buffer::read(stream* in)
	{
		// Read action bytes.
		for (;;)
		{

			int	instruction_start = m_buffer.size();

			int	pc = m_buffer.size();

			int	action_id = in->read_u8();
			m_buffer.push_back(action_id);

			if (action_id & 0x80)
			{
				// Action contains extra data.  Read it.
				int	length = in->read_u16();
				m_buffer.push_back(length & 0x0FF);
				m_buffer.push_back((length >> 8) & 0x0FF);
				for (int i = 0; i < length; i++)
				{
					unsigned char	b = in->read_u8();
					m_buffer.push_back(b);
				}
			}

			IF_VERBOSE_ACTION(log_msg("%4d\t", pc); log_disasm(&m_buffer[instruction_start]); );

			if (action_id == 0)
			{
				// end of action buffer.
				break;
			}
		}
	}


	void	action_buffer::process_decl_dict(int start_pc, int stop_pc)
	// Interpret the decl_dict opcode.  Don't read stop_pc or
	// later.  A dictionary is some static strings embedded in the
	// action buffer; there should only be one dictionary per
	// action buffer.
	//
	// NOTE: Normally the dictionary is declared as the first
	// action in an action buffer, but I've seen what looks like
	// some form of copy protection that amounts to:
	//
	// <start of action buffer>
	//          push true
	//          branch_if_true label
	//          decl_dict   [0]   // this is never executed, but has lots of orphan data declared in the opcode
	// label:   // (embedded inside the previous opcode; looks like an invalid jump)
	//          ... "protected" code here, including the real decl_dict opcode ...
	//          <end of the dummy decl_dict [0] opcode>
	//

	// So we just interpret the first decl_dict we come to, and
	// cache the results.  If we ever hit a different decl_dict in
	// the same action_buffer, then we log an error and ignore it.
	{
		assert(stop_pc <= m_buffer.size());

		if (m_decl_dict_processed_at == start_pc)
		{
			// We've already processed this decl_dict.
			int	count = m_buffer[start_pc + 3] | (m_buffer[start_pc + 4] << 8);
			assert(m_dictionary.size() == count);
			UNUSED(count);
			return;
		}

		if (m_decl_dict_processed_at != -1)
		{
			log_error("error: process_decl_dict(%d, %d): decl_dict was already processed at %d\n",
				  start_pc,
				  stop_pc,
				  m_decl_dict_processed_at);
			return;
		}

		m_decl_dict_processed_at = start_pc;

		// Actual processing.
		int	i = start_pc;
		int	length = m_buffer[i + 1] | (m_buffer[i + 2] << 8);
		int	count = m_buffer[i + 3] | (m_buffer[i + 4] << 8);
		i += 2;

		UNUSED(length);

		assert(start_pc + 3 + length == stop_pc);

		m_dictionary.resize(count);


		// Index the strings.
		for (int ct = 0; ct < count; ct++)
		{
			// Point into the current action buffer.
			m_dictionary[ct] = (const char*) &m_buffer[3 + i];

			while (m_buffer[3 + i])
			{
				// safety check.
				if (i >= stop_pc)
				{
					log_error("error: action buffer dict length exceeded\n");

					// Jam something into the remaining (invalid) entries.
					while (ct < count)
					{
						m_dictionary[ct] = "<invalid>";
						ct++;
					}
					return;
				}
				i++;
			}
			i++;
		}
	}


	void	action_buffer::execute(as_environment* env)
	// Interpret the actions in this action buffer, and evaluate
	// them in the given environment.  Execute our whole buffer,
	// without any arguments passed in.
	{
		// Flash keeps local variables after executing of frame actions
		// therefore we should not delete them
//		int	local_stack_top = env->get_local_frame_top();
//		env->add_frame_barrier();

		// sanity check
		assert(env->m_local_frames.size() < 1000);


		array<with_stack_entry>	empty_with_stack;
		execute(env, 0, m_buffer.size(), NULL, empty_with_stack, false /* not function2 */);

//		env->set_local_frame_top(local_stack_top);

	}


	// local function, called from 0x46 & 0x55 opcode implementation only
	void action_buffer::enumerate(as_environment* env, as_object_interface* obj)
	{
		assert(env);

		// The end of the enumeration
		as_value nullvalue;
		nullvalue.set_null();
		env->push(nullvalue);
		IF_VERBOSE_ACTION(log_msg("-------------- enumerate - push: NULL\n"));

		if (obj == NULL)
		{
			return;
		}

		obj->enumerate(env);
	}

	typedef exported_module as_plugin* (*gameswf_module_init)(const array<as_value>& params);

	as_plugin* action_buffer::load_as_plugin(const tu_string& classname,
						const array<as_value>& params)
	// loads user defined class from DLL / shared library
	{
		tu_loadlib* lib = new tu_loadlib(classname.c_str());

		// get module interface
		gameswf_module_init module_init = (gameswf_module_init) lib->get_function("gameswf_module_init");

		// create plugin instance
		if (module_init)
		{
			as_plugin* plugin = (module_init)(params);
			if (plugin)
			{
				plugin->m_lib = lib;
			}
			return plugin;
		}

		delete lib;
		return NULL;
	}
	
	void action_buffer::operator=(const action_buffer& ab)
	{
		// TODO: optimize
		m_buffer = ab.m_buffer;
		m_dictionary = ab.m_dictionary;
		m_decl_dict_processed_at = ab.m_decl_dict_processed_at;
	}

	as_object* action_buffer::create_proto(as_object* obj, const as_value& constructor)
	{
		as_value	prototype_val;
		constructor.to_object()->get_member("prototype", &prototype_val);
		as_object_interface* prototype = prototype_val.to_object();
		assert(prototype);
		prototype->copy_to(obj);

		as_object* proto = new as_object();
		proto->m_this_ptr = obj->cast_to_as_object()->m_this_ptr;

		as_value prototype_constructor;
		prototype->get_member("__constructor__", &prototype_constructor);
		proto->set_member("__constructor__", prototype_constructor);

		obj->m_proto = proto;
		return proto;
	}

	void	action_buffer::execute(
		as_environment* env,
		int start_pc,
		int exec_bytes,
		as_value* retval,
		const array<with_stack_entry>& initial_with_stack,
		bool is_function2)
	// Interpret the specified subset of the actions in our
	// buffer.  Caller is responsible for cleaning up our local
	// stack frame (it may have passed its arguments in via the
	// local stack frame).
	// 
	// The is_function2 flag determines whether to use global or local registers.
	{
		action_init();	// @@ stick this somewhere else; need some global static init function

		assert(env);

		array<with_stack_entry>	with_stack(initial_with_stack);

		// Some corner case behaviors depend on the SWF file version.
//		int version = env->get_target()->get_movie_definition()->get_version();
		int version = get_current_root()->get_movie_version();

#if 0
		// Check the time
		if (periodic_events.expired()) {
			periodic_events.poll_event_handlers(env);
		}
#endif
		
		character*	original_target = env->get_target();

		int	stop_pc = start_pc + exec_bytes;

		for (int pc = start_pc; pc < stop_pc; )
		{
			// Cleanup any expired "with" blocks.
			while (with_stack.size() > 0
			       && pc >= with_stack.back().m_block_end_pc)
			{
				// Drop this stack element
				with_stack.resize(with_stack.size() - 1);
			}

			// Get the opcode.
			int	action_id = m_buffer[pc];
			if ((action_id & 0x80) == 0)
			{
				IF_VERBOSE_ACTION(log_msg("EX:\t"); log_disasm(&m_buffer[pc]));

				// IF_VERBOSE_ACTION(log_msg("Action ID is: 0x%x\n", action_id));
			
				// Simple action; no extra data.
				switch (action_id)
				{
				default:
					break;

				case 0x00:	// end of actions.
					return;

				case 0x04:	// next frame.
					env->get_target()->goto_frame(env->get_target()->get_current_frame() + 1);
					break;

				case 0x05:	// prev frame.
					env->get_target()->goto_frame(env->get_target()->get_current_frame() - 1);
					break;

				case 0x06:	// action play
					env->get_target()->set_play_state(character::PLAY);
					break;

				case 0x07:	// action stop
					env->get_target()->set_play_state(character::STOP);
					break;

				case 0x08:	// toggle quality
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;

				case 0x09:	// stop sounds
					{
						sound_handler* s = get_sound_handler();
						if (s != NULL)
						{
							s->stop_all_sounds();
						}
					}
					break;

				case 0x0A:	// add
				{
					env->top(1) += env->top(0);
					env->drop(1);
					break;
				}
				case 0x0B:	// subtract
				{
					env->top(1) -= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0C:	// multiply
				{
					env->top(1) *= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0D:	// divide
				{
					env->top(1) /= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0E:	// equal
				{
					env->top(1).set_bool(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}
				case 0x0F:	// less than
				{
					env->top(1).set_bool(env->top(1) < env->top(0));
					env->drop(1);
					break;
				}
				case 0x10:	// logical and
				{
					env->top(1).set_bool(env->top(1).to_bool() && env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x11:	// logical or
				{
					env->top(1).set_bool(env->top(1).to_bool() || env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x12:	// logical not
				{
					env->top(0).set_bool(! env->top(0).to_bool());
					break;
				}
				case 0x13:	// string equal
				{
					env->top(1).set_bool(env->top(1).to_tu_string() == env->top(0).to_tu_string());
					env->drop(1);
					break;
				}
				case 0x14:	// string length
				{
					env->top(0).set_int(env->top(0).to_tu_string_versioned(version).utf8_length());
					break;
				}
				case 0x15:	// substring
				{
					int	size = int(env->top(0).to_number());
					int	base = int(env->top(1).to_number()) - 1;  // 1-based indices
					const tu_string&	str = env->top(2).to_tu_string_versioned(version);

					// Keep base within range.
					base = iclamp(base, 0, str.length());

					// Truncate if necessary.
					size = imin(str.length() - base, size);


					// @@ This can be done without new allocations if we get dirtier w/ internals
					// of as_value and tu_string...
					tu_string	new_string = str.c_str() + base;
					new_string.resize(size);

					env->drop(2);
					env->top(0).set_tu_string(new_string);

					break;
				}
				case 0x17:	// pop
				{
					if (env->m_stack.size() > 0)
					{
						env->drop(1);
					}
					else
					{
						log_error("error: empty stack\n");
					}
					break;
				}
				case 0x18:	// int
				{
					env->top(0).set_int(int(floor(env->top(0).to_number())));
					break;
				}
				case 0x1C:	// get variable
				{
					as_value var_name = env->pop();
					tu_string var_string = var_name.to_tu_string();

					as_value variable = env->get_variable(var_string, with_stack);
					env->push(variable);
					if (variable.to_object() == NULL) {
						IF_VERBOSE_ACTION(log_msg("-------------- get var: %s=%s\n",
									  var_string.c_str(),
									  variable.to_tu_string().c_str()));
					} else {
						IF_VERBOSE_ACTION(log_msg("-------------- get var: %s=%s at %p\n",
									  var_string.c_str(),
									  variable.to_tu_string().c_str(), variable.to_object()));
					}
					

					break;
				}
				case 0x1D:	// set variable
				{
					env->set_variable(env->top(1).to_tu_string(), env->top(0), with_stack);
					IF_VERBOSE_ACTION(log_msg("-------------- set var: %s \n",
								  env->top(1).to_tu_string().c_str()));

					env->drop(2);
					break;
				}
				case 0x20:	// set target expression
				{
					// Flash doc says:
					// This action behaves exactly like the original ActionSetTarget from SWF 3, 
					// but is stack-based to enable the target path to be the result of expression evaluation.
					env->set_target(env->top(0),original_target);
					env->drop(1);
					break;
				}
				case 0x21:	// string concat
				{
					tu_string str = env->top(1).to_tu_string_versioned(version);
					str += env->top(0).to_tu_string_versioned(version);
					env->drop(1);
					env->top(0).set_tu_string(str);
					break;
				}
				case 0x22:	// get property
				{

					character*	target = env->find_target(env->top(1));
					if (target)
					{
						env->top(1) = get_property(target, (int) env->top(0).to_number());
					}
					else
					{
						env->top(1) = as_value();
					}
					env->drop(1);
					break;
				}

				case 0x23:	// set property
				{

					character*	target = env->find_target(env->top(2));
					if (target)
					{
						set_property(target, (int) env->top(1).to_number(), env->top(0));
					}
					env->drop(3);
					break;
				}

				case 0x24:
				// duplicateMovieClip(target:String, newname:String, depth:Number) : Void
				// duplicateMovieClip(target:MovieClip, newname:String, depth:Number) : Void
				{
					character* target = NULL;
					switch (env->top(2).get_type())
					{
						case as_value::OBJECT:
							target = env->top(2).to_object()->cast_to_character();
							break;
						case as_value::STRING:
							{
								as_value val = env->get_variable(env->top(2).to_string(), with_stack);
								if (val.to_object() != NULL)
								{
									target = val.to_object()->cast_to_character();
								}
							}
							break;
						default:
							break;
					}

					if (target)
					{
						target->clone_display_object(
							env->top(1).to_tu_string(),
							(int) env->top(0).to_number());
					}

					env->drop(3);
					break;
				}

				case 0x25:	// remove clip
					env->get_target()->remove_display_object(env->top(0).to_tu_string());
					env->drop(1);
					break;

				case 0x26:	// trace
				{
					// Log the stack val.
					as_global_trace(fn_call(&env->top(0), NULL, env, 1, env->get_top_index()));
					env->drop(1);
					break;
				}

				case 0x27:	// start drag movie
				{
					character::drag_state	st;

					st.m_character = env->find_target(env->top(0));
					if (st.m_character == NULL)
					{
						log_error("error: start_drag of invalid target '%s'.\n",
							  env->top(0).to_string());
					}

					st.m_lock_center = env->top(1).to_bool();
					st.m_bound = env->top(2).to_bool();
					if (st.m_bound)
					{
						st.m_bound_x0 = (float) env->top(6).to_number();
						st.m_bound_y0 = (float) env->top(5).to_number();
						st.m_bound_x1 = (float) env->top(4).to_number();
						st.m_bound_y1 = (float) env->top(3).to_number();
						env->drop(4);
					}
					env->drop(3);

					character*	root_movie = env->get_target()->get_root_movie();
					assert(root_movie);

					if (root_movie && st.m_character)
					{
						root_movie->set_drag_state(st);
					}
					
					break;
				}

				case 0x28:	// stop drag movie
				{
					character*	root_movie = env->get_target()->get_root_movie();
					assert(root_movie);

					root_movie->stop_drag();

					break;
				}

				case 0x29:	// string less than
				{
					env->top(1).set_bool(env->top(1).to_tu_string() < env->top(0).to_tu_string());
					break;
				}

				case 0x2A:	// throw
				{
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x2B:	// cast_object
				{
					// TODO

					//
					// Pop o1, pop s2
					// Make sure o1 is an instance of s2.
					// If the cast succeeds, push o1, else push NULL.
					//
					// The cast doesn't appear to coerce at all, it's more
					// like a dynamic_cast<> in C++ I think.
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x2C:	// implements
				{
					// Declare that a class s1 implements one or more
					// interfaces (i2 == number of interfaces, s3..sn are the names
					// of the interfaces).
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x30:	// random
				{
					int	max = int(env->top(0).to_number());
					if (max < 1) max = 1;
					env->top(0).set_int(tu_random::next_random() % max);
					break;
				}
				case 0x31:	// mb length
				{
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x32:	// ord
				{
					// ASCII code of first character
					env->top(0).set_int(env->top(0).to_string()[0]);
					break;
				}
				case 0x33:	// chr
				{
					char	buf[2];
					buf[0] = int(env->top(0).to_number());
					buf[1] = 0;
					env->top(0).set_string(buf);
					break;
				}

				case 0x34:	// get timer
					// Push milliseconds since we started playing.
					env->push((double) (tu_timer::get_ticks() - s_start_time));
					break;

				case 0x35:	// mb substring
				{
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x37:	// mb chr
				{
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x3A:	// delete
				{
					tu_string varname(env->top(0).to_tu_string());
					as_object_interface* obj_interface = env->top(1).to_object();
					env->drop(1);

					bool retcode = false;
					if (obj_interface) {
						as_value val;
						if (obj_interface->get_member(varname, &val)) {
							obj_interface->set_member(varname, as_value());
							retcode = true;
						}
					}

					env->top(0).set_bool(retcode);
					break;
				}
				case 0x3B:	// delete2
				{
					tu_string varname(env->top(0).to_tu_string());
					as_value obj(env->get_variable_raw(varname, with_stack));

					if (obj.get_type() != as_value::UNDEFINED)
					{
						// drop refs
						obj.set_undefined();

						// finally to remove it we need set varname to undefined

						// try delete local var
						int	local_index = env->find_local(varname, true);
						if (local_index >= 0)
						{
							// local var is found
							env->m_local_frames[local_index].m_value.set_undefined();
							env->top(0).set_bool(true);
							break;
						}
						else
						{
							// check var in m_variables
							as_value val;
							if (env->m_variables.get(varname, &val))
							{
								env->m_variables.set(varname, obj);
								env->top(0).set_bool(true);
								break;
							}
						}
					}

					// can't delete var
					env->top(0).set_bool(false);
					break;
				}

				case 0x3C:	// set local
				{
					as_value	value = env->pop();
					as_value	varname = env->pop();
					env->set_local(varname.to_tu_string(), value);
					break;
				}

				case 0x3D:	// call function
				{
					as_value	function;
					if (env->top(0).get_type() == as_value::STRING)
					{
						// Function is a string; lookup the function.
						const tu_string&	function_name = env->top(0).to_tu_string();
						function = env->get_variable(function_name, with_stack);

						// super constructor, Flash 6 
						if (function.get_type() == as_value::OBJECT)
						{
							as_object_interface* obj = function.to_object();
							if (obj)
							{
								assert(function_name == "super");
								obj->get_member("constructor", &function);
							}
						}

						if (function.get_type() != as_value::C_FUNCTION
						    && function.get_type() != as_value::AS_FUNCTION)
						{
							log_error("error in call_function: '%s' is not a function\n",
								  function_name.c_str());
						}
					}
					else
					{
						// Hopefully the actual function object is here.
						function = env->top(0);
					}
					int	nargs = (int) env->top(1).to_number();
					as_value	result = call_method(function, env, NULL, nargs, env->get_top_index() - 2);
					env->drop(nargs + 1);
					env->top(0) = result;
					break;
				}
				case 0x3E:	// return
				{
					// Put top of stack in the provided a slot, if
					// it's not NULL.
					if (retval)
					{
						*retval = env->top(0);
					}
					env->drop(1);

					// Skip the rest of this buffer (return from this action_buffer).
					pc = stop_pc;

					break;
				}
				case 0x3F:	// modulo
				{
					as_value	result;
					double	y = env->pop().to_number();
					double	x = env->pop().to_number();
					if (y != 0)
					{
						result.set_double(fmod(x, y));
					}
					env->push(result);
					break;
				}
				case 0x40:	// new
				{
					as_value	classname = env->pop();
					IF_VERBOSE_ACTION(log_msg("-------------- new object: %s\n",
								  classname.to_tu_string().c_str()));
					int	nargs = (int) env->pop().to_number();
					as_value constructor = env->get_variable(classname.to_tu_string(), with_stack);
					as_value new_obj;
					if (constructor.get_type() == as_value::C_FUNCTION)
					{
						// C function is responsible for creating the new object and setting members.
						(constructor.to_c_function())(fn_call(&new_obj, NULL, env, nargs, env->get_top_index()));
					}
					else if (as_as_function* ctor_as_func = constructor.to_as_function())
					{

						// Create an empty object
						smart_ptr<as_object>	new_obj_ptr = new as_object();
						as_object* proto = create_proto(new_obj_ptr.get_ptr(), constructor);
						proto->m_this_ptr = new_obj_ptr.get_ptr();

						// Call the actual constructor function; new_obj is its 'this'.
						// We don't need the function result.
						call_method(constructor, env, new_obj_ptr.get_ptr(), nargs, env->get_top_index());
						new_obj.set_as_object_interface(new_obj_ptr.get_ptr());
					}
					else
					{
						if (classname != "String")
						{
							// try to load user defined class from DLL / shared library
							array<as_value> params;
							int first_arg_bottom_index = env->get_top_index();
							for (int i = 0; i < nargs; i++)
							{
								params.push_back(env->bottom(first_arg_bottom_index - i));
							}

							as_plugin* plugin = load_as_plugin(classname.to_tu_string(), params);
							if (plugin)
							{
								new_obj.set_as_object_interface(plugin);
							}
							else
							{
								log_error("can't create object with unknown class '%s'\n",
									  classname.to_tu_string().c_str());
							}
						}
						else
						{
							log_msg("Created special String class\n");
						}
					}

					// places new object to heap
					if (new_obj.to_object())
					{
						get_heap()->set(new_obj.to_object(), false);
					}

					env->drop(nargs);
					env->push(new_obj);
					break;
				}
				case 0x41:	// declare local
				{
					const tu_string&	varname = env->top(0).to_tu_string();
					env->declare_local(varname);
					env->drop(1);
					break;
				}
				case 0x42:	// init array
				{
 					// Call the array constructor
 					as_value	result;
 					as_global_array_ctor(fn_call(&result, NULL, env, -1, -1));
 					as_object_interface* ao = result.to_object();
 					assert(ao);
					UNUSED(ao);
 					env->push(result);
					break;
				}
				case 0x43:	// declare object
				{
				// Pops elems off of the stack. Pops [value1, name1, …, valueN, nameN] off the stack.
				// It does the following:
				// 1 Pops the number of initial properties from the stack.
				// 2 Initializes the object as a ScriptObject.
				// 3 Sets the object type to “Object”.
				// 4 Pops each initial property off the stack. For each initial property, the value of the property is
				// popped off the stack, then the name of the property is popped off the stack. The name of the
				// property is converted to a string. The value may be of any type.				{

					int n = (int) env->pop().to_number();
					as_object* obj = new as_object();

					// Set members
					for (int i = 0; i < n; i++)
					{
						obj->set_member(env->top(1).to_tu_string(), env->top(0));
						env->drop(2);
					}

					env->push(obj); 
					break;
				}
				case 0x44:	// type of
				{
					switch(env->top(0).get_type())
					{
					case as_value::UNDEFINED:
						env->top(0).set_string("undefined");
						break;
					case as_value::STRING:
						env->top(0).set_string("string");
						break;
					case as_value::NUMBER:
						env->top(0).set_string("number");
						break;
					case as_value::BOOLEAN:
						env->top(0).set_string("boolean");
						break;
					case as_value::OBJECT:
						if (env->top(0).to_object())
						{
							if (env->top(0).to_object()->cast_to_sprite())
							{
								env->top(0).set_string("movieclip");
								break;
							}
						}
						env->top(0).set_string("object");

						break;
					case as_value::NULLTYPE:
						env->top(0).set_string("null");
						break;
					case as_value::AS_FUNCTION:
					case as_value::C_FUNCTION:
						env->top(0).set_string("function");
						break;
					default:
						log_error("typeof unknown type: %02X\n", env->top(0).get_type());
						break;
					}
					break;
				}
				case 0x45:	// get target
				{
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x46:	// enumerate
				{
					as_value var_name = env->pop();
					const tu_string& var_string = var_name.to_tu_string();
					as_value variable = env->get_variable(var_string, with_stack);
					enumerate(env, variable.to_object());
					break;
				}
				case 0x47:	// add_t (typed)
				{
					if (env->top(0).get_type() == as_value::STRING
					    || env->top(1).get_type() == as_value::STRING)
					{
						env->top(1).set_tu_string(env->top(1).call_to_string(env));
						env->top(1).string_concat(env->top(0).call_to_string(env));
					}
					else
					{
						env->top(1) += env->top(0);
					}
					env->drop(1);
					break;
				}
				case 0x48:	// less than (typed)
				{
					if (env->top(1).get_type() == as_value::STRING)
					{
						env->top(1).set_bool(env->top(1).to_tu_string() < env->top(0).to_tu_string());
					}
					else
					{
						env->top(1).set_bool(env->top(1) < env->top(0));
					}
					env->drop(1);
					break;
				}
				case 0x49:	// equal (typed)
				{
					// @@ identical to untyped equal, as far as I can tell...
					env->top(1).set_bool(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}
				case 0x4A:	// to number
				{
					double n = env->top(0).to_number();
					env->top(0).set_double(n);
					break;
				}
				case 0x4B:	// to string
				{
					tu_string str = env->top(0).to_tu_string_versioned(version);
					env->top(0).set_tu_string(str);
					break;
				}
				case 0x4C:	// dup
					env->push(env->top(0));
					break;
				
				case 0x4D:	// swap
				{
					as_value	temp = env->top(1);
					env->top(1) = env->top(0);
					env->top(0) = temp;
					break;
				}
				case 0x4E:	// get member
				{
					as_object_interface*	obj = env->top(1).to_object();

					// Special case: String has a member "length"
					if (obj == NULL
					    && env->top(1).get_type() == as_value::STRING
					    && env->top(0).to_tu_stringi() == "length")
					{
						int	len = env->top(1).to_tu_string_versioned(version).utf8_length();
						env->top(1).set_int(len);
					}
					else
					{
						env->top(1).set_undefined();
						if (obj)
						{
							if (obj->get_member(env->top(0).to_tu_string(), &(env->top(1))) == false)
							{
								// try '__resolve' property
								as_value val;
								if (obj->get_member("__resolve", &val))
								{
									// call __resolve
									as_as_function* resolve = val.to_as_function();
									if (resolve)
									{
										(*resolve)(fn_call(&val, obj, env, 1, env->get_top_index()));
										env->top(1) = val;
									}
								}
							}

							if (env->top(1).to_object() == NULL) {
								IF_VERBOSE_ACTION(log_msg("-------------- get_member %s=%s\n",
												env->top(0).to_tu_string().c_str(),
												env->top(1).to_tu_string().c_str()));
							} else {
								IF_VERBOSE_ACTION(log_msg("-------------- get_member %s=%s at %p\n",
												env->top(0).to_tu_string().c_str(),
												env->top(1).to_tu_string().c_str(), env->top(1).to_object()));
							}
						}
						else
						{
							// @@ log error?
						}
					}
					env->drop(1);
					break;
				}
				case 0x4F:	// set member
				{
					as_object_interface*	obj = env->top(2).to_object();
					if (obj)
					{
						obj->set_member(env->top(1).to_tu_string(), env->top(0));
						IF_VERBOSE_ACTION(
							log_msg("-------------- set_member [%p].%s=%s\n",
//								env->top(2).to_tu_string().c_str(),
								obj,
								env->top(1).to_tu_string().c_str(),
								env->top(0).to_tu_string().c_str()));
					}
					else
					{
						// Invalid object, can't set.
						IF_VERBOSE_ACTION(
							log_msg("-------------- set_member %s.%s=%s on invalid object!\n",
								env->top(2).to_tu_string().c_str(),
								env->top(1).to_tu_string().c_str(),
								env->top(0).to_tu_string().c_str()));
					}
					env->drop(3);
					break;
				}
				case 0x50:	// increment
					env->top(0) += 1;
					break;
				case 0x51:	// decrement
					env->top(0) -= 1;
					break;
				case 0x52:	// call method
				{
					int	nargs = (int) env->top(2).to_number();
					as_value	result;
					const tu_string&	method_name = env->top(0).to_tu_string();

					as_object_interface* obj = env->top(1).to_object();
					if (obj) {
						as_value	method;
						if (obj->get_member(method_name, &method))
						{
							result = call_method(
								method,
								env,
								obj,
								nargs,
								env->get_top_index() - 3);
						}
						else
						{
							if (method_name == "undefined" || method_name == "")
							{
								// Flash help says:
								// If the method name is blank or undefined, the object is taken to be 
								// a function object that should be invoked, 
								// rather than the container object of a method. For example, if
								// CallMethod is invoked with object obj and method name blank,
								// it's equivalent to using the syntax:
								// obj();

								assert(obj->cast_to_as_object());
								if (obj->cast_to_as_object()->m_this_ptr != NULL)
								{
									as_value constructor;
									if (obj->get_member("__constructor__", &constructor))
									{
										create_proto(obj->cast_to_as_object(), constructor);
										result = call_method(
										constructor,
										env,
										obj,
										nargs,
										env->get_top_index() - 3);
									}
								}
							}
							else
							{
								log_error("error: call_method can't find method %s\n",
										method_name.c_str());
							}
						}
					}
					else if (env->top(1).get_type() == as_value::STRING)
					{
						// Handle methods on literal strings.
						string_method(
							fn_call(&result, NULL, env, nargs, env->get_top_index() - 3),
							method_name.to_tu_stringi(),
							env->top(1).to_tu_string_versioned(version));
					}
					else if (env->top(1).get_type() == as_value::C_FUNCTION)
					{
						// Catch method calls on String
						// constructor.  There may be a cleaner
						// way to do this. Perhaps we call the
						// constructor function with a special flag, to
						// indicate that it's a method call?
						if (env->top(1).to_c_function() == string_ctor)
						{
							tu_string dummy;
							string_method(
								fn_call(&result, NULL, env, nargs, env->get_top_index() - 3),
								method_name.to_tu_stringi(),
								dummy);
						}
						else
						{
							log_error("error: method call on unknown c function.\n");
						}
					}
					else
					{
						if (env->top(1).get_type() == as_value::NUMBER
						    && method_name == "toString")
						{
							// Numbers have a .toString() method.
							if (nargs > 0)
							{
								result.set_tu_string(number_to_string((int) env->top(3).to_number(),
								(int) env->top(1).to_number()));
							}
							else
							{
								result.set_tu_string(env->top(1).to_tu_string());
							}
						}
						else
						{
							log_error("error: call_method '%s' on invalid object.\n",
								  method_name.c_str());
						}
					}
					env->drop(nargs + 2);
					env->top(0) = result;
					break;
				}

				case 0x53:	// new method
				{

				// Invokes a constructor function to create a new object.
				// A new object is constructed and passed to the constructor function 
				// as the value of the this keyword. Arguments may be specified to the
				// constructor function. The return value from the constructor function 
				// is discarded. The newly constructed object is pushed to the stack. 
				// Similar to ActionCallMethod and ActionNewObject.

				// 1 Pops the name of the method from the stack.
				// 2 Pops the ScriptObject from the stack. If the name of the method is blank,
				// the ScriptObject is treated as a function object which is invoked as 
				// the constructor function. If the method name is not blank, the named method
				// of the ScriptObject is invoked.
				// 3 Pops the number of arguments from the stack.
				// 4 Executes the method call.
				// 5 Pushes the newly constructed object to the stack. 
				// Note, if there is no appropriate return value (i.e: the function does not have
				// a “return” statement), a “push undefined” is generated by the
				// compiler and is pushed to the stack. 
				// The “undefined” return value should be popped off the stack.

					as_value	constructor = env->pop();
					as_object_interface*	obj = env->pop().to_object();
					int	nargs = (int) env->pop().to_number();

					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
/*
					// Create an empty object
					smart_ptr<as_object>	new_obj = new as_object();

					if (constructor.get_type() == as_value::C_FUNCTION)
					{
						// C function is responsible for creating the new object and setting members.
						// TODO: test
						(constructor.to_c_function())(fn_call(NULL, new_obj.get_ptr(), env, nargs, env->get_top_index()));
					}
					else if (as_as_function* ctor_as_func = constructor.to_as_function())
					{
						as_object* proto = create_proto(new_obj.get_ptr(), constructor);
						proto->m_this_ptr = new_obj.get_ptr();

						// Call the actual constructor function; new_obj is its 'this'.
						// We don't need the function result.
						call_method(constructor, env, new_obj.get_ptr(), nargs, env->get_top_index());
					}
					else
					{
						as_value val;
						if (obj->get_member(constructor.to_string(), &val))
						{
							as_as_function* func = val.to_as_function();
							assert(func);
							as_value prototype;
							func->m_properties->get_member("prototype", &prototype);
							prototype.to_object()->set_member("__constructor__", func);

							as_object* proto = create_proto(new_obj.get_ptr(), func);

							proto->m_this_ptr = new_obj.get_ptr();

							// Call the actual constructor function; new_obj is its 'this'.
							// We don't need the function result.
							call_method(func, env, new_obj.get_ptr(), nargs, env->get_top_index());
						}

					}


					// places new object to heap
					get_heap()->set(new_obj.get_ptr(), false);
*/
					env->drop(nargs);
//					env->push(new_obj.get_ptr());
					env->push(as_value());	// hack
					break;
				}

				case 0x54:	// instance of
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				case 0x55:	// enumerate object
				{
					as_value variable = env->pop();
					enumerate(env, variable.to_object());
					break;
				}
				case 0x60:	// bitwise and
					env->top(1) &= env->top(0);
					env->drop(1);
					break;
				case 0x61:	// bitwise or
					env->top(1) |= env->top(0);
					env->drop(1);
					break;
				case 0x62:	// bitwise xor
					env->top(1) ^= env->top(0);
					env->drop(1);
					break;
				case 0x63:	// shift left
					env->top(1).shl(env->top(0));
					env->drop(1);
					break;
				case 0x64:	// shift right (signed)
					env->top(1).asr(env->top(0));
					env->drop(1);
					break;
				case 0x65:	// shift right (unsigned)
					env->top(1).lsr(env->top(0));
					env->drop(1);
					break;
				case 0x66:	// strict equal
					if (env->top(1).get_type() != env->top(0).get_type())
					{
						// Types don't match.
						env->top(1).set_bool(false);
						env->drop(1);
					}
					else
					{
						env->top(1).set_bool(env->top(1) == env->top(0));
						env->drop(1);
					}
					break;
				case 0x67:	// gt (typed)
					if (env->top(1).get_type() == as_value::STRING)
					{
						env->top(1).set_bool(env->top(1).to_tu_string() > env->top(0).to_tu_string());
					}
					else
					{
						env->top(1).set_bool(env->top(1).to_number() > env->top(0).to_number());
					}
					env->drop(1);
					break;
				case 0x68:	// string gt
					env->top(1).set_bool(env->top(1).to_tu_string() > env->top(0).to_tu_string());
					env->drop(1);
					break;

				case 0x69:	// extends

					// These steps are the equivalent to the following ActionScript:
					// Subclass.prototype = new Object();
					// Subclass.prototype.__proto__ = Superclass.prototype;
					// Subclass.prototype.__constructor__ = Superclass;

					as_value& super = env->top(0);
					as_value& sub = env->top(1);

					// TODO: extends MovieClip, ...
					assert(super.to_object());
					assert(sub.to_object());

					as_value super_prototype;
					super.to_object()->get_member("prototype", &super_prototype);

					smart_ptr<as_object> new_prototype = new as_object();
					new_prototype->m_proto = super_prototype.to_object();
					new_prototype->set_member("__constructor__", super);

					sub.to_object()->set_member("prototype", new_prototype.get_ptr());

					env->drop(2);
					break;

				}
				pc++;	// advance to next action.
			}
			else
			{
				IF_VERBOSE_ACTION(log_msg("EX:\t"); log_disasm(&m_buffer[pc]));

				// Action containing extra data.
				int	length = m_buffer[pc + 1] | (m_buffer[pc + 2] << 8);
				int	next_pc = pc + length + 3;

				switch (action_id)
				{
				default:
					break;

				case 0x81:	// goto frame
				{
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					// 0-based already?
					//// Convert from 1-based to 0-based
					//frame--;
					env->get_target()->goto_frame(frame);
					break;
				}

				case 0x83:	// get url
				{
					// If this is an FSCommand, then call the callback
					// handler, if any.

					// Two strings as args.
					const char*	url = (const char*) &(m_buffer[pc + 3]);
					int	url_len = strlen(url);
					const char*	target = (const char*) &(m_buffer[pc + 3 + url_len + 1]);

					// If the url starts with "FSCommand:", then this is
					// a message for the host app.
					if (strncmp(url, "FSCommand:", 10) == 0)
					{
						if (s_fscommand_handler)
						{
							// Call into the app.
							(*s_fscommand_handler)(env->get_target()->get_root_interface(), url + 10, target);
						}
					}
					else
					{
						env->load_file(url, as_value(target));
					}

					break;
				}

				case 0x87:	// store_register
				{
					int	reg = m_buffer[pc + 3];
					// Save top of stack in specified register.
					if (is_function2)
					{
						env->set_register(reg, env->top(0));

						IF_VERBOSE_ACTION(
							log_msg("-------------- local register[%d] = '%s'\n",
								reg,
								env->top(0).to_string()));
					}
					else if (reg >= 0 && reg < 4)
					{
						env->m_global_register[reg] = env->top(0);
					
						IF_VERBOSE_ACTION(
							log_msg("-------------- global register[%d] = '%s'\n",
								reg,
								env->top(0).to_string()));
					}
					else
					{
						log_error("store_register[%d] -- register out of bounds!", reg);
					}

					break;
				}

				case 0x88:	// decl_dict: declare dictionary
				{
					int	i = pc;
					//int	count = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					i += 2;

					process_decl_dict(pc, next_pc);

					break;
				}

				case 0x8A:	// wait for frame
				{
					// If we haven't loaded a specified frame yet, then we're supposed to skip
					// some specified number of actions.
					//
					// Since we don't load incrementally, just ignore this opcode.
					break;
				}

				case 0x8B:	// set target
				{
					// Change the movie we're working on.
					as_value val = (const char*) &m_buffer[pc + 3];
					env->set_target(val, original_target);
					break;
				}

				case 0x8C:	// go to labeled frame, goto_frame_lbl
				{
					char*	frame_label = (char*) &m_buffer[pc + 3];
					env->get_target()->goto_labeled_frame(frame_label);
					break;
				}

				case 0x8D:	// wait for frame expression (?)
				{
					// Pop the frame number to wait for; if it's not loaded skip the
					// specified number of actions.
					//
					// Since we don't support incremental loading, pop our arg and
					// don't do anything.
					env->drop(1);
					break;
				}

				case 0x8E:	// function2
				{
					as_as_function*	func = new as_as_function(this, next_pc, with_stack);
					func->set_target(env->get_target());
					func->set_is_function2();

					int	i = pc;
					i += 3;

					// Extract name.
					// @@ security: watch out for possible missing terminator here!
					tu_string	name = (const char*) &m_buffer[i];
					i += name.length() + 1;

					// Get number of arguments.
					int	nargs = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;

					// Get the count of local registers used by this function.
					uint8	register_count = m_buffer[i];
					i += 1;
					func->set_local_register_count(register_count);

					// Flags, for controlling register assignment of implicit args.
					uint16	flags = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;
					func->set_function2_flags(flags);

					// Get the register assignments and names of the arguments.
					for (int n = 0; n < nargs; n++)
					{
						int	arg_register = m_buffer[i];
						i++;

						// @@ security: watch out for possible missing terminator here!
						func->add_arg(arg_register, (const char*) &m_buffer[i]);
						i += func->m_args.back().m_name.length() + 1;
					}

					// Get the length of the actual function code.
					int	length = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;
					func->set_length(length);

					// Skip the function body (don't interpret it now).
					next_pc += length;

					// If we have a name, then save the function in this
					// environment under that name.
					as_value	function_value(func);
					if (name.length() > 0)
					{
						// @@ NOTE: should this be m_target->set_variable()???
						env->set_member(name, function_value);
					}

					// Also leave it on the stack.
					env->push_val(function_value);

					break;
				}

				case 0x94:	// with
				{
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					UNUSED(frame);
					IF_VERBOSE_ACTION(log_msg("-------------- with block start: stack size is %d\n", with_stack.size()));
					if (with_stack.size() < 8)
					{
 						int	block_length = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
 						int	block_end = next_pc + block_length;
 						as_object_interface*	with_obj = env->top(0).to_object();
 						with_stack.push_back(with_stack_entry(with_obj, block_end));
					}
					env->drop(1);
					break;
				}
				case 0x96:	// push_data
				{
					int i = pc;
					while (i - pc < length)
					{
						int	type = m_buffer[3 + i];
						i++;
						if (type == 0)
						{
							// string
							const char*	str = (const char*) &m_buffer[3 + i];
							i += strlen(str) + 1;
							env->push(str);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", str));
						}
						else if (type == 1)
						{
							// float (little-endian)
							union {
								float	f;
								Uint32	i;
							} u;
							compiler_assert(sizeof(u) == sizeof(u.i));

							memcpy(&u.i, &m_buffer[3 + i], 4);
							u.i = swap_le32(u.i);
							i += 4;

							env->push(u.f);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed '%f'\n", u.f));
						}
						else if (type == 2)
						{
							as_value nullvalue;
							nullvalue.set_null();
							env->push(nullvalue);	

							IF_VERBOSE_ACTION(log_msg("-------------- pushed NULL\n"));
						}
						else if (type == 3)
						{
							env->push(as_value());

							IF_VERBOSE_ACTION(log_msg("-------------- pushed UNDEFINED\n"));
						}
						else if (type == 4)
						{
							// contents of register
							int	reg = m_buffer[3 + i];
							UNUSED(reg);
							i++;
							if (is_function2)
							{
								env->push(*env->get_register(reg));
								IF_VERBOSE_ACTION(
									log_msg("-------------- pushed local register[%d] = '%s'\n",
										reg,
										env->top(0).to_string()));
							}
							else if (reg < 0 || reg >= 4)
							{
								env->push(as_value());
								log_error("push register[%d] -- register out of bounds!\n", reg);
							}
							else
							{
								env->push(env->m_global_register[reg]);
								IF_VERBOSE_ACTION(
									log_msg("-------------- pushed global register[%d] = '%s'\n",
										reg,
										env->top(0).to_string()));
							}

						}
						else if (type == 5)
						{
							bool	bool_val = m_buffer[3 + i] ? true : false;
							i++;
//							log_msg("bool(%d)\n", bool_val);
							env->push(bool_val);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed %s\n", bool_val ? "true" : "false"));
						}
						else if (type == 6)
						{
							// double
							// wacky format: 45670123
							union {
								double	d;
								Uint64	i;
								struct {
									Uint32	lo;
									Uint32	hi;
								} sub;
							} u;
							compiler_assert(sizeof(u) == sizeof(u.i));

							memcpy(&u.sub.hi, &m_buffer[3 + i], 4);
							memcpy(&u.sub.lo, &m_buffer[3 + i + 4], 4);
							u.i = swap_le64(u.i);
							i += 8;

							env->push(u.d);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed double %f\n", u.d));
						}
						else if (type == 7)
						{
							// int32
							Sint32	val = m_buffer[3 + i]
								| (m_buffer[3 + i + 1] << 8)
								| (m_buffer[3 + i + 2] << 16)
								| (m_buffer[3 + i + 3] << 24);
							i += 4;
						
							env->push(val);


							IF_VERBOSE_ACTION(log_msg("-------------- pushed int32 %d\n", val));
						}
						else if (type == 8)
						{
							int	id = m_buffer[3 + i];
							i++;
							if (id < m_dictionary.size())
							{
								env->push(m_dictionary[id]);

								IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", m_dictionary[id].c_str()));
							}
							else
							{
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								env->push(0);
								IF_VERBOSE_ACTION(log_msg("-------------- pushed 0\n"));
							}
						}
						else if (type == 9)
						{
							int	id = m_buffer[3 + i] | (m_buffer[4 + i] << 8);
							i += 2;
							if (id < m_dictionary.size())
							{
								env->push(m_dictionary[id]);
								IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", m_dictionary[id].c_str()));
							}
							else
							{
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								env->push(0);

								IF_VERBOSE_ACTION(log_msg("-------------- pushed 0"));
							}
						}
					}
					
					break;
				}
				case 0x99:	// branch always (goto)
				{
					Sint16	offset = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					next_pc += offset;
					// @@ TODO range checks
					break;
				}
				case 0x9A:	// get url 2
				{
					int	method = m_buffer[pc + 3];
					UNUSED(method);

					const char*	target = env->top(0).to_string();
					const char*	url = env->top(1).to_string();

					// If the url starts with "FSCommand:", then this is
					// a message for the host app.
					if (strncmp(url, "FSCommand:", 10) == 0)
					{
						if (s_fscommand_handler)
						{
							// Call into the app.
							(*s_fscommand_handler)(env->get_target()->get_root_interface(), url + 10, target);
						}
					}
					else
					{
						env->load_file(url, env->top(0));
					}
					env->drop(2);
					break;
				}

				case 0x9B:	// declare function
				{
					as_as_function*	func = new as_as_function(this, next_pc, with_stack);
					func->set_target(env->get_target());

					int	i = pc;
					i += 3;

					// Extract name.
					// @@ security: watch out for possible missing terminator here!
					tu_string	name = (const char*) &m_buffer[i];
					i += name.length() + 1;

					// Get number of arguments.
					int	nargs = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;

					// Get the names of the arguments.
					for (int n = 0; n < nargs; n++)
					{
						// @@ security: watch out for possible missing terminator here!
						func->add_arg(0, (const char*) &m_buffer[i]);
						i += func->m_args.back().m_name.length() + 1;
					}

					// Get the length of the actual function code.
					int	length = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;
					func->set_length(length);

					// Skip the function body (don't interpret it now).
					next_pc += length;

					// If we have a name, then save the function in this
					// environment under that name.
					as_value	function_value(func);
					if (name.length() > 0)
					{
						// @@ NOTE: should this be m_target->set_variable()???
						env->set_member(name, function_value);
					}

					// Also leave it on the stack.
					env->push_val(function_value);

					break;
				}

				case 0x9D:	// branch if true
				{
					Sint16	offset = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					
					bool	test = env->top(0).to_bool();
					env->drop(1);
					if (test)
					{
						IF_VERBOSE_ACTION(log_msg("-------------- branch is done\n"));
						next_pc += offset;

						if (next_pc > stop_pc)
						{
							log_error("branch to offset %d -- this section only runs to %d\n",
								  next_pc,
								  stop_pc);
						}
					}
					else
					{
						IF_VERBOSE_ACTION(log_msg("-------------- no branch\n"));
					}
					break;
				}
				case 0x9E:	// call frame
				{
					// Note: no extra data in this instruction!
					assert(env->m_target);
					env->m_target->call_frame_actions(env->top(0));
					env->drop(1);

					break;
				}

				case 0x9F:	// goto frame expression, goto_frame_exp
				{
					// From Alexi's SWF ref:
					//
					// Pop a value or a string and jump to the specified
					// frame. When a string is specified, it can include a
					// path to a sprite as in:
					// 
					//   /Test:55
					// 
					// When f_play is ON, the action is to play as soon as
					// that frame is reached. Otherwise, the
					// frame is shown in stop mode.

					unsigned char	play_flag = m_buffer[pc + 3];
					character::play_state	state = play_flag ? character::PLAY : character::STOP;

					character* target = env->get_target();
					bool success = false;

					if (env->top(0).get_type() == as_value::UNDEFINED)
					{
						// No-op.
					}
					else if (env->top(0).get_type() == as_value::STRING)
					{
						// @@ TODO: parse possible sprite path...
						
						// Also, if the frame spec is actually a number (not a label), then
						// we need to do the conversion...

						const char* frame_label = env->top(0).to_string();
						if (target->goto_labeled_frame(frame_label))
						{
							success = true;
						}
						else
						{
							// Couldn't find the label.  Try converting to a number.
							double num;
							if (string_to_number(&num, env->top(0).to_string()))
							{
								int frame_number = int(num);
								target->goto_frame(frame_number);
								success = true;
							}
							// else no-op.
						}
					}
					else if (env->top(0).get_type() == as_value::OBJECT)
					{
						// This is a no-op; see test_goto_frame.swf
					}
					else if (env->top(0).get_type() == as_value::NUMBER)
					{
						// Frame numbers appear to be 0-based!  @@ Verify.
						int frame_number = int(env->top(0).to_number());
						target->goto_frame(frame_number);
						success = true;
					}

					if (success)
					{
						target->set_play_state(state);
					}
					
					env->drop(1);

					break;
				}
				
				}
				pc = next_pc;
			}
		}

		env->set_target(original_target);
	}

	//
	// event_id
	//


	static tu_string	s_function_names[event_id::EVENT_COUNT] =
	{
		"INVALID",		 // INVALID
		"onPress",		 // PRESS
		"onRelease",		 // RELEASE
		"onRelease_Outside",	 // RELEASE_OUTSIDE
		"onRoll_Over",		 // ROLL_OVER
		"onRoll_Out",		 // ROLL_OUT
		"onDrag_Over",		 // DRAG_OVER
		"onDrag_Out",		 // DRAG_OUT
		"onKeyPress",		 // KEY_PRESS
		"onInitialize",		 // INITIALIZE

		"onLoad",		 // LOAD
		"onUnload",		 // UNLOAD
		"onEnterFrame",		 // ENTER_FRAME
		"onMouseDown",		 // MOUSE_DOWN
		"onMouseUp",		 // MOUSE_UP
		"onMouseMove",		 // MOUSE_MOVE
		"onKeyDown",		 // KEY_DOWN
		"onKeyUp",		 // KEY_UP
		"onData",		 // DATA

		"onConstruct",
		"onSetFocus",
		"onKillFocus",

		// MovieClipLoader events
		"onLoadComplete",
		"onLoadError",
		"onLoadInit",
		"onLoadProgress",
		"onLoadStart"

	};

	const tu_string&	event_id::get_function_name() const
	{
		assert(m_id > INVALID && m_id < EVENT_COUNT);
		return s_function_names[m_id];
	}


	// Standard member lookup.
	static stringi_hash<as_standard_member>	s_standard_member_map;

	void clear_standard_member_map()
	{
		s_standard_member_map.clear();
	}

	as_standard_member	get_standard_member(const tu_stringi& name)
	{
		if (s_standard_member_map.size() == 0)
		{
			s_standard_member_map.set_capacity(int(AS_STANDARD_MEMBER_COUNT));

			s_standard_member_map.add("_x", M_X);
			s_standard_member_map.add("_y", M_Y);
			s_standard_member_map.add("_xscale", M_XSCALE);
			s_standard_member_map.add("_yscale", M_YSCALE);
			s_standard_member_map.add("_currentframe", M_CURRENTFRAME);
			s_standard_member_map.add("_totalframes", M_TOTALFRAMES);
			s_standard_member_map.add("_alpha", M_ALPHA);
			s_standard_member_map.add("_visible", M_VISIBLE);
			s_standard_member_map.add("_width", M_WIDTH);
			s_standard_member_map.add("_height", M_HEIGHT);
			s_standard_member_map.add("_rotation", M_ROTATION);
			s_standard_member_map.add("_target", M_TARGET);
			s_standard_member_map.add("_framesloaded", M_FRAMESLOADED);
			s_standard_member_map.add("_name", M_NAME);
			s_standard_member_map.add("_droptarget", M_DROPTARGET);
			s_standard_member_map.add("_url", M_URL);
			s_standard_member_map.add("_highquality", M_HIGHQUALITY);
			s_standard_member_map.add("_focusrect", M_FOCUSRECT);
			s_standard_member_map.add("_soundbuftime", M_SOUNDBUFTIME);
			s_standard_member_map.add("_xmouse", M_XMOUSE);
			s_standard_member_map.add("_ymouse", M_YMOUSE);
			s_standard_member_map.add("_parent", M_PARENT);
//			s_standard_member_map.add("text", M_TEXT);
//			s_standard_member_map.add("textWidth", M_TEXTWIDTH);
//			s_standard_member_map.add("textColor", M_TEXTCOLOR);
//			s_standard_member_map.add("onLoad", M_ONLOAD);
		}

		as_standard_member	result = M_INVALID_MEMBER;
		s_standard_member_map.get(name, &result);

		return result;
	}


	//
	// Disassembler
	//


#define COMPILE_DISASM 1

#ifndef COMPILE_DISASM

	void	log_disasm(const unsigned char* instruction_data)
	// No disassembler in this version...
	{
		log_msg("<no disasm>\n");
	}

#else // COMPILE_DISASM

	void	log_disasm(const unsigned char* instruction_data)
	// Disassemble one instruction to the log.
	{
		enum arg_format {
			ARG_NONE = 0,
			ARG_STR,
			ARG_HEX,	// default hex dump, in case the format is unknown or unsupported
			ARG_U8,
			ARG_U16,
			ARG_S16,
			ARG_PUSH_DATA,
			ARG_DECL_DICT,
			ARG_FUNCTION2
		};
		struct inst_info
		{
			int	m_action_id;
			const char*	m_instruction;

			arg_format	m_arg_format;
		};

		static inst_info	s_instruction_table[] = {
			{ 0x04, "next_frame", ARG_NONE },
			{ 0x05, "prev_frame", ARG_NONE },
			{ 0x06, "play", ARG_NONE },
			{ 0x07, "stop", ARG_NONE },
			{ 0x08, "toggle_qlty", ARG_NONE },
			{ 0x09, "stop_sounds", ARG_NONE },
			{ 0x0A, "add", ARG_NONE },
			{ 0x0B, "sub", ARG_NONE },
			{ 0x0C, "mul", ARG_NONE },
			{ 0x0D, "div", ARG_NONE },
			{ 0x0E, "equ", ARG_NONE },
			{ 0x0F, "lt", ARG_NONE },
			{ 0x10, "and", ARG_NONE },
			{ 0x11, "or", ARG_NONE },
			{ 0x12, "not", ARG_NONE },
			{ 0x13, "str_eq", ARG_NONE },
			{ 0x14, "str_len", ARG_NONE },
			{ 0x15, "substr", ARG_NONE },
			{ 0x17, "pop", ARG_NONE },
			{ 0x18, "floor", ARG_NONE },
			{ 0x1C, "get_var", ARG_NONE },
			{ 0x1D, "set_var", ARG_NONE },
			{ 0x20, "set_target_exp", ARG_NONE },
			{ 0x21, "str_cat", ARG_NONE },
			{ 0x22, "get_prop", ARG_NONE },
			{ 0x23, "set_prop", ARG_NONE },
			{ 0x24, "dup_sprite", ARG_NONE },
			{ 0x25, "rem_sprite", ARG_NONE },
			{ 0x26, "trace", ARG_NONE },
			{ 0x27, "start_drag", ARG_NONE },
			{ 0x28, "stop_drag", ARG_NONE },
			{ 0x29, "str_lt", ARG_NONE },
			{ 0x2B, "cast_object", ARG_NONE },
			{ 0x30, "random", ARG_NONE },
			{ 0x31, "mb_length", ARG_NONE },
			{ 0x32, "ord", ARG_NONE },
			{ 0x33, "chr", ARG_NONE },
			{ 0x34, "get_timer", ARG_NONE },
			{ 0x35, "substr_mb", ARG_NONE },
			{ 0x36, "ord_mb", ARG_NONE },
			{ 0x37, "chr_mb", ARG_NONE },
			{ 0x3A, "delete", ARG_NONE },
			{ 0x3B, "delete_all", ARG_NONE },
			{ 0x3C, "set_local", ARG_NONE },
			{ 0x3D, "call_func", ARG_NONE },
			{ 0x3E, "return", ARG_NONE },
			{ 0x3F, "mod", ARG_NONE },
			{ 0x40, "new", ARG_NONE },
			{ 0x41, "decl_local", ARG_NONE },
			{ 0x42, "decl_array", ARG_NONE },
			{ 0x43, "decl_obj", ARG_NONE },
			{ 0x44, "type_of", ARG_NONE },
			{ 0x45, "get_target", ARG_NONE },
			{ 0x46, "enumerate", ARG_NONE },
			{ 0x47, "add_t", ARG_NONE },
			{ 0x48, "lt_t", ARG_NONE },
			{ 0x49, "eq_t", ARG_NONE },
			{ 0x4A, "number", ARG_NONE },
			{ 0x4B, "string", ARG_NONE },
			{ 0x4C, "dup", ARG_NONE },
			{ 0x4D, "swap", ARG_NONE },
			{ 0x4E, "get_member", ARG_NONE },
			{ 0x4F, "set_member", ARG_NONE },
			{ 0x50, "inc", ARG_NONE },
			{ 0x51, "dec", ARG_NONE },
			{ 0x52, "call_method", ARG_NONE },
			{ 0x53, "new_method", ARG_NONE },
			{ 0x54, "is_inst_of", ARG_NONE },
			{ 0x55, "enum_object", ARG_NONE },
			{ 0x60, "bit_and", ARG_NONE },
			{ 0x61, "bit_or", ARG_NONE },
			{ 0x62, "bit_xor", ARG_NONE },
			{ 0x63, "shl", ARG_NONE },
			{ 0x64, "asr", ARG_NONE },
			{ 0x65, "lsr", ARG_NONE },
			{ 0x66, "eq_strict", ARG_NONE },
			{ 0x67, "gt_t", ARG_NONE },
			{ 0x68, "gt_str", ARG_NONE },
			{ 0x69, "extends", ARG_NONE },
			
			{ 0x81, "goto_frame", ARG_U16 },
			{ 0x83, "get_url", ARG_STR },
			{ 0x87, "store_register", ARG_U8 },
			{ 0x88, "decl_dict", ARG_DECL_DICT },
			{ 0x8A, "wait_for_frame", ARG_HEX },
			{ 0x8B, "set_target", ARG_STR },
			{ 0x8C, "goto_frame_lbl", ARG_STR },
			{ 0x8D, "wait_for_fr_exp", ARG_HEX },
			{ 0x8E, "function2", ARG_FUNCTION2 },
			{ 0x94, "with", ARG_U16 },
			{ 0x96, "push_data", ARG_PUSH_DATA },
			{ 0x99, "goto", ARG_S16 },
			{ 0x9A, "get_url2", ARG_HEX },
			// { 0x8E, "function2", ARG_HEX },
			{ 0x9B, "func", ARG_HEX },
			{ 0x9D, "branch_if_true", ARG_S16 },
			{ 0x9E, "call_frame", ARG_HEX },
			{ 0x9F, "goto_frame_exp", ARG_HEX },
			{ 0x00, "<end>", ARG_NONE }
		};

		int	action_id = instruction_data[0];
		inst_info*	info = NULL;

		for (int i = 0; ; i++)
		{
			if (s_instruction_table[i].m_action_id == action_id)
			{
				info = &s_instruction_table[i];
			}

			if (s_instruction_table[i].m_action_id == 0)
			{
				// Stop at the end of the table and give up.
				break;
			}
		}

		arg_format	fmt = ARG_HEX;

		// Show instruction.
		if (info == NULL)
		{
			log_msg("<unknown>[0x%02X]", action_id);
		}
		else
		{
			log_msg("%-15s", info->m_instruction);
			fmt = info->m_arg_format;
		}

		// Show instruction argument(s).
		if (action_id & 0x80)
		{
			assert(fmt != ARG_NONE);

			int	length = instruction_data[1] | (instruction_data[2] << 8);

			// log_msg(" [%d]", length);

			if (fmt == ARG_HEX)
			{
				for (int i = 0; i < length; i++)
				{
					log_msg(" 0x%02X", instruction_data[3 + i]);
				}
				log_msg("\n");
			}
			else if (fmt == ARG_STR)
			{
				log_msg(" \"");
				for (int i = 0; i < length; i++)
				{
					log_msg("%c", instruction_data[3 + i]);
				}
				log_msg("\"\n");
			}
			else if (fmt == ARG_U8)
			{
				int	val = instruction_data[3];
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_U16)
			{
				int	val = instruction_data[3] | (instruction_data[4] << 8);
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_S16)
			{
				int	val = instruction_data[3] | (instruction_data[4] << 8);
				if (val & 0x8000) val |= ~0x7FFF;	// sign-extend
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_PUSH_DATA)
			{
				log_msg("\n");
				int i = 0;

				while (i < length)
				{
					int	type = instruction_data[3 + i];
					i++;
					log_msg("\t\t");	// indent
					if (type == 0)
					{
						// string
						log_msg("\"");
						while (instruction_data[3 + i])
						{
							log_msg("%c", instruction_data[3 + i]);
							i++;
						}
						i++;
						log_msg("\"\n");
					}
					else if (type == 1)
					{
						// float (little-endian)
						union {
							float	f;
							Uint32	i;
						} u;
						compiler_assert(sizeof(u) == sizeof(u.i));

						memcpy(&u.i, instruction_data + 3 + i, 4);
						u.i = swap_le32(u.i);
						i += 4;

						log_msg("(float) %f\n", u.f);
					}
					else if (type == 2)
					{
						log_msg("NULL\n");
					}
					else if (type == 3)
					{
						log_msg("undef\n");
					}
					else if (type == 4)
					{
						// contents of register
						int	reg = instruction_data[3 + i];
						i++;
						log_msg("reg[%d]\n", reg);
					}
					else if (type == 5)
					{
						int	bool_val = instruction_data[3 + i];
						i++;
						log_msg("bool(%d)\n", bool_val);
					}
					else if (type == 6)
					{
						// double
						// wacky format: 45670123
						union {
							double	d;
							Uint64	i;
							struct {
								Uint32	lo;
								Uint32	hi;
							} sub;
						} u;
						compiler_assert(sizeof(u) == sizeof(u.i));

						memcpy(&u.sub.hi, instruction_data + 3 + i, 4);
						memcpy(&u.sub.lo, instruction_data + 3 + i + 4, 4);
						u.i = swap_le64(u.i);
						i += 8;


						log_msg("(double) %f\n", u.d);
					}
					else if (type == 7)
					{
						// int32
						Sint32	val = instruction_data[3 + i]
							| (instruction_data[3 + i + 1] << 8)
							| (instruction_data[3 + i + 2] << 16)
							| (instruction_data[3 + i + 3] << 24);
						i += 4;
						log_msg("(int) %d\n", val);
					}
					else if (type == 8)
					{
						int	id = instruction_data[3 + i];
						i++;
						log_msg("dict_lookup[%d]\n", id);
					}
					else if (type == 9)
					{
						int	id = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
						i += 2;
						log_msg("dict_lookup_lg[%d]\n", id);
					}
				}
			}
			else if (fmt == ARG_DECL_DICT)
			{
				int	i = 0;
				int	count = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				log_msg(" [%d]\n", count);

				// Print strings.
				for (int ct = 0; ct < count; ct++)
				{
					log_msg("\t\t");	// indent

					log_msg("\"");
					while (instruction_data[3 + i])
					{
						// safety check.
						if (i >= length)
						{
							log_msg("<disasm error -- length exceeded>\n");
							break;
						}

						log_msg("%c", instruction_data[3 + i]);
						i++;
					}
					log_msg("\"\n");
					i++;
				}
			}
			else if (fmt == ARG_FUNCTION2)
			{
				// Signature info for a function2 opcode.
				int	i = 0;
				const char*	function_name = (const char*) &instruction_data[3 + i];
				i += strlen(function_name) + 1;

				int	arg_count = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				int	reg_count = instruction_data[3 + i];
				i++;

				log_msg("\n\t\tname = '%s', arg_count = %d, reg_count = %d\n",
					function_name, arg_count, reg_count);

				uint16	flags = (instruction_data[3 + i]) | (instruction_data[3 + i + 1] << 8);
				i += 2;

				// @@ What is the difference between "super" and "_parent"?

				bool	preload_global = (flags & 0x100) != 0;
				bool	preload_parent = (flags & 0x80) != 0;
				bool	preload_root   = (flags & 0x40) != 0;

				bool	suppress_super = (flags & 0x20) != 0;
				bool	preload_super  = (flags & 0x10) != 0;
				bool	suppress_args  = (flags & 0x08) != 0;
				bool	preload_args   = (flags & 0x04) != 0;
				bool	suppress_this  = (flags & 0x02) != 0;
				bool	preload_this   = (flags & 0x01) != 0;

				log_msg("\t\t        pg = %d\n"
					"\t\t        pp = %d\n"
					"\t\t        pr = %d\n"
					"\t\tss = %d, ps = %d\n"
					"\t\tsa = %d, pa = %d\n"
					"\t\tst = %d, pt = %d\n",
					int(preload_global),
					int(preload_parent),
					int(preload_root),
					int(suppress_super),
					int(preload_super),
					int(suppress_args),
					int(preload_args),
					int(suppress_this),
					int(preload_this));

				for (int argi = 0; argi < arg_count; argi++)
				{
					int	arg_register = instruction_data[3 + i];
					i++;
					const char*	arg_name = (const char*) &instruction_data[3 + i];
					i += strlen(arg_name) + 1;

					log_msg("\t\targ[%d] - reg[%d] - '%s'\n", argi, arg_register, arg_name);
				}

				int	function_length = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				log_msg("\t\tfunction length = %d\n", function_length);
			}
		}
		else
		{
			log_msg("\n");
		}
	}

#endif // COMPILE_DISASM


	// garbage collector

	void heap::clear()
	{
		for (hash<smart_ptr<as_object_interface>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object_interface* obj = it->first.get_ptr();
			if (obj)
			{
				if (obj->get_ref_count() > 1)
				{
					hash<as_object_interface*, bool> visited_objects;
					obj->clear_refs(&visited_objects, obj);
				}
			}
		}
		m_heap.clear();
	}

	bool heap::is_garbage(as_object_interface* obj)
	{
		bool is_garbage = false;
		m_heap.get(obj, &is_garbage);
		return is_garbage;
	}

	void heap::set(as_object_interface* obj, bool is_garbage)
	{
		m_heap.set(obj, is_garbage);
	}

	void heap::set_as_garbage()
	{
		for (hash<smart_ptr<as_object_interface>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object_interface* obj = it->first.get_ptr();
			if (obj)
			{
				m_heap.set(obj, true);
			}
		}
	}

	void heap::clear_garbage()
	{
		s_global->not_garbage();
		for (hash<smart_ptr<as_object_interface>, bool>::iterator it = m_heap.begin();
			it != m_heap.end(); ++it)
		{
			as_object_interface* obj = it->first.get_ptr();
			if (obj)
			{
				if (it->second)	// is garbage ?
				{
					if (obj->get_ref_count() > 1)	// is in heap only ?
					{
						hash<as_object_interface*, bool> visited_objects;
						obj->clear_refs(&visited_objects, obj);
					}
					m_heap.erase(obj);
				}
			}
		}
	}


};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
