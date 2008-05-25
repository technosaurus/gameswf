// gameswf_avm2.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AVM2 implementation


#ifndef GAMESWF_AVM2_H
#define GAMESWF_AVM2_H

#include "gameswf/gameswf_function.h"

namespace gameswf
{
	struct abc_def;

	struct option_detail
	{
		int m_value;
		Uint8 m_kind;
	};

	struct as_avm2_function : public as_function
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_S_FUNCTION };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_function::is(class_id);
		}


		enum flags 
		{
			NEED_ARGUMENTS = 0x01,
			NEED_ACTIVATION = 0x02,
			NEED_REST = 0x04,
			HAS_OPTIONAL = 0x08,
			SET_DXNS = 0x40,
			HAS_PARAM_NAMES = 0x80
		};

		int m_return_type;
		array<int> m_param_type;
		int m_name;
		Uint8 m_flags;
		array<option_detail> m_options;
//		param_info m_param_names;


		as_avm2_function(player* player);
		~as_avm2_function();

		// Dispatch.
		virtual void	operator()(const fn_call& fn);

		void	read(stream* in, abc_def* abc);

	};

}

#endif
