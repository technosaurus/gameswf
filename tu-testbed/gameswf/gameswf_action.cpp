// gameswf_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_stream.h"

#include <stdio.h>


#ifdef _WIN32
#define snprintf _snprintf
#endif // _WIN32


namespace gameswf
{
	//
	// action stuff
	//


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

		void	execute(movie* m)
		{
			m->add_action_buffer(&m_buf);
		}

		void	execute_state(movie* m)
		{
			// left empty because actions don't have to be replayed when seeking the movie.
		}
	};

	void	do_action_loader(stream* in, int tag_type, movie* m)
	{
		IF_VERBOSE_PARSE(log_msg("tag %d: do_action_loader\n", tag_type));

		IF_VERBOSE_ACTION(log_msg("-- actions in frame %d\n", m->get_current_frame()));

		assert(in);
		assert(tag_type == 12);
		assert(m);
		
		do_action*	da = new do_action;
		da->read(in);

		m->add_execute_tag(da);
	}


	//
	// action_buffer
	//

	// Disassemble one instruction to the log.
	static void	log_disasm(const unsigned char* instruction_data);

	void	action_buffer::read(stream* in)
	{
		// Read action bytes.
		for (;;)
		{
			int	instruction_start = m_buffer.size();

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

			IF_VERBOSE_ACTION(log_msg("\t"); log_disasm(&m_buffer[instruction_start]); );

			if (action_id == 0)
			{
				// end of action buffer.
				return;
			}
		}
	}


	void	action_buffer::execute(as_environment* env)
	// Interpret the actions in this action buffer, and evaluate them
	// in the given environment.
	{
		assert(env);

		movie*	original_target = env->get_target();
		UNUSED(original_target);		// Avoid warnings.

		array<const char*>	dictionary;

		for (int pc = 0; pc < m_buffer.size(); )
		{
			int	action_id = m_buffer[pc];
			if ((action_id & 0x80) == 0)
			{
				IF_VERBOSE_ACTION(log_msg("EX:\t"); log_disasm(&m_buffer[pc]));

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
					env->get_target()->set_play_state(movie::PLAY);
					break;

				case 0x07:	// action stop
					env->get_target()->set_play_state(movie::STOP);
					break;

				case 0x08:	// toggle quality
				case 0x09:	// stop sounds
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
					env->top(1).set(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}
				case 0x0F:	// less than
				{
					env->top(1).set(env->top(1) < env->top(1));
					env->drop(1);
					break;
				}
				case 0x10:	// logical and
				{
					env->top(1).set(env->top(1).to_bool() && env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x11:	// logical or
				{
					env->top(1).set(env->top(1).to_bool() && env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x12:	// logical not
				{
					env->top(0).set(! env->top(0).to_bool());
					break;
				}
				case 0x13:	// string equal
				{
					env->top(1).set(env->top(1).to_tu_string() == env->top(0).to_tu_string());
					env->drop(1);
					break;
				}
				case 0x14:	// string length
				{
					env->top(0).set(env->top(0).to_tu_string().length());
					break;
				}
				case 0x15:	// substring
				{
					int	size = int(env->top(0).to_number());
					int	base = int(env->top(1).to_number()) - 1;
					const tu_string&	str = env->top(2).to_tu_string();

					// Keep base within range.
					base = iclamp(base, 0, str.length());

					// Truncate if necessary.
					size = imin(str.length() - base, size);

					// @@ This can be done without new allocations if we get dirtier w/ internals
					// of as_value and tu_string...
					tu_string	new_string = str.c_str() + base;
					new_string.resize(size);

					env->drop(2);

					break;
				}
				case 0x17:	// pop
				{
					env->drop(1);
					break;
				}
				case 0x18:	// int
				{
					env->top(0).set(int(floor(env->top(0).to_number())));
					break;
				}
				case 0x1C:	// get variable
				{
					as_value	varname = env->pop();
					env->push(env->get_variable(varname.to_tu_string()));
					break;
				}
				case 0x1D:	// set variable
				{
					as_value	val = env->pop();
					as_value	var_specifier = env->pop();
					env->set_variable(var_specifier.to_string(), val);
					break;
				}
				case 0x20:	// set target expression
				{
					// @@ TODO
					break;
				}
				case 0x21:	// string concat
				{
					env->top(1).string_concat(env->top(0).to_tu_string());
					env->drop(1);
					break;
				}
				case 0x22:	// get property
				case 0x23:	// set property
				case 0x24:	// duplicate clip (sprite?)
				case 0x25:	// remove clip
				case 0x26:	// trace
				case 0x27:	// start drag movie
				case 0x28:	// stop drag movie
					break;
				case 0x29:	// string less than
				{
					env->top(1).set(env->top(1).to_tu_string() < env->top(0).to_tu_string());
					break;
				}
				case 0x30:	// random
				{
					int	max = int(env->top(0).to_number());
					if (max < 1) max = 1;
					env->top(0).set(double(rand() % max));
					break;
				}
				case 0x31:	// mb length
				{
					// @@ TODO
					break;
				}
				case 0x32:	// ord
				{
					// ASCII code of first character
					env->top(0).set(env->top(0).to_string()[0]);
					break;
				}
				case 0x33:	// chr
				{
					char	buf[2];
					buf[0] = int(env->top(0).to_number());
					buf[1] = 0;
					env->top(0).set(buf);
					break;
				}

				case 0x34:	// get timer
					// Push milliseconds since we started playing.
					env->push(floorf(env->m_target->get_timer() * 1000.0f));
					break;

				case 0x35:	// mb substring
				{
					// @@ TODO
				}
				case 0x37:	// mb chr
				{
					// @@ TODO
					break;
				}
				case 0x3A:	// delete
				{
					// @@ TODO
					break;
				}
				case 0x3B:	// delete all
				{
					// @@ TODO
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
					// @@ TODO
					break;
				}
				case 0x3E:	// return
				{
					// @@ TODO
					break;
				}
				case 0x3F:	// modulo
				{
					// @@ TODO
					break;
				}
				case 0x40:	// new
				{
					// @@ TODO
					break;
				}
				case 0x41:	// declare local
				{
					// @@ TODO
					break;
				}
				case 0x42:	// declare array
				{
					// @@ TODO
					break;
				}
				case 0x43:	// declare object
				{
					// @@ TODO
					break;
				}
				case 0x44:	// type of
				{
					// @@ TODO
					break;
				}
				case 0x45:	// get target
				{
					// @@ TODO
					break;
				}
				case 0x46:	// enumerate
				{
					// @@ TODO
					break;
				}
				case 0x47:	// add (typed)
				{
					// @@ TODO
					break;
				}
				case 0x48:	// less than (typed)
				{
					// @@ TODO
					break;
				}
				case 0x49:	// equal (typed)
				{
					// @@ identical to untyped equal, as far as I can tell...
					env->top(1).set(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}

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
					env->get_target()->goto_frame(frame);
					break;
				}

				case 0x83:	// get url
				{
					// @@ TODO should call back into client app, probably...
					break;
				}

				case 0x88:	// declare dictionary
				{
					int	i = pc;
					int	count = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					i += 2;

					dictionary.resize(count);

					// Index the strings.
					for (int ct = 0; ct < count; ct++)
					{
						// Point into the current action buffer.
						dictionary[ct] = (const char*) &m_buffer[3 + i];

						while (m_buffer[3 + i])
						{
							// safety check.
							if (i >= length)
							{
								log_error("error: action buffer dict length exceeded>\n");
								break;
							}

							i++;
						}
						i++;
					}
					break;
				}

				case 0x8A:	// wait for frame
				{
					// @@ TODO I think this has to deal with incremental loading
					break;
				}

				case 0x8B:	// set target
				{
					// Change the movie we're working on.
					const char* target_name = (const char*) &m_buffer[pc + 3];
					if (target_name[0] == 0) { env->set_target(original_target); }
					else {
//						env->set_target(env->get_target()->find_labeled_target(target_name));
//						if (env->get_target() == NULL) env->set_target(original_target);
					}
					break;
				}

				case 0x8C:	// go to labeled frame
				{
					// char*	frame_label = (char*) &m_buffer[pc + 3];
					// env->get_target()->goto_labeled_frame(frame_label);
					break;
				}

				case 0x8D:	// wait for frame expression (?)
					break;

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
						}
						else if (type == 2)
						{
							// NULL
							env->push(NULL);	// @@???
						}
						else if (type == 3)
						{
							env->push(as_value(as_value::UNDEFINED));
						}
						else if (type == 4)
						{
							// contents of register
							int	reg = m_buffer[3 + i];
							i++;
//							log_msg("reg[%d]\n", reg);
							env->push(/* contents of register[reg] */ false);	// @@ TODO
						}
						else if (type == 5)
						{
							bool	bool_val = m_buffer[3 + i] ? true : false;
							i++;
//							log_msg("bool(%d)\n", bool_val);
							env->push(bool_val);
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
						}
						else if (type == 8)
						{
							int	id = m_buffer[3 + i];
							i++;
							if (id < dictionary.size())
							{
								env->push(dictionary[id]);
							}
							else
							{
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								env->push(0);
							}
						}
						else if (type == 9)
						{
							int	id = m_buffer[3 + i] | (m_buffer[4 + i] << 8);
							i += 2;
							if (id < dictionary.size())
							{
								env->push(dictionary[id]);
							}
							else
							{
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								env->push(0);
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
					break;
				case 0x9D:	// branch if true
				{
					Sint16	offset = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					
					bool	test = env->top(0).to_bool();
					env->drop(1);
					if (test)
					{
						next_pc += offset;
						// @@ TODO range checks
					}
					break;
				}
				case 0x9E:	// call frame
				case 0x9F:	// goto expression (?)
					break;
				
				}
				pc = next_pc;
			}
		}

		env->set_target(original_target);
	}


	//
	// as_value -- ActionScript value type
	//


	const char*	as_value::to_string() const
	// Conversion to string.
	{
		return to_tu_string().c_str();
	}


	const tu_string&	as_value::to_tu_string() const
	// Conversion to const tu_string&.
	{
		if (m_type == STRING) { /* don't need to do anything */ }
		else if (m_type == NUMBER)
		{
			char buffer[50];
			snprintf(buffer, 50, "%g", m_number_value);
			m_string_value = buffer;
		}
		else if (m_type == UNDEFINED)
		{
			m_string_value = "<undefined>";
		}
		else
		{
			m_string_value = "<error -- invalid type>";
			assert(0);
		}
		
		return m_string_value;
	}

	
	double	as_value::to_number() const
	// Conversion to double.
	{
		if (m_type == STRING)
		{
			m_number_value = atof(m_string_value.c_str());
		}
		else if (m_type == NUMBER)
		{
			// don't need to do anything.
		}
		else
		{
			m_number_value = 0;
		}
		
		return m_number_value;
	}


	bool	as_value::to_bool() const
	// Conversion to boolean.
	{
		if (m_type == STRING)
		{
			// @@ Hm.
			return to_number() != 0.0;
		}
		else if (m_type == NUMBER)
		{
			return m_number_value != 0.0;
		}
		else
		{
			return false;
		}
	}


	bool	as_value::operator==(const as_value& v) const
	// Return true if operands are equal.
	{
		if (m_type == STRING)
		{
			return m_string_value == v.to_tu_string();
		}
		else if (m_type == NUMBER)
		{
			return m_number_value == v.to_number();
		}
		else
		{
			return m_type == v.m_type;
		}
	}

	
	void	as_value::string_concat(const tu_string& str)
	// Sets *this to this string plus the given string.
	{
		to_tu_string();	// make sure our m_string_value is initialized
		m_type = STRING;
		m_string_value += str;
	}


	//
	// as_environment
	//


	as_value	as_environment::get_variable(const tu_string& varname) const
	// Return the value of the given var, if it's defined.
	{
		// Check locals.
		as_value	local = NULL;
		if (m_local_variables.get(varname, &local))
		{
			// Get existing.
			return local;
		}

		// @@ TODO, ask m_target, globals, etc...

		// Fallback.
		// @@ log error?
		return as_value(as_value::UNDEFINED);
	}


	void	as_environment::set_variable(const char* path, const as_value& val)
	// Given a path to variable, set its value.
	{
		// @@ Need to implement the whole path rigmarole.
		// @@ For the moment, skip that and talk directly to the current target.

		assert(m_target);

		bool	success = m_target->set_value(path, val);
		if (success == false)
		{
			IF_VERBOSE_ACTION(log_msg("set_variable(%s, \"%s\") failed\n", path, val.to_string()));
		}
	}


	void	as_environment::set_local(const tu_string& varname, const as_value& val)
	// Set/initialize the value of the local variable.
	{
		m_local_variables.set(varname, val);
	}



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
			ARG_U16,
			ARG_S16,
			ARG_PUSH_DATA,
			ARG_DECL_DICT };
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
			
			{ 0x81, "goto_frame", ARG_U16 },
			{ 0x83, "get_url", ARG_STR },
			{ 0x88, "decl_dict", ARG_DECL_DICT },
			{ 0x8A, "wait_for_frame", ARG_HEX },
			{ 0x8B, "set_target", ARG_STR },
			{ 0x8C, "goto_frame_lbl", ARG_STR },
			{ 0x8D, "wait_for_fr_exp", ARG_HEX },
			{ 0x94, "with", ARG_U16 },
			{ 0x96, "push_data", ARG_PUSH_DATA },
			{ 0x99, "goto", ARG_S16 },
			{ 0x9A, "get_url2", ARG_HEX },
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
			log_msg("<unknown>[%02X]", action_id);
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
		}
		else
		{
			log_msg("\n");
		}
	}

#endif // COMPILE_DISASM


};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
