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
				// creates unbinded property
				// 'obj' is a __proto__
				obj->set_member(fn.arg(0).to_string(), as_value(getter, setter));
				fn.result->set_bool(true);
				return;
			}
		}
		fn.result->set_bool(false);
	}

	// public hasOwnProperty(name:String) : Boolean
	// Indicates whether an object has a specified property defined. 
	// This method returns true if the target object has a property that
	// matches the string specified by the name parameter, and false otherwise.
	// This method does not check the object's prototype chain and returns true only 
	// if the property exists on the object itself.
	void	as_object_hasownproperty(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			as_object* obj = fn.this_ptr->cast_to_as_object();
			assert(obj);
			as_member m;
			if (obj->m_members.get(fn.arg(0).to_tu_stringi(), &m))
			{
				fn.result->set_bool(true);
				return;
			}
		}
		fn.result->set_bool(false);
	}


	// public watch(name:String, callback:Function, [userData:Object]) : Boolean
	// Registers an event handler to be invoked when a specified property of
	// an ActionScript object changes. When the property changes,
	// the event handler is invoked with myObject as the containing object. 
	void	as_object_watch(const fn_call& fn)
	{
		bool ret = false;
		if (fn.nargs >= 2)
		{
			assert(fn.this_ptr);
			as_object* obj = fn.this_ptr->cast_to_as_object();
			assert(obj);

			ret = obj->watch(fn.arg(0).to_tu_string(), fn.arg(1).to_as_function(), 
				fn.nargs > 2 ? fn.arg(2) : as_value());
		}
		fn.result->set_bool(ret);
	}

	//public unwatch(name:String) : Boolean
	// Removes a watchpoint that Object.watch() created.
	// This method returns a value of true if the watchpoint is successfully removed,
	// false otherwise.
	void	as_object_unwatch(const fn_call& fn)
	{
		bool ret = false;
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			as_object* obj = fn.this_ptr->cast_to_as_object();
			assert(obj);
			ret = obj->unwatch(fn.arg(0).to_tu_string());
		}
		fn.result->set_bool(ret);
	}

	as_object::as_object() :
		m_is_collector_called(false),
		m_is_clear_called(false)
	{
		set_member("addProperty", as_object_addproperty);
		set_member_flags("addProperty", as_prop_flags::DONT_ENUM);
		set_member("hasOwnProperty", as_object_hasownproperty);
		set_member_flags("hasOwnProperty", as_prop_flags::DONT_ENUM);
		set_member("watch", as_object_watch);
		set_member_flags("watch", as_prop_flags::DONT_ENUM);
		set_member("unwatch", as_object_unwatch);
		set_member_flags("unwatch", as_prop_flags::DONT_ENUM);
	}

	as_object::~as_object()
	{
	}

	bool as_object::add_property(const tu_string& name, const as_value& val)
	{
		as_object_interface* proto = get_proto();
		if (proto && val.get_type() == as_value::PROPERTY)
		{
			proto->set_member(name, val);
			return true;
		}
		return false;
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
				as_value watch_val;
				watch_t watch;
				if (m_watch.get(name, &watch))
				{
					watch.m_func->m_env->push(watch.m_user_data);	// params
					watch.m_func->m_env->push(val);		// newVal
					watch.m_func->m_env->push(it->second.get_member_value());	// oldVal
					watch.m_func->m_env->push(name);	// property

					(*watch.m_func)(fn_call(&watch_val, this, watch.m_func->m_env.get_ptr(), 4,
						watch.m_func->m_env->get_top_index()));

					watch.m_func->m_env->drop(4);
				}
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
						// binds property (sets the target)
						assert(val->m_property_target == NULL);
						val->m_property_target = this;
						add_ref();
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

	void as_object::collect_garbage()
	{
		// Is it a reentrance ?
		if (m_is_collector_called)
		{
			return;
		}
		m_is_collector_called = true;

		if (get_garbage()->get(this, NULL))
		{
			get_garbage()->set(this, false);
		}

		for (stringi_hash<as_member>::const_iterator it = m_members.begin();
			it != m_members.end(); ++it)
		{
			as_object_interface* obj = it->second.get_member_value().to_object();
			if (obj)
			{
				obj->collect_garbage();
			}
		}

		m_is_collector_called = false;
	}

	void	as_object::clear_refs(as_object_interface* this_ptr)
	{
		// Is it a reentrance ?
		if (m_is_clear_called)
		{
			return;
		}
		m_is_clear_called = true;

		for (stringi_hash<as_member>::iterator it = m_members.begin();
			it != m_members.end(); ++it)
		{
			as_object_interface* obj = it->second.get_member_value().to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					it->second.set_member_value(as_value());
				}
				else
				{
					obj->clear_refs(this_ptr);
				}
			}
		}

		m_is_clear_called = false;
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
					as_environment* env = method.to_as_function()->m_env.get_ptr();
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

	bool as_object::watch(const tu_string& name, as_as_function* callback,
		const as_value& user_data)
	{
		if (callback == NULL)
		{
			return false;
		}

		watch_t watch;
		watch.m_func = callback;
		watch.m_user_data = user_data;
		m_watch.add(name, watch);
		return true;
	}

	bool as_object::unwatch(const tu_string& name)
	{
		watch_t watch;
		if (m_watch.get(name, &watch))
		{
			m_watch.erase(name);
			return true;
		}
		return false;
	}

	void as_object::dump()
	// for debugging
	// retrieves members & print them
	{
		printf("\n*** object 0x%X ***\n", this);
		for (stringi_hash<as_member>::const_iterator it = m_members.begin(); 
			it != m_members.end(); ++it)
		{
			printf("%s: %s\n", it->first.c_str(), it->second.get_member_value().to_string());
		}
		printf("***\n");
	}

}
