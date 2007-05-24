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
				obj->set_member(fn.arg(0).to_string(), as_value(getter, setter));
				fn.result->set_bool(true);
				return;
			}
		}
		fn.result->set_bool(false);
	}

	bool	as_object::set_member(const tu_stringi& name, const as_value& val )
	{
		//printf("SET MEMBER: %s at %p for object %p\n", name.c_str(), val.to_object(), this);
		if (name == "prototype")
		{
			if (m_prototype) m_prototype->drop_ref();
			m_prototype = val.to_object();
			if (m_prototype) m_prototype->add_ref();
		}
		else
		{
			stringi_hash<as_member>::const_iterator it = this->m_members.find(name);

			if ( it != this->m_members.end() ) {

				const as_prop_flags flags = (it.get_value()).get_member_flags();

				// is the member read-only ?
				if (!flags.get_read_only()) {
					m_members.set(name, as_member(val, flags));
				}

			} else {
				m_members.set(name, as_member(val));
			}
		}
		return true;
	}

	bool	as_object::get_member(const tu_stringi& name, as_value* val)
	{
		//printf("GET MEMBER: %s at %p for object %p\n", name.c_str(), val, this);
		if (name == "prototype")
		{
			val->set_as_object_interface(m_prototype);
			return true;
		}
		else {
			as_member m;

			if (m_members.get(name, &m) == false)
			{
				if (m_prototype != NULL)
				{
					return m_prototype->get_member(name, val);
				}
				return false;
			} else {
				*val=m.get_member_value();
				return true;
			}
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

	void	as_object::clear()
	{
		m_members.clear();
		if (m_prototype)
		{
			m_prototype->drop_ref();
			m_prototype = NULL;
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
					as_environment* env = method.to_as_function()->m_env;
					assert(env);

					call_method(method, env, NULL, 0, env->get_top_index());
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

				IF_VERBOSE_ACTION(log_msg("---enumerate - push: %s\n",
					it.get_key().c_str()));
			}

			++it;
		}

		if (m_prototype)
		{
			const as_object* prototype = m_prototype->cast_to_as_object();
			if (prototype != NULL)
			{
				stringi_hash<as_member>::const_iterator it = prototype->m_members.begin();
				while (it != prototype->m_members.end())
				{
					const as_member member = (it.get_value());

					if (! member.get_member_flags().get_dont_enum())
					{
						env->push(as_value(it.get_key()));

						IF_VERBOSE_ACTION(log_msg("---enumerate - push: %s\n",
							it.get_key().c_str()));
					}

					++it;
				};
			}
		}
	}

}

