// container.h	-- Thatcher Ulrich <tu@tulrich.com> 31 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic C++ containers.  Problem: STL is murder on compile times,
// and is hard to debug.  And also it's really easy to shoot yourself
// in the foot.  In general I don't like it very much.
//
// These are not as comprehensive or general, but I think might be
// more usable for day-to-day coding.  Someday when STL compiles with
// lightning speed, these can go away or (where the interface
// diverges) turn into tiny wrappers.
//
// Basic approach: array<> acts like a stripped-down version of
// std::vector<>, or a slightly beefed up native array.
//
// hash<> is a very simple take on std::hash_map, without the awful
// iterator-oriented interface.


#ifndef CONTAINER_H
#define CONTAINER_H


#include <stdlib.h>
#include <string.h>
#include <new>	// for placement new
#include "base/utility.h"
#include "base/dlmalloc.h"


#ifdef _WIN32
#pragma warning(disable : 4345)	// in MSVC 7.1, warning about placement new POD default initializer
#endif // _WIN32


template<class T>
class array {
// Resizable array.  Don't put anything in here that can't be moved
// around by bitwise copy.  Don't keep the address of an element; the
// array contents will move around as it gets resized.
//
// Default constructor and destructor get called on the elements as
// they are added or removed from the active part of the array.
public:
	array() : m_buffer(0), m_size(0), m_buffer_size(0) {}
	array(int size_hint) : m_buffer(0), m_size(0), m_buffer_size(0) { resize(size_hint); }
	~array() {
		clear();
	}

