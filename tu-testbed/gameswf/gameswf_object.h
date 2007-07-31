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

		// It is used to register an event handler to be invoked when
		// a specified property of object changes.
		// TODO: create container based on stringi_hash<as_value>
		// watch should be coomon
		typedef struct watch_t
		{
			as_as_function* m_func;
			as_value m_user_data;
		};
		stringi_hash<watch_t>	m_watch;

		as_object();
		virtual ~as_object();
		
		virtual const char*	get_text_value() const { return NULL; }
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool get_member(const tu_stringi& name, as_member* member) const;
		virtual bool	set_member_flags(const tu_stringi& name, const int flags);
		virtual character*	cast_to_character();
		virtual void	clear_ref(hash<as_object_interface*, int>& trace, as_object_interface* this_ptr);
		virtual bool	on_event(const event_id& id);
		virtual as_object* cast_to_as_object() { return this; }
		virtual	void enumerate(as_environment* env);
		virtual as_object_interface* get_proto();

		virtual bool watch(const tu_string& name, as_as_function* callback, const as_value& user_data);
		virtual bool unwatch(const tu_string& name);

		void dump();
	};

	//
	// plugin object
	//

	typedef gameswf_plugin* (*gameswf_module_init)();

	struct as_plugin : public as_object
	{
		as_plugin(tu_loadlib* ll, const array<plugin_value>& params);
		~as_plugin();

		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual gameswf_plugin* cast_to_plugin() { return m_plugin; }

	private:

		gameswf_plugin* m_plugin;

	};


}

#endif
