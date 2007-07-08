// gameswf_object.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#ifndef GAMESWF_OBJECT_H
#define GAMESWF_OBJECT_H

#include "gameswf/gameswf_value.h"
#include "gameswf/gameswf_environment.h"
#include "gameswf/gameswf_types.h"
#include "base/container.h"
#include "base/smart_ptr.h"

namespace gameswf
{
	void	as_object_addproperty(const fn_call& fn);

	struct as_object : public as_object_interface
	{
		stringi_hash<as_member>	m_members;
		as_object_interface*	m_prototype;

		as_object() : m_prototype(NULL)
		{
			set_member("addProperty", as_object_addproperty);
			set_member_flags("addProperty", as_prop_flags::DONT_ENUM);
		}

		as_object(as_object_interface* proto) : m_prototype(proto)
		{
			if (m_prototype)
			{
				m_prototype->add_ref();
			}
		}

		virtual ~as_object()
		{
			if (m_prototype)
			{
				m_prototype->drop_ref();
			}
		}
		
		virtual const char*	get_text_value() const { return NULL; }
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool get_member(const tu_stringi& name, as_member* member) const;
		virtual bool	set_member_flags(const tu_stringi& name, const int flags);
		virtual character*	cast_to_character();
		void	clear();
		virtual bool	on_event(const event_id& id);
		virtual as_object* cast_to_as_object() { return this; }
		virtual	void enumerate(as_environment* env);
	};
}

#endif