	// Basic array access.
	T&	operator[](int index) { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	const T&	operator[](int index) const { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	int	size() const { return m_size; }

	void	push_back(const T& val)
	// Insert the given element at the end of the array.
	{
		// DO NOT pass elements of your own vector into
		// push_back()!  Since we're using references,
		// resize() may munge the element storage!
		assert(&val < &m_buffer[0] || &val > &m_buffer[m_buffer_size]);

		int	new_size = m_size + 1;
		resize(new_size);
		(*this)[new_size-1] = val;
	}

	T	pop_back()
	// Return the last element and remove it from the array.
	{
		assert(m_size > 0);
		T	t = (*this)[m_size - 1];
		resize(m_size - 1);
		return t;
	}

	// Access the first element.
	T&	front() { return (*this)[0]; }
	const T&	front() const { return (*this)[0]; }

	// Access the last element.
	T&	back() { return (*this)[m_size-1]; }
	const T&	back() const { return (*this)[m_size-1]; }

	void	clear()
	// Empty and destruct all elements.
	{
		resize(0);
	}

	// void	remove(int index);

	void	operator=(const array<T>& a)
	// Array copy.  Copies the contents of a into this array.
	{
		resize(a.size());
		for (int i = 0; i < m_size; i++) {
			*(m_buffer + i) = a[i];
		}
	}


	void	remove(int index)
	// Removing an element from the array is an expensive operation!
	// It compacts only after removing the last element.
	{
		assert(index >= 0 && index < m_size);

		if (m_size == 1)
		{
			clear();
		}
		else
		{
			m_buffer[index].~T();

			memmove(m_buffer+index, m_buffer+index+1, sizeof(T) * (m_size - 1 - index));
			m_size--;
		}
	}


	void	insert(int index, const T& val = T())
	// Insert the given object at the given index shifting all the elements up.
	{
		assert(index >= 0 && index <= m_size);

		resize(m_size + 1);

		if (index < m_size - 1)
		{
			// is it safe to use memmove?
			memmove(m_buffer+index+1, m_buffer+index, sizeof(T) * (m_size - 1 - index));
		}

		m_buffer[index] = val;
	}


	void	append(const array<T>& other)
	// Append the given data to our array.
	{
		append(other.m_buffer, other.size());
	}


	void	append(const T other[], int count)
	// Append the given data to our array.
	{
		if (count > 0)
		{
			int	size0 = m_size;
			resize(m_size + count);
			memcpy(&m_buffer[size0], &other[0], count * sizeof(T));
		}
	}


	void	resize(int new_size)
	// Preserve existing elements.  Newly created elements are
	// initialized with default element of T.  Removed elements
	// are destructed.
	{
		assert(new_size >= 0);

		int	old_size = m_size;
		m_size = new_size;

		// Destruct old elements (if we're shrinking).
		{for (int i = new_size; i < old_size; i++) {
			(m_buffer + i)->~T();
		}}

		if (new_size == 0) {
			m_buffer_size = 0;
			reserve(m_buffer_size);
		} else if (m_size <= m_buffer_size && m_size > m_buffer_size >> 1) {
			// don't compact yet.
			assert(m_buffer != 0);
		} else {
			m_buffer_size = m_size + (m_size >> 2);
			reserve(m_buffer_size);
		}

		// Copy default T into new elements.
		{for (int i = old_size; i < new_size; i++) {
			new (m_buffer + i) T();	// placement new
		}}
	}

	void	reserve(int rsize)
	{
		assert(m_size >= 0);
		m_buffer_size = rsize;

		// Resize the buffer.
		if (m_buffer_size == 0) {
			if (m_buffer) {
				dlfree(m_buffer);
			}
			m_buffer = 0;
		} else {
			if (m_buffer) {
				m_buffer = (T*) dlrealloc(m_buffer, sizeof(T) * m_buffer_size);
			} else {
				m_buffer = (T*) dlmalloc(sizeof(T) * m_buffer_size);
			}
		}			
	}

	void	transfer_members(array<T>* a)
	// UNSAFE!  Low-level utility function: replace this array's
	// members with a's members.
	{
		m_buffer = a->m_buffer;
		m_size = a->m_size;
		m_buffer_size = a->m_buffer_size;

		a->m_buffer = 0;
		a->m_size = 0;
		a->m_buffer_size = 0;
	}

private:
	T*	m_buffer;
	int	m_size;
	int	m_buffer_size;
};


inline unsigned int	bernstein_hash(const void* data_in, int size, unsigned int seed = 5381)
// Computes a hash of the given data buffer.
// Hash function suggested by http://www.cs.yorku.ca/~oz/hash.html
// Due to Dan Bernstein.  Allegedly very good on strings.
{
	const unsigned char*	data = (const unsigned char*) data_in;
	unsigned int	h = seed;
	while (size > 0) {
		size--;
		h = ((h << 5) + h) ^ (unsigned) data[size];
	}

	// Alternative: "sdbm" hash function, suggested at same web page above.
	// h = 0;
	// for bytes { h = (h << 16) + (h << 6) - hash + *p; }

	return h;
}


template<class T>
class fixed_size_hash
// Computes a hash of an object's representation.
{
public:
	static unsigned int	compute(const T& data)
	{
		unsigned char*	p = (unsigned char*) &data;
		int	size = sizeof(T);

		return bernstein_hash(p, size);
	}
};


template<class T, class U, class hash_functor = fixed_size_hash<T> >
class hash {
// Fairly stupid hash table.  TODO: study Lua's hashes, use their
// tricks.
//
// Never shrinks, unless you explicitly clear() or resize() it.
// Expands on demand, though.  For best results, if you know roughly
// how big your table will be, default it to that size when you create
// it.
public:
	hash() { m_entry_count = 0;  m_size_mask = 0; }
	hash(int size_hint) { m_entry_count = 0; m_size_mask = 0; resize(size_hint); }
	~hash() {
		clear();
	}

	// @@ need a "remove()" or "set()" function, to replace/remove existing key.

	void	set(const T& key, const U& value)
	// Set a new or existing value under the key, to the value.
	{
		if (m_table.size() > 0)
		{
			unsigned int	hash_value = hash_functor::compute(key);
			int	index = hash_value & m_size_mask;	// % m_table.size();
			for (int i = 0; i < m_table[index].size(); i++) {
				if (m_table[index][i].key == key) {
					m_table[index][i].value = value;
					return;
				}
			}
		}

		// Entry under key doesn't exist.
		add(key, value);
	}

	void	add(const T& key, const U& value)
	// Add a new value to the hash table, under the specified key.
	{
		assert(get(key, NULL) == false);

		m_entry_count++;
		check_expand();

		unsigned int	hash_value = hash_functor::compute(key);
		entry	e;
		e.key = key;
		e.value = value;

		int	index = hash_value & m_size_mask;	// % m_table.size();
		m_table[index].push_back(e);
	}

	void	clear()
	// Remove all entries from the hash table.
	{
		for (int i = 0; i < m_table.size(); i++) {
			m_table[i].clear();
		}
		m_table.clear();
		m_entry_count = 0;
		m_size_mask = 0;
	}

	bool	is_empty() const
	// Returns true if the hash is empty.
	{
		return m_entry_count==0;
	}


	bool	get(const T& key, U* value) const
	// Retrieve the value under the given key.
	//
	// If there's no value under the key, then return false and leave
	// *value alone.
	//
	// If there is a value, return true, and set *value to the entry's
	// value.
	//
	// If value == NULL, return true or false according to the
	// presence of the key, but don't touch *value.
	{
		if (m_table.size() == 0) {
			return false;
		}

		unsigned int	hash_value = hash_functor::compute(key);
		int	index = hash_value & m_size_mask;	// % m_table.size();
		for (int i = 0; i < m_table[index].size(); i++) {
			if (m_table[index][i].key == key) {
				if (value) {
					*value = m_table[index][i].value;
				}
				return true;
			}
		}
		return false;
	}

	void	check_expand()
	// Resize the hash table, if it looks like the size is too small
	// for the current number of entries.
	{
		int	new_size = m_table.size();

		if (m_table.size() == 0 && m_entry_count > 0) {
			// Need to expand.  Let's make the base table size 256
			// elements.
			new_size = 256;
		} else if (m_table.size() * 2 < m_entry_count) {
			// Expand.
			new_size = (m_entry_count * 3) >> 1;
		}

		if (new_size != m_table.size()) {
			resize(new_size);
		}
	}


	void	resize(int new_size)
	// Resize the hash table to the given size (Rehash the contents of
	// the current table).
	{
		if (new_size <= 0) {
			// Special case.
			clear();
			return;
		}

		// Force new_size to be a power of two.
		int	bits = fchop(log2((float)(new_size-1)) + 1);
		assert((1 << bits) >= new_size);

		new_size = 1 << bits;
		m_size_mask = new_size - 1;

		array< array<entry> >	new_table(new_size);

		// Copy all entries to the new table.
		{for (int i = 0; i < m_table.size(); i++) {
			for (int j = 0; j < m_table[i].size(); j++) {
				entry&	e = m_table[i][j];
				unsigned int	hash_value = hash_functor::compute(e.key);
				
				int	index = hash_value & m_size_mask;	// % new_table.size();
				new_table[index].push_back(e);
			}
			m_table[i].clear();
		}}
		m_table.clear();

		// Replace old table data with new table's data.
		m_table.transfer_members(&new_table);
	}


	// Iterator API.  @@ TODO fix the API so it's more STL-like.  No point in confusing clients.

	struct iterator
	{
		// @@ in STL, operator* returns a pair<T,U>, right?
		// We could return a struct entry, and rename the
		// members of struct entry as {first,second}, no?

		T	get_key() const { return m_hash->m_table[m_index0][m_index1].key; }
		U	get_value() const { return m_hash->m_table[m_index0][m_index1].value; }

		void	operator++()
		{
			assert(m_hash);

			if (m_index0 < m_hash->m_table.size())
			{
				if (m_index1 < m_hash->m_table[m_index0].size() - 1)
				{
					m_index1++;
				}
				else
				{
					m_index0++;
					while (m_index0 < m_hash->m_table.size()
					       && m_hash->m_table[m_index0].size() == 0)
					{
						m_index0++;
					}
					m_index1 = 0;
				}
			}
		}

		bool	operator==(const iterator& it) const
		{
			if (is_end() && it.is_end())
			{
				return true;
			}
			else
			{
				return
					m_hash == it.m_hash
					&& m_index0 == it.m_index0
					&& m_index1 == it.m_index1;
			}
		}

		bool	operator!=(const iterator& it) const { return ! (*this == it); }


		bool	is_end() const
		{
			return
				m_hash == NULL
				|| m_index0 >= m_hash->m_table.size();
		}

	private:
		friend class hash<T,U,hash_functor>;

		iterator(const hash* h, int i0, int i1)
			:
			m_hash(h),
			m_index0(i0),
			m_index1(i1)
		{
		}

		const hash*	m_hash;
		int	m_index0, m_index1;
	};
	friend struct iterator;

	iterator	begin() const
	{
		// Scan til we hit the first valid entry.
		int	i0 = 0;
		while (i0 < m_table.size()
			&& m_table[i0].size() == 0)
		{
			i0++;
		}
		return iterator(this, i0, 0);
	}
	iterator	end() const { return iterator(this, m_table.size(), 0); }

private:

	struct entry {
		T	key;
		U	value;
	};
	
	int	m_entry_count;
	int	m_size_mask;
	array< array<entry> >	m_table;
};


// very simple string-like type
class tu_string
{
public:
	tu_string() { m_buffer.push_back(0); }
	tu_string(const char* str)
	{
		m_buffer.resize(strlen(str) + 1);
		strcpy(&m_buffer[0], str);
	}
	tu_string(const tu_string& str)
	{
		m_buffer = str.m_buffer;
	}

	operator const char*() const
	{
		return &m_buffer[0];
	}

	const char*	c_str() const
	{
		return (const char*) (*this);
	}

	// operator= returns void; if you want to know why, ask Charles Bloom :)
	// (executive summary: a = b = c is an invitation to bad code)
	void	operator=(const char* str)
	{
		m_buffer.resize(strlen(str) + 1);
		strcpy(&m_buffer[0], str);
	}

	void	operator=(const tu_string& str)
	{
		m_buffer = str.m_buffer;
	}

	bool	operator==(const char* str) const
	{
		return strcmp(*this, str) == 0;
	}

	bool	operator==(const tu_string& str) const
	{
		return strcmp(*this, str) == 0;
	}

	int	length() const { return m_buffer.size() - 1; }
	int	size() const { return length(); }

	char&	operator[](int index) { return m_buffer[index]; }
	const char&	operator[](int index) const { return m_buffer[index]; }

	void	operator+=(const char* str)
	{
		int	str_length = strlen(str);
		int	old_length = length();
		assert(old_length >= 0);
		m_buffer.resize(old_length + str_length + 1);
		strcpy(&m_buffer[old_length], str);
	}

	void	operator+=(const tu_string& str)
	{
		int	str_length = str.length();
		int	old_length = length();
		assert(old_length >= 0);
		m_buffer.resize(old_length + str_length + 1);
		strcpy(&m_buffer[old_length], str.c_str());
	}

	tu_string	operator+(const char* str) const
	// NOT EFFICIENT!  But convenient.
	{
		tu_string	new_string(*this);
		new_string += str;
		return new_string;
	}

	bool	operator<(const char* str) const
	{
		return strcmp(c_str(), str) < 0;
	}
	bool	operator<(const tu_string& str) const
	{
		return *this < str.c_str();
	}

	void	resize(int new_size)
	{
		assert(new_size >= 0);

		m_buffer.resize(new_size + 1);
		m_buffer.back() = 0;	// terminate with \0
	}

private:
	array<char>	m_buffer;	// we do store the terminating \0
};


template<class T>
class string_hash_functor
// Computes a hash of a string-like object (something that has
// ::length() and ::[int]).
{
public:
	static unsigned int	compute(const T& data)
	{
		int	size = data.length();

		return bernstein_hash((const char*) data, size);
	}
};


template<class U>
class string_hash : public hash<tu_string, U, string_hash_functor<tu_string> >
{
};


#endif // CONTAINER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
