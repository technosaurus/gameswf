// container.h	-- Thatcher Ulrich <tu@tulrich.com> 31 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic C++ containers.  Problem: STL is murder on compile times,
// and is hard to debug.  And also it's really easy to shoot yourself
// in the foot.  These are not as comprehensive, but might be
// better in those other dimensions.


#ifndef CONTAINER_H
#define CONTAINER_H


#include <engine/utility.h>


// Resizable array.  Don't put anything in here that needs a
// destructor called on it when the array is destructed.
template<class T>
class array {
public:
	array(int size_hint = 0) : m_buffer(0), m_size(0) { resize(size_hint); }
	~array() { resize(0); }

	T&	operator[](int index) { assert(index >= 0 && index < m_size); return m_buffer[index]; }

	int	size() { return m_size; }
	void	push_back(const T& val) { resize(m_size + 1); (*this)[m_size-1] = val; }

	void	resize(int new_size)
	// Preserve existing elements.  Newly created elements are unintialized.
	// TODO: should not be inline!  No reason to bloat the code.
	{
		// TODO: does an allocation every time.  Should probably be a little smarter.
		// Although maybe it's better to let realloc() work its magic.

		assert(new_size >= 0);

		m_size = new_size;
		if (new_size == 0) {
			if (m_buffer) {
				free(m_buffer);
			}
			m_buffer = 0;
		} else {
			if (m_buffer) {
				m_buffer = (T*) realloc(m_buffer, sizeof(T) * m_size);
			} else {
				m_buffer = (T*) malloc(sizeof(T) * m_size);
			}
		}			
	}

	// void	remove(int index);

private:
	T*	m_buffer;
	int	m_size;
};


#endif // CONTAINER_H

