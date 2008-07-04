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
#include "gameswf_jit.h"
#include "gameswf/gameswf_as_classes/as_array.h"
#include "gameswf/gameswf_as_classes/as_class.h"

namespace gameswf
{

	as_3_function::as_3_function(abc_def* abc, int method, player* player) :
		as_function(player),
		m_method(method),
		m_abc(abc),
		m_return_type( -1 ),
		m_name( -1 ),
		m_flags( 0 ),
		m_max_stack( 0 ),
		m_local_count( 0 ),
		m_init_scope_depth( 0 ),
		m_max_scope_depth( 0 )
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

		// Register 0 holds the ?this? object. This value is never null.
		assert(this_ptr);
		local_register[0] = this_ptr;

		// Registers 1 through method_info.param_count holds parameter values.
		// If fewer than method_body_info.local_count values are supplied to the call then
		// the remaining values are either the values provided by default value declarations 
		// or the value undefined.
		for (int i = 0; i < m_param_type.size(); i++)
		{
			// A zero value denotes the any (?*?) type.
			//const char* name = m_abc->get_multiname(m_param_type[i]);
			local_register[i + 1] = fn.arg( i );	// hack
		}

		// Create stack.
		vm_stack	stack;

		// Create scope stack.
		vm_stack	scope;

		// push '_global' into scope
		scope.push(get_global());

		// get flash package
		as_value val;
		get_global()->get_member("flash", &val);
		as_object* flash = val.to_object();
		assert(flash);

		// push 'Events'  into scope
		flash->get_member("Events", &val);
		scope.push(val);

#ifdef __GAMESWF_ENABLE_JIT__

		if( !m_compiled_code.is_valid() )
		{
			compile();
			m_compiled_code.initialize();
		}

#endif

