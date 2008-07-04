// as_class.h	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script 3 Class object

#ifndef GAMESWF_AS_CLASS_H
#define GAMESWF_AS_CLASS_H

#include "gameswf/gameswf_object.h"

namespace gameswf
{
	class as_class : public as_object
	{
	public:
		as_class( player * player, const vm_stack & scope ) : 
			as_object( player ),
			m_scope( scope )
		{
		}

	private:

		vm_stack m_scope;
		
	};

}


#endif
