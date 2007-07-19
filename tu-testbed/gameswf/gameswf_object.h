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
#include "gameswf/gameswf_plugin.h"
#include "base/container.h"
#include "base/smart_ptr.h"
#include "base/tu_loadlib.h"

namespace gameswf
{
	void	as_object_addproperty(const fn_call& fn);

	struct as_object : public as_object_interface
	{
		stringi_hash<as_member>	m_members;

		as_object()
		{
		}

		virtual ~as_object()
		{
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
		virtual as_object_interface* get_proto();
		void dump();
	};

	//
	// plugin object
	//

	typedef bool (*gameswf_module_init)();
	typedef bool (*gameswf_module_close)();
	typedef bool (*gameswf_module_getmember)(const tu_stringi& name, plugin_value* pval);
	typedef bool (*gameswf_module_setmember)(const tu_stringi& name, const plugin_value& pval);

	struct as_plugin : public as_object
	{
		as_plugin(tu_loadlib* ll);
		~as_plugin();

		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);

	private:

		gameswf_module_init m_module_init;
		gameswf_module_close m_module_close;
		gameswf_module_getmember m_module_getmember;
		gameswf_module_setmember m_module_setmember;
	};


}

#endif