		if( m_compiled_code.is_valid() )
		{
			try
			{
				m_compiled_code.call< array<as_value>&, vm_stack&, vm_stack&, as_value* >
					(local_register, stack, scope, fn.result );
			}
			catch( ... )
			{
				log_msg( "jitted code crashed" );
			}
		}
		else
		{
			// Execute the actions.
			IF_VERBOSE_ACTION(log_msg("\nEX: call method #%d\n", m_method));
			execute(local_register, stack, scope, fn.result);
			IF_VERBOSE_ACTION(log_msg("EX: ended. stack_size=%d\n", stack.size()));
		}

	}

	// interperate action script bytecode
	void	as_3_function::execute(array<as_value>& lregister,
		vm_stack& stack, vm_stack& scope, as_value* result)
	{
		// m_abc may be destroyed
		assert(m_abc != NULL);

		// some method have no body
		if (m_code.size() == 0)
		{
			return;
		}

		int ip = 0;
		do
		{
			Uint8 opcode = m_code[ip++];
			switch (opcode)
			{
				case 0x14: // ifne
				{
					bool taken;
					//Follows ECMA-262 11.9.3
					if( taken = !as_value::abstract_equality_comparison(scope[ scope.size() - 2 ], scope[ scope.size() - 1 ]) )
					{
						int offset = m_code[ip] | m_code[ip+1]<<8 | m_code[ip+2]<<16;
						ip += offset;
					}

					ip += 3;

					IF_VERBOSE_ACTION(log_msg("EX: ifne\t %s\n", taken? "taken": "not taken"));

				} break;

				case 0x1D: // popscope
				{
					scope.pop();

					IF_VERBOSE_ACTION(log_msg("EX: popscope\n"));
					break;
				}

				case 0x24:	// pushbyte
				{
					int byte_value;
					ip += read_vu30(byte_value, &m_code[ip]);
					stack.push(byte_value);

					IF_VERBOSE_ACTION(log_msg("EX: pushbyte\t %d\n", byte_value));

					break;
				}

				case 0x26:  // pushtrue
				{
					stack.push( true );

					IF_VERBOSE_ACTION(log_msg("EX: pushtrue\n"));
				}
				break;

				case 0x27:  // pushfalse
				{
					stack.push( false );

					IF_VERBOSE_ACTION(log_msg("EX: pushfalse\n"));
				}
				break;

				case 0x2D:	// pushint
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					int val = m_abc->get_integer(index);
					stack.push(val);

					IF_VERBOSE_ACTION(log_msg("EX: pushint\t %d\n", val));

					break;
				}

				case 0x2C:	// pushstring
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* val = m_abc->get_string(index);
					stack.push(val);

					IF_VERBOSE_ACTION(log_msg("EX: pushstring\t '%s'\n", val));

					break;
				}

				case 0x2F:	// pushdouble
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					double val = m_abc->get_double(index);
					stack.push(val);

					IF_VERBOSE_ACTION(log_msg("EX: pushdouble\t %f\n", val));

					break;
				}

				case 0x30:	// pushscope
				{
					as_value val = stack.pop();
					scope.push(val);

					IF_VERBOSE_ACTION(log_msg("EX: pushscope\t %s\n", val.to_xstring()));

					break;
				}

				case 0x47:	// returnvoid
				{
					IF_VERBOSE_ACTION(log_msg("EX: returnvoid\t\n"));
					result->set_undefined();
					break;
				}

				case 0x49:	// constructsuper
				{
					// stack: object, arg1, arg2, ..., argn
					int arg_count;
					ip += read_vu30(arg_count, &m_code[ip]);

					as_environment env(get_player());
					for (int i = 0; i < arg_count; i++)
					{
						env.push(stack.pop());
					}

					smart_ptr<as_object> obj = stack.top(0).to_object();

					// Assume we are in a constructor
					tu_string class_name = m_abc->get_class_from_constructor( m_method );
					tu_string super_class_name = m_abc->get_super_class( class_name );


					as_object * super = obj.get_ptr();

					while( super->get_proto() )
					{
						super = super->get_proto();
					}

					as_function * function = m_abc->get_class_constructor( super_class_name );
					if( !function )
					{
						as_value value;
						if( get_player()->get_global()->get_member( super_class_name, &value ) )
						{
							function = cast_to<as_function>( value.to_object() );
						}
					}
					assert( function );
					as_object* proto = super->create_proto( function );

					call_method( function, &env, obj.get_ptr(), arg_count, 0);

					//stack.top(0) = obj.get_ptr();

					IF_VERBOSE_ACTION(log_msg("EX: constructsuper\t 0x%p(args:%d)\n", obj.get_ptr(), arg_count));

					break;
				}

				case 0x4A: //constructprop
				// Stack ..., obj, [ns], [name], arg1,...,argn => ..., value
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);
					const char * name_space = m_abc->get_multiname_namespace(index);

					int arg_count;
					ip += read_vu30(arg_count, &m_code[ip]);

					as_environment env(get_player());
					for (int i = 0; i < arg_count; i++)
					{
						env.push(stack.top(i));
					}
					stack.drop(arg_count);

					as_object* obj = stack.pop().to_object();

					as_value func, func2;

					smart_ptr<as_object> new_object;
					if( obj && obj->get_member(name, &func))
					{
						new_object = new as_object(get_player());
						//:TODO: create prototype .... ( move instanciate class from character -> as_object )
						call_method( m_abc->get_class_constructor( name ), &env, new_object.get_ptr(), arg_count, 0 );
					}

					IF_VERBOSE_ACTION(log_msg("EX: constructprop\t 0x%p.%s(args:%d)\n", obj, name, arg_count));

					stack.push( new_object.get_ptr() );
				}
				break;

				case 0x4F:	// callpropvoid, Call a property, discarding the return value.
				// Stack: ..., obj, [ns], [name], arg1,...,argn => ...
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);

					int arg_count;
					ip += read_vu30(arg_count, &m_code[ip]);

					as_environment env(get_player());
					for (int i = 0; i < arg_count; i++)
					{
						env.push(stack.top(i));
					}
					stack.drop(arg_count);

					as_object* obj = stack.pop().to_object();

					as_value func, func2;

					if( obj &&  obj->get_member(name, &func))
					{
						if( func.is_function() )
						{
							call_method(func, &env, obj, arg_count, env.get_top_index());
						}
						else if(func.to_object()->get_member( "__call__", &func2 ) )
						{
							//todo patch scope
							call_method(func2, &env, obj, arg_count, env.get_top_index());
						}
					}

					IF_VERBOSE_ACTION(log_msg("EX: callpropvoid\t 0x%p.%s(args:%d)\n", obj, name, arg_count));

					break;
				}

				case 0x56: //newarray
				{
					int arg_count;
					
					ip += read_vu30(arg_count, &m_code[ip]);

					as_array * array = new as_array( get_player() );
					
					int offset = stack.size() - arg_count;

					for( int arg_index = 0; arg_index < arg_count; ++arg_index )
					{
						array->push( stack[ offset + arg_index ] );
					}

					stack.resize( offset + 1 );
					stack.top(0) = array;

					IF_VERBOSE_ACTION(log_msg("EX: newarray\t arg_count:%i\n", arg_count));

				} break;

				case 0x58: // newclass
				{
					// stack:	?, basetype => ?, newclass
					int class_index;
					ip += read_vu30( class_index, &m_code[ip] );

					as_object* basetype = stack.top(0).to_object();
					smart_ptr<as_object> new_obj = new as_class(get_player(), scope);

					new_obj->set_member("__prototype__", basetype);

					as_environment env( get_player() );
					call_method( m_abc->get_class_function( class_index ), &env, new_obj.get_ptr(), 0, 0 );

					stack.top(0).set_as_object(new_obj.get_ptr());
	
					break;
				}

				case 0x5D:	// findpropstrict
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);

					// search property in scope
					as_object* obj = NULL;
					for (int i = scope.size() - 1; i >= 0; i--)
					{
						as_value val;
						if (scope[i].get_member(name, &val))
						{
							obj = scope[i].to_object();
							break;
						}
					}

					//Search for a script entry to execute

					as_function * func;
					if( !obj && ( func = m_abc->get_script_function( name ) ) )
					{
						get_global()->set_member( name, new as_object( get_player() ) );

						as_environment env( get_player() );

						call_method( func, &env, get_global(), 0, 0 );

						obj = get_global();
					}
					IF_VERBOSE_ACTION(log_msg("EX: findpropstrict\t %s, obj=0x%p\n", name, obj));

					stack.push(obj);
					break;
				}

				case 0x5E:	// findproperty, Search the scope stack for a property
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);
					const char * name_space = m_abc->get_multiname_namespace(index);

					as_object* obj = NULL;
					for (int i = scope.size() - 1; i >= 0; i--)
					{
						as_value val;
						if (scope[i].get_member(name, &val))
						{
							obj = scope[i].to_object();
							break;
						}
					}

					IF_VERBOSE_ACTION(log_msg("EX: findproperty\t '%s', obj=0x%p\n", name, obj));

					stack.push(obj);
					break;

				}

				case 0x60:	// getlex, Find and get a property.
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);

					// search property in scope
					as_value val;
					for (int i = scope.size() - 1; i >= 0; i--)
					{
						if (scope[i].get_member(name, &val))
						{
							break;
						}
					}

					IF_VERBOSE_ACTION(log_msg("EX: getlex\t %s, value=%s\n", name, val.to_xstring()));

					stack.push(val);
					break;
				}

				case 0x65: // getscopeobject
				{
					int index = m_code[ip];
					++ip;

					assert( index < scope.size() );

					stack.push( scope[index] );

					IF_VERBOSE_ACTION(log_msg("EX: getscopeobject\t index=%i, value=%s\n", index, stack.top(0).to_xstring()));

				} break;

				case 0x66:	// getproperty
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);

					as_object* obj = stack.top(0).to_object();
					if (obj)
					{
						obj->get_member(name, &stack.top(0));
					}
					else
					{
						stack.top(0).set_undefined();
					}

					IF_VERBOSE_ACTION(log_msg("EX: getproperty\t %s, value=%s\n", name, stack.top(0).to_xstring()));

					break;
				}

				case 0x68:	// initproperty, Initialize a property.
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const char* name = m_abc->get_multiname(index);

					as_value& val = stack.top(0);
					as_object* obj = stack.top(1).to_object();
					if (obj)
					{
						obj->set_member(name, val);
					}

					IF_VERBOSE_ACTION(log_msg("EX: initproperty\t 0x%p.%s=%s\n", obj, name, val.to_xstring()));

					stack.drop(2);
					break;
				}

				case 0xA0:	// Add two values
				{
					if (stack.top(0).is_string() || stack.top(1).is_string())
					{
						tu_string str = stack.top(1).to_string();
						str += stack.top(0).to_string();
						stack.top(1).set_tu_string(str);
					}
					else
					{
						stack.top(1) += stack.top(0).to_number();
					}
					stack.drop(1);

					break;
				}

				case 0xA2: // multiply
				{
					stack.top(1) = stack.top(1).to_number() * stack.top(0).to_number();
					stack.drop(1);

					IF_VERBOSE_ACTION(log_msg("EX: multiply\n"));
					
					break;
				}

				case 0xD0:	// getlocal_0
				case 0xD1:	// getlocal_1
				case 0xD2:	// getlocal_2
				case 0xD3:	// getlocal_3
				{
					as_value& val = lregister[opcode & 0x03];
					stack.push(val);
					IF_VERBOSE_ACTION(log_msg("EX: getlocal_%d\t %s\n", opcode & 0x03, val.to_xstring()));
					break;
				}

				case 0xD4:	// setlocal_0
				case 0xD5:	// setlocal_1
				case 0xD6:	// setlocal_2
				case 0xD7:	// setlocal_3
				{
					lregister[opcode & 0x03] = stack.pop();

					IF_VERBOSE_ACTION(log_msg("EX: setlocal_%d\t %s\n", opcode & 0x03, lregister[opcode & 0x03].to_xstring()));
					break;
				}

				default:
					log_msg("TODO opcode 0x%02X\n", opcode);
					return;
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
