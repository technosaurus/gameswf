// gameswf_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#include "gameswf/gameswf_object.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_function.h"
#include "gameswf/gameswf_log.h"

namespace gameswf
{

	void	as_object_addproperty(const fn_call& fn)
	{
		if (fn.nargs == 3)
		{
			assert(fn.this_ptr);
			as_object* obj = fn.this_ptr->cast_to_as_object();
			assert(obj);
			as_as_function* getter = fn.arg(1).to_as_function();
			as_as_function* setter = fn.arg(2).to_as_function();
			if (getter || setter)
			{
				obj->set_member(fn.arg(0).to_string(), as_value(obj, getter, setter));
				fn.result->set_bool(true);
				return;
			}
		}
		fn.result->set_bool(false);
	}

	as_object::as_object()
	{
		set_member("addProperty", as_object_addproperty);
		set_member_flags("addProperty", as_prop_flags::DONT_ENUM);
	}

	as_object::~as_object()
	{
		hash<as_object_interface*, int> trace;
		clear_ref(trace, this);
	}

	bool	as_object::set_member(const tu_stringi& name, const as_value& val )
	{
		//printf("SET MEMBER: %s at %p for object %p\n", name.c_str(), val.to_object(), this);
		as_value v;
		if (get_member(name, &v))
		{
			if (v.get_type() == as_value::PROPERTY)
			{
				v.set_property(val);
				return true;
			}
		}

		stringi_hash<as_member>::const_iterator it = this->m_members.find(name);
		if (it != this->m_members.end())
		{
			const as_prop_flags flags = (it.get_value()).get_member_flags();

			// is the member read-only ?
			if (!flags.get_read_only())
			{
				m_members.set(name, as_member(val, flags));
			}
		}
		else
		{
			m_members.set(name, as_member(val));
		}
		return true;
	}

	as_object_interface* as_object::get_proto()
	{
		as_member m;
		if (m_members.get("__proto__", &m))
		{
			return m.get_member_value().to_object();
		}
		return NULL;
	}

	bool	as_object::get_member(const tu_stringi& name, as_value* val)
	{
		//printf("GET MEMBER: %s at %p for object %p\n", name.c_str(), val, this);
		as_member m;
		if (m_members.get(name, &m) == false)
		{
			as_object_interface* proto = get_proto();
			if (proto)
			{
				if (proto->get_member(name, val))
				{
					if (val->get_type() == as_value::PROPERTY)
					{
						// changes the target
						val->set_as_property(this, val->m_getter, val->m_setter);
					}
					return true;
				}
			}
			return false;
		}

		*val = m.get_member_value();
		return true;
	}

	bool as_object::get_member(const tu_stringi& name, as_member* member) const
	{
		//printf("GET MEMBER: %s at %p for object %p\n", name.c_str(), val, this);
		assert(member != NULL);
		return m_members.get(name, member);
	}

	bool	as_object::set_member_flags(const tu_stringi& name, const int flags)
	{
		as_member member;
		if (this->get_member(name, &member)) {
			as_prop_flags f = member.get_member_flags();
			f.set_flags(flags);
			member.set_member_flags(f);

			m_members.set(name, member);

			return true;
		}
		return false;
	}

	character*	as_object::cast_to_character()
	// This object is not a movie; no conversion.
	{
		return NULL;
	}

	void	as_object::clear_ref(hash<as_object_interface*, int>& trace, as_object_interface* this_ptr)
	{
		// We were here ?
		int unused;
		if (trace.get(this, &unused))
		{
			return;
		}
		trace.add(this, 0);

		as_value undef;
		for (stringi_hash<as_member>::iterator it = m_members.begin();
			it != m_members.end(); ++it)
		{
			as_object_interface* obj = it->second.get_member_value().to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					it->second.set_member_value(undef);
				}
				else
				{
					obj->clear_ref(trace, this_ptr);
				}
			}
		}
		m_members.clear();
	}

	bool	as_object::on_event(const event_id& id)
	{
		// Check for member function.
		bool called = false;
		{
			const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
			if (method_name.length() > 0)
			{
				as_value	method;
				if (get_member(method_name, &method))
				{
					as_environment* env = method.to_as_function()->m_env;
					assert(env);

					call_method(method, env, this, 0, env->get_top_index());
					called = true;
				}
			}
		}

		return called;
	}

	void as_object::enumerate(as_environment* env)
	// retrieves members & pushes them into env
	{
		stringi_hash<as_member>::const_iterator it = m_members.begin();
		while (it != m_members.end())
		{
			const as_member member = (it.get_value());

			if (! member.get_member_flags().get_dont_enum())
			{
				env->push(as_value(it.get_key()));

				IF_VERBOSE_ACTION(log_msg("-------------- enumerate - push: %s\n",
					it.get_key().c_str()));
			}

			++it;
		}

		as_object_interface* proto = get_proto();
		if (proto)
		{
			proto->enumerate(env);
		}
	}

	void as_object::dump()
	// for debugging
	// retrieves members & print them
	{
		printf("\n*** object 0x%X ***\n", (Uint32) this);
		for (stringi_hash<as_member>::const_iterator it = m_members.begin(); 
			it != m_members.end(); ++it)
		{
			printf("%s: %s\n", it->first.c_str(), it->second.get_member_value().to_string());
		}
		printf("***\n");
	}

	//
	// plugin object
	//

	as_plugin::as_plugin(tu_loadlib* ll, const array<plugin_value>& params) :
		m_plugin(NULL)
	{
		assert(ll);

		// get module interface
		gameswf_module_init module_init = (gameswf_module_init) ll->get_function("gameswf_module_init");

		// create plugin instance
		if (module_init)
		{
			m_plugin = (module_init)();
		}
	}

	as_plugin::~as_plugin()
	{
		if (m_plugin)
		{
			delete m_plugin;
		}
	}

	bool	as_plugin::get_member(const tu_stringi& name, as_value* val)
	{
		if (m_plugin)
		{
			plugin_value pval;
			m_plugin->get_member(name, &pval);
			*val = pval;
			return true;
		}
		return false;
	}

	bool	as_plugin::set_member(const tu_stringi& name, const as_value& val)
	{
		if (m_plugin)
		{
			plugin_value pval = val.to_plugin_value();
			return m_plugin->set_member(name, pval);
		}
		return false;
	}

}
