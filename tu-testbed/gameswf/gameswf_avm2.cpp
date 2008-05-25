// gameswf_avm2.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AVM2 implementation

#include "gameswf/gameswf_avm2.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_abc.h"

namespace gameswf
{

	as_avm2_function::as_avm2_function(player* player) :
		as_function(player)
	{
		m_this_ptr = this;

		// any function MUST have prototype
		builtin_member("prototype", new as_object(player));
	}

	as_avm2_function::~as_avm2_function()
	{
	}

	void	as_avm2_function::operator()(const fn_call& fn)
	{
		//TODO

		// find body
		// execute it
	}

	//	method_info
	//	{
	//		u30 param_count
	//		u30 return_type
	//		u30 param_type[param_count]
	//		u30 name
	//		u8 flags
	//		option_info options
	//		param_info param_names
	//	}
	void as_avm2_function::read(stream* in, abc_def* abc)
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
			abc->get_string(m_name), abc->get_multiname(m_return_type), m_param_type.size()));

	}

}
