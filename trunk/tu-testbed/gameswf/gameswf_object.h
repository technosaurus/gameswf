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
#include "base/tu_loadlib.h"

namespace gameswf
{
	exported_module void	as_object_addproperty(const fn_call& fn);

	struct as_object : public as_object_interface
	{
		stringi_hash<as_member>	m_members;

		// It is used to register an event handler to be invoked when
		// a specified property of object changes.
		// TODO: create container based on stringi_hash<as_value>
		// watch should be coomon
		struct as_watch
		{
			as_watch() :	m_func(NULL)
			{
			}

			as_as_function* m_func;
			as_value m_user_data;
		};

		stringi_hash<as_watch>	m_watch;
		weak_ptr<resource> m_this_ptr;

		// We can place reference to __proto__ into members but it used very often
		// so for optimization we place it into instance
		smart_ptr<as_object_interface> m_proto;	// for optimization

		exported_module as_object();
		exported_module virtual ~as_object();
		
		exported_module virtual const char*	get_text_value() const { return NULL; }
		exported_module virtual bool	set_member(const tu_stringi& name, const as_value& val);
		exported_module virtual bool	get_member(const tu_stringi& name, as_value* val);
		exported_module virtual bool get_member(const tu_stringi& name, as_member* member) const;
		exported_module virtual bool	set_member_flags(const tu_stringi& name, const int flags);
		exported_module virtual bool	on_event(const event_id& id);
		exported_module virtual as_object* cast_to_as_object() { return this; }
		exported_module virtual	void enumerate(as_environment* env);
		exported_module virtual as_object_interface* get_proto() const;
		exported_module virtual bool watch(const tu_string& name, as_as_function* callback, const as_value& user_data);
		exported_module virtual bool unwatch(const tu_string& name);
		exported_module virtual void clear_refs(hash<as_object_interface*, bool>* visited_objects,
			as_object_interface* this_ptr);
		exported_module virtual void not_garbage();
		exported_module virtual void copy_to(as_object_interface* target);
		exported_module void dump();
		exported_module as_object_interface* find_target(const tu_string& path);
	};

	struct as_plugin : public as_object
	{
		tu_loadlib* m_lib;

		as_plugin() :
			m_lib(NULL)
		{
		}

		~as_plugin() 
		{
			delete m_lib;
		}

	};

}

#endif