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

	const char*	next_slash_or_dot(const char* word);

	//TODO: the same for sprite_instance
	void	as_object_addproperty(const fn_call& fn)
	{
		if (fn.nargs == 3)
		{
			assert(fn.this_ptr);

			// creates unbinded property
			fn.this_ptr->set_member(fn.arg(0).to_string(), as_value(fn.arg(1), fn.arg(2)));
			fn.result->set_bool(true);
			return;
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
			as_object* obj = cast_to<as_object>(fn.this_ptr);
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
			as_object* obj = cast_to<as_object>(fn.this_ptr);
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
			as_object* obj = cast_to<as_object>(fn.this_ptr);
			assert(obj);
			ret = obj->unwatch(fn.arg(0).to_tu_string());
		}
		fn.result->set_bool(ret);
	}

	// for debugging
	void	as_object_dump(const fn_call& fn)
	{
		if (fn.this_ptr)
		{
			fn.this_ptr->dump();
		}
	}

	as_object::as_object()
	{
		set_member("addProperty", as_object_addproperty);
		set_member_flags("addProperty", as_prop_flags::DONT_ENUM);
		set_member("hasOwnProperty", as_object_hasownproperty);
		set_member_flags("hasOwnProperty", as_prop_flags::DONT_ENUM);
		set_member("watch", as_object_watch);
		set_member_flags("watch", as_prop_flags::DONT_ENUM);
		set_member("unwatch", as_object_unwatch);
		set_member_flags("unwatch", as_prop_flags::DONT_ENUM);

		// for debugging
		set_member("dump", as_object_dump);
		set_member_flags("dump", as_prop_flags::DONT_ENUM);
	}

	as_object::~as_object()
	{
	}

	bool	as_object::set_member(const tu_stringi& name, const as_value& new_val)
	{
//		printf("SET MEMBER: %s at %p for object %p\n", name.c_str(), val.to_object(), this);
		as_value val(new_val);
		as_value old_val;
		if (as_object::get_member(name, &old_val))
		{
			if (old_val.get_type() == as_value::PROPERTY)
			{
				old_val.set_property(val);
				return true;
			}
		}

		// try watch
		as_watch watch;
		m_watch.get(name, &watch);
		if (watch.m_func)
		{
			as_environment env;
			env.push(watch.m_user_data);	// params
			env.push(val);		// newVal
			env.push(old_val);	// oldVal
			env.push(name);	// property
			val.set_undefined();
			(*watch.m_func)(fn_call(&val, this, &env, 4, env.get_top_index()));
		}

		stringi_hash<as_member>::const_iterator it = this->m_members.find(name);
		if (it != this->m_members.end())
		{
			const as_prop_flags flags = (it.get_value()).get_member_flags();
			// is the member read-only ?
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

	as_object_interface* as_object::get_proto() const
	{
		return m_proto.get_ptr();
	}

	bool	as_object::get_member(const tu_stringi& name, as_value* val)
	{
		//printf("GET MEMBER: %s at %p for object %p\n", name.c_str(), val, this);
		as_member m;
		if (m_members.get(name, &m))
		{
			*val = m.get_member_value();
		}
		else
		{
			as_object_interface* proto = get_proto();
			if (proto == NULL)
			{
				return false;
			}

			if (proto->get_member(name, val) == false)
			{
				return false;
			}
		}

		if (val->get_type() == as_value::PROPERTY)
		{
			// binds property (sets the target)
			if (val->m_property_target)
			{
				val->m_property_target->drop_ref();
			}
			val->m_property_target = this;
			add_ref();
		}

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
		if (as_object::get_member(name, &member)) {
			as_prop_flags f = member.get_member_flags();
			f.set_flags(flags);
			member.set_member_flags(f);

			m_members.set(name, member);

			return true;
		}
		return false;
	}

	void	as_object::clear_refs(hash<as_object_interface*, bool>* visited_objects,
		as_object_interface* this_ptr)
	{
		// Is it a reentrance ?
		if (visited_objects->get(this, NULL))
		{
			return;
		}
		visited_objects->set(this, true);

		as_value undefined;
		for (stringi_hash<as_member>::iterator it = m_members.begin();
			it != m_members.end(); ++it)
		{
			as_object_interface* obj = it->second.get_member_value().to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					it->second.set_member_value(undefined);
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
			}
		}
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
					as_environment env;
					int nargs = 0;
					if (id.m_args)
					{
						nargs = id.m_args->size();
						for (int i = nargs - 1; i >=0; i--)
						{
							env.push((*id.m_args)[i]);
						}
					}
					call_method(method, &env, this, nargs, env.get_top_index());
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

//		as_object_interface* proto = get_proto();
//		if (proto)
//		{
//			proto->enumerate(env);
//		}
	}

	bool as_object::watch(const tu_string& name, as_as_function* callback,
		const as_value& user_data)
	{
		if (callback == NULL)
		{
			return false;
		}

		as_watch watch;
		watch.m_func = callback;
		watch.m_user_data = user_data;
		m_watch.set(name, watch);
		return true;
	}

	bool as_object::unwatch(const tu_string& name)
	{
		as_watch watch;
		if (m_watch.get(name, &watch))
		{
			m_watch.erase(name);
			return true;
		}
		return false;
	}

	void as_object::copy_to(as_object_interface* target)
	// Copy all members from 'this' to target
	{
		if (target)
		{
			for (stringi_hash<as_member>::const_iterator it = m_members.begin(); 
				it != m_members.end(); ++it ) 
			{ 
				target->set_member(it->first, it->second.get_member_value()); 
			} 
		}
	}

	void as_object::dump()
	// for debugging
	// retrieves members & print them
	{
		printf("\n*** object 0x%p ***\n", this);
		for (stringi_hash<as_member>::const_iterator it = m_members.begin(); 
			it != m_members.end(); ++it)
		{
			as_value val = it->second.get_member_value();
			if (val.get_type() == as_value::OBJECT)
			{
				printf("%s: <as_object 0x%p>\n", it->first.c_str(), val.to_object());
				continue;
			}
			if (val.get_type() == as_value::PROPERTY)
			{
				printf("%s: <as_property 0x%p, target 0x%p, getter 0x%p, setter 0x%p>\n",
					it->first.c_str(), val.m_property, val.m_property_target,
					val.m_property->m_getter, val.m_property->m_setter);
				continue;
			}

			printf("%s: %s\n", it->first.c_str(), it->second.get_member_value().to_string());
		}
		printf("***\n");
	}

	as_object_interface*	as_object::find_target(const tu_string& path)
	// Find the object referenced by the given path.
	{
		if (path.length() <= 0)
		{
			return NULL;
		}

		as_object_interface*	obj = this;
		
		const char*	p = path.c_str();
		tu_string	subpart;

		for (;;)
		{
			const char*	next_slash = next_slash_or_dot(p);
			subpart = p;
			if (next_slash == p)
			{
				log_error("error: invalid path '%s'\n", path.c_str());
				break;
			}
			else if (next_slash)
			{
				// Cut off the slash and everything after it.
				subpart.resize(next_slash - p);
			}

			as_value val;
			obj->get_member(subpart, &val);
			obj = val.to_object();

			if (obj == NULL || next_slash == NULL)
			{
				break;
			}

			p = next_slash + 1;
		}
		return obj;
	}

	// marks 'this' as 'not garbage'
	void as_object::not_garbage()
	{
		// Whether there were we here already ?
		if (get_heap()->is_garbage(this))
		{
			// 'this' and its members is not garbage
			get_heap()->set(this, false);
			for (stringi_hash<as_member>::iterator it = m_members.begin();
				it != m_members.end(); ++it)
			{
				as_object_interface* obj = it->second.get_member_value().to_object();
				if (obj)
				{
					obj->not_garbage();
				}
			}
		}
	}

}
