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


template<class T>
class array {
// Resizable array.  Don't put anything in here that needs a
// destructor called on it when the array is destructed.
public:
	array(int size_hint = 0) : m_buffer(0), m_size(0), m_buffer_size(0) { resize(size_hint); }
	~array() { resize(0); }

	T&	operator[](int index) { assert(index >= 0 && index < m_size); return m_buffer[index]; }

	int	size() { return m_size; }
	void	push_back(const T& val)
	{
		int	new_size = m_size + 1;
		resize(new_size);
		(*this)[new_size-1] = val;
	}

	void	resize(int new_size)
	// Preserve existing elements.  Newly created elements are unintialized.
	// TODO: should not be inline!  No reason to bloat the code.
	{
		assert(new_size >= 0);

		m_size = new_size;
		if (m_size <= m_buffer_size) {
			return;
		}

		// Need to expand the buffer.
		m_buffer_size = m_size + (m_size >> 2);
		if (m_buffer_size == 0) {
			if (m_buffer) {
				free(m_buffer);
			}
			m_buffer = 0;
		} else {
			if (m_buffer) {
				m_buffer = (T*) realloc(m_buffer, sizeof(T) * m_buffer_size);
			} else {
				m_buffer = (T*) malloc(sizeof(T) * m_buffer_size);
			}
		}			
	}

	void	clear() { resize(0); }

	// void	remove(int index);

private:
	T*	m_buffer;
	int	m_size;
	int	m_buffer_size;
};


inline	compute_hash(const char* data, int size)
// Computes a hash of the given buffer.
{
	int	h = 0;
	while (size > 0) {
		h = (h << 5) + *data * 101;	// hm, replace with a real hash function.
		data++;
		size--;
	}
	return h;
}


template<class T, class U>
class hash {
// Really stupid hash, for hashing using fixed-sized objects as keys.
public:
	hash() {}

	void	add(T key, U value) {
		int	hash_value = compute_hash((char*) &key, sizeof(key));
		entry	e;
		e.key = key;
		e.value = value;

		int	index = hash_value & 4095;
		m_table[index].push_back(e);
	}

	void	clear()
	{
		for (int i = 0; i < 4096; i++) {
			m_table[i].clear();
		}
	}

	bool	get(T key, U* value) {
		int	hash_value = compute_hash((char*) &key, sizeof(key));
		int	index = hash_value & 4095;
		for (int i = 0; i < m_table[index].size(); i++) {
			if (m_table[index][i].key == key) {
				*value = m_table[index][i].value;
				return true;
			}
		}
		return false;
	}

private:
	struct entry {
		T	key;
		U	value;
	};
	array<entry>	m_table[4096];
};


#endif // CONTAINER_H

