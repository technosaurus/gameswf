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
	exported_module void	as_object_registerclass( const fn_call& fn );
	exported_module void	as_object_hasownproperty(const fn_call& fn);
	exported_module void	as_object_watch(const fn_call& fn);
	exported_module void	as_object_unwatch(const fn_call& fn);
	exported_module void	as_object_dump(const fn_call& fn);

	//
	// as_prop_flags
	//
	// flags defining the level of protection of a member
	struct as_prop_flags
	{
		enum flag
		{
			DONT_ENUM = 0x01,
			DONT_DELETE = 0x02,
			READ_ONLY = 0x04
		};

		// Numeric flags
		int m_flags;
		// if true, this value is protected (internal to gameswf)
		bool m_is_protected;

		// mask for flags
		static const int as_prop_flags_mask = DONT_ENUM | DONT_DELETE | READ_ONLY;

		// Default constructor
		as_prop_flags() : m_flags(0), m_is_protected(false)
		{
		}

		// Constructor, from numerical value
		as_prop_flags(int flags) : m_flags(flags), m_is_protected(false)
		{
		}

		// accessor to m_readOnly
		bool get_read_only() const { return (((this->m_flags & READ_ONLY) != 0) ? true : false); }

		// accessor to m_dontDelete
		bool get_dont_delete() const { return (((this->m_flags & DONT_DELETE) != 0) ? true : false); }

		// accessor to m_dontEnum
		bool get_dont_enum() const { return (((this->m_flags & DONT_ENUM) != 0) ? true : false);	}

		// accesor to the numerical flags value
		int get_flags() const { return this->m_flags; }

		// accessor to m_is_protected
		bool get_is_protected() const { return this->m_is_protected; }
		// setter to m_is_protected
		void set_get_is_protected(const bool is_protected) { this->m_is_protected = is_protected; }

		// set the numerical flags value (return the new value )
		// If unlocked is false, you cannot un-protect from over-write,
		// you cannot un-protect from deletion and you cannot
		// un-hide from the for..in loop construct
		int set_flags(const int setTrue, const int set_false = 0)
		{
			if (!this->get_is_protected())
			{
				this->m_flags = this->m_flags & (~set_false);
				this->m_flags |= setTrue;
			}

			return get_flags();
		}
	};

	//
	// as_member
	//
	// member for as_object: value + flags
	struct as_member {
		// value
		as_value m_value;
		// Properties flags
		as_prop_flags m_flags;

		// Default constructor
		as_member() {}

		// Constructor
		as_member(const as_value &value,const as_prop_flags flags=as_prop_flags()) :
			m_value(value),
			m_flags(flags)
		{
		}

		// accessor to the value
		const as_value& get_member_value() const { return m_value; }
		// accessor to the properties flags
		const as_prop_flags& get_member_flags() const { return m_flags; }

		// set the value
		void set_member_value(const as_value &value)  { m_value = value; }
		// accessor to the properties flags
		void set_member_flags(const as_prop_flags &flags)  { m_flags = flags; }
	};

	struct as_object : public as_object_interface
	{
		// Unique id of a gameswf resource
		enum	{ m_class_id = AS_OBJECT };
		exported_module virtual bool is(int class_id) const
		{
			return m_class_id == class_id;
		}

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

			as_s_function* m_func;
			as_value m_user_data;
		};

		// primitive data type has no dynamic members
		stringi_hash<as_watch>*	m_watch;

		// it's used for passing new created object pointer to constructors chain
		weak_ptr<as_object> m_this_ptr;

		// We can place reference to __proto__ into members but it used very often
		// so for optimization we place it into instance
		smart_ptr<as_object> m_proto;	// for optimization

		exported_module as_object();
		exported_module virtual ~as_object();
		
		exported_module virtual const char*	to_string() { return "[object Object]"; }
		exported_module virtual double	to_number();
		exported_module virtual bool to_bool() { return true; }
		exported_module virtual const char*	typeof() { return "object"; }

		exported_module void	builtin_member(const tu_stringi& name, const as_value& val, 
			as_prop_flags flags = as_prop_flags::DONT_ENUM | as_prop_flags::READ_ONLY);
		exported_module virtual bool	set_member(const tu_stringi& name, const as_value& val);
		exported_module virtual bool	get_member(const tu_stringi& name, as_value* val);
		exported_module virtual bool get_member(const tu_stringi& name, as_member* member) const;
		exported_module virtual bool	set_member_flags(const tu_stringi& name, const int flags);
		exported_module virtual bool	on_event(const event_id& id);
		exported_module virtual	void enumerate(as_environment* env);
		exported_module virtual as_object* get_proto() const;
		exported_module virtual bool watch(const tu_string& name, as_s_function* callback, const as_value& user_data);
		exported_module virtual bool unwatch(const tu_string& name);
		exported_module virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
		exported_module virtual void not_garbage();
		exported_module virtual void copy_to(as_object* target);
		exported_module void dump();
		exported_module as_object* find_target(const tu_string& path);
		exported_module virtual root* get_root() { return get_current_root(); }
		exported_module	virtual as_environment*	get_environment() { return 0; }
		exported_module virtual void advance(float delta_time) { assert(0); }
	};

}

#endif
