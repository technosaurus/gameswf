// gameswf_avm2.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AVM2 implementation

#include "gameswf/gameswf_avm2.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_abc.h"
#include "gameswf/gameswf_disasm.h"
#include "gameswf/gameswf_character.h"

namespace gameswf
{

	as_3_function::as_3_function(abc_def* abc, int method, player* player) :
		as_function(player),
		m_method(method),
		m_abc(abc)
	{
		m_this_ptr = this;

		// any function MUST have prototype
		builtin_member("prototype", new as_object(player));
	}

	as_3_function::~as_3_function()
	{
	}

	void	as_3_function::operator()(const fn_call& fn)
	// dispatch
	{
		assert(fn.env);

		// try to use caller environment
		// if the caller object has own environment then we use its environment
		as_environment* env = fn.env;
		if (fn.this_ptr)
		{
			if (fn.this_ptr->get_environment())
			{
				env = fn.this_ptr->get_environment();
			}
		}

		// set 'this'
		as_object* this_ptr = env->get_target();
		if (fn.this_ptr)
		{
			this_ptr = fn.this_ptr;
			if (this_ptr->m_this_ptr != NULL)
			{
				this_ptr = this_ptr->m_this_ptr.get_ptr();
			}
		}

		// Create local registers.
		array<as_value>	local_register;
		local_register.resize(m_local_count + 1);

		//	Register 0 holds the “this” object. This value is never null.
		assert(this_ptr);
		local_register[0] = this_ptr;

		// Create stack.
		array<as_value>	stack;

		// Create scope stack.
		array<as_value>	scope;

		// Execute the actions.
		execute(local_register, stack, scope, fn.result);

	}

	void	as_3_function::execute(array<as_value>& lregister,
		array<as_value>& stack,
		array<as_value>& scope,
		as_value* result)
	{
		int ip = 0;
		do
		{
			Uint8 opcode = m_code[ip];
			ip++;

			switch (opcode)
			{
				case 0x24:	// pushbyte
				{
					int byte_value;
					ip += read_vu30(byte_value, &m_code[ip]);
					stack.push_back(byte_value);
					break;
				}

				case 0x30:	// pushscope
					scope.push_back(stack.back());
					stack.resize(stack.size() - 1); 
					break;

				case 0x49:	// constructsuper
				{
					// stack: object, arg1, arg2, ..., argn
					int arg_count;
					ip += read_vu30(arg_count, &m_code[ip]);

					as_object* obj = stack.back().to_object();
					//TODO, construct super of obj

					stack.resize(stack.size() - 1 - arg_count); 
					break;
				}

				case 0x5D:	// findpropstrict
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					//TODO

					break;
				}

				case 0xD0:	// getlocal_0
				case 0xD1:	// getlocal_1
				case 0xD2:	// getlocal_2
				case 0xD3:	// getlocal_3
					stack.push_back(lregister[opcode & 0x03]);
					break;

				default:
					log_msg("TODO opcode 0x%02X\n", opcode);
					break;
			}

		}
		while (ip < m_code.size());

	}

	void as_3_function::read(stream* in)
	// read method_info
	{
		int param_count = in->read_vu30();
		
		// The return_type field is an index into the multiname
		m_return_type = in->read_vu30();


		m_param_type.resize(param_count);
		for (int i = 0; i < param_count; i++)
		{
			m_param_type[i] = in->read_vu30();
		}

		m_name = in->read_vu30();
		m_flags = in->read_u8();

		if (m_flags & HAS_OPTIONAL)
		{
			int option_count = in->read_vu30();
			m_options.resize(option_count);

			for (int o = 0; o < option_count; ++o)
			{
				m_options[o].m_value = in->read_vu30();
				m_options[o].m_kind = in->read_u8();
			}
		}

		if (m_flags & HAS_PARAM_NAMES)
		{
			assert(0 && "todo");
//		param_info param_names
		}

		IF_VERBOSE_PARSE(log_msg("method_info: name='%s', type='%s', params=%d\n",
			m_abc->get_string(m_name), m_abc->get_multiname(m_return_type), m_param_type.size()));

	}

	void as_3_function::read_body(stream* in)
	// read body_info
	{
		assert(m_method == in->read_vu30());

		IF_VERBOSE_PARSE(log_msg("body_info[%d]\n", m_method));

		m_max_stack = in->read_vu30();
		m_local_count = in->read_vu30();
		m_init_scope_depth = in->read_vu30();
		m_max_scope_depth = in->read_vu30();

		int i, n;
		n = in->read_vu30();	// code_length
		m_code.resize(n);
		for (i = 0; i < n; i++)
		{
			m_code[i] = in->read_u8();
		}

		n = in->read_vu30();	// exception_count
		m_exception.resize(n);
		for (i = 0; i < n; i++)
		{
			except_info* e = new except_info();
			e->read(in, m_abc.get_ptr());
			m_exception[i] = e;
		}

		n = in->read_vu30();	// trait_count
		m_trait.resize(n);
		for (int i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, m_abc.get_ptr());
			m_trait[i] = trait;
		}

		IF_VERBOSE_PARSE(log_msg("method	%i\n", m_method));
		IF_VERBOSE_PARSE(log_disasm_avm2(m_code, m_abc.get_ptr()));

	}

}
