// smart_ptr.h	-- by Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Smart (ref-counting) pointer classes.  Uses "intrusive" approach:
// the types pointed to must have add_ref() and drop_ref() methods.
// Typically this is done by inheriting from a ref_counted class,
// although the nice thing about templates is that no particular
// ref-counted class is mandated.


#ifndef SMART_PTR_H


#include "base/tu_config.h"
#include "base/utility.h"


// A smart (strong) pointer asserts that the pointed-to object will
// not go away as long as the strong pointer is valid.  "Owners" of an
// object should keep strong pointers; other objects should use a
// strong pointer temporarily while they are actively using the
// object, to prevent the object from being deleted.
template<class T>
struct smart_ptr
{
	smart_ptr(T* ptr)
		:
		m_ptr(ptr)
	{
		if (ptr)
		{
			ptr->add_ref();
		}
	}

	smart_ptr() : m_ptr(NULL) {}
	smart_ptr(const smart_ptr<T>& s)
		:
		m_ptr(s.m_ptr)
	{
		if (m_ptr)
		{
			m_ptr->add_ref();
		}
	}

	~smart_ptr()
	{
		if (m_ptr)
		{
			m_ptr->drop_ref();
		}
	}

	operator bool() const { return m_ptr != NULL; }
	void	operator=(const smart_ptr<T>& s) { set_ref(s.m_ptr); }
//	void	operator=(const weak_ptr<T>& w);
	T*	operator->() { assert(m_ptr); return m_ptr; }
	T*	get_ptr() { return m_ptr; }
	bool	operator==(T* p) { return m_ptr == p; }
	bool	operator!=(T* p) { return m_ptr != p; }

	// Provide work-alikes for static_cast, dynamic_cast, implicit up-cast?  ("gentle_cast" a la ajb?)

private:
	void	set_ref(T* ptr)
	{
		if (ptr != m_ptr)
		{
			if (m_ptr)
			{
				m_ptr->drop_ref();
			}
			m_ptr = ptr;

			if (m_ptr)
			{
				m_ptr->add_ref();
			}
		}
	}

//	friend weak_ptr;

	T*	m_ptr;
};



// @@ TODO
#if 0
// A weak pointer points at an object, but the object may be deleted
// at any time, in which case the weak pointer automatically becomes
// NULL.  The only way to use a weak pointer is by converting it to a
// strong pointer (i.e. for temporary use).
//
// Usage idiom:
//
// if (smart_ptr<my_type> ptr = m_weak_ptr_to_my_type) { ... use ptr->whatever() safely in here ... }
//
template<class T>
struct weak_ptr
{
	// @@ implement similar to smart_ptr<>, but with link(s) to other weak_ptr's.
	// type T must have some methods for adding/deleting associated weak_ptr's.
};
#endif // 0


#endif // SMART_PTR_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
