// container.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some non-inline implementation help for generic containers.


#include "base/container.h"


void	tu_string::resize(int new_size)
{
	{
		assert(new_size >= 0);

		if (using_heap() == false)
		{
			if (new_size < 15)
			{
				// Stay with internal storage.
				m_local.m_size = (char) (new_size + 1);
				m_local.m_buffer[new_size] = 0;	// terminate
			}
			else
			{
				// need to allocate heap buffer.
				int	capacity = new_size + 1;
				// round up.
				capacity = (capacity + 15) & ~15;
				char*	buf = (char*) tu_malloc(capacity);

				// Copy existing data.
				strcpy(buf, m_local.m_buffer);

				// Set the heap state.
				m_heap.m_buffer = buf;
				m_heap.m_all_ones = char(~0);
				m_heap.m_size = new_size + 1;
				m_heap.m_capacity = capacity;
			}
		}
		else
		{
			// Currently using heap storage.
			if (new_size < 15)
			{
				// Switch to local storage.

				// Be sure to get stack copies of m_heap info, before we overwrite it.
				char*	old_buffer = m_heap.m_buffer;
				int	old_capacity = m_heap.m_capacity;
				UNUSED(old_capacity);

				// Copy existing string info.
				m_local.m_size = (char) (new_size + 1);
				strncpy(m_local.m_buffer, old_buffer, 16);
				m_local.m_buffer[new_size] = 0;	// ensure termination.

				tu_free(old_buffer, old_capacity);
			}
			else
			{
				// Changing size of heap buffer.
				int	capacity = new_size + 1;
				// Round up.
				capacity = (capacity + 15) & ~15;
				if (capacity != m_heap.m_capacity)
				{
					m_heap.m_buffer = (char*) tu_realloc(m_heap.m_buffer, capacity, m_heap.m_capacity);
					m_heap.m_capacity = capacity;
				}
				// else we're OK with existing buffer.

				m_heap.m_size = new_size + 1;

				// Ensure termination.
				m_heap.m_buffer[new_size] = 0;
			}
		}

		//m_buffer.resize(new_size + 1);
		//m_buffer.back() = 0;	// terminate with \0
	}
}



#ifdef CONTAINER_UNIT_TEST


int	main()
{
	tu_string	a("test1");

	printf("%s\n", a.c_str());

	assert(a == "test1");
	assert(a.length() == 5);

	a += "2";
	assert(a == "test12");

	a += "this is some more text";
	assert(a.length() == 28);

	tu_string	b;
	assert(b.length() == 0);
	assert(b[0] == 0);
	assert(b.c_str()[0] == 0);

	tu_string c = a + b;

	assert(c.length() == a.length());

	c.resize(2);
	assert(c == "te");
	assert(c == tu_string("te"));

	assert(tu_string("fourscore and sevent") == "fourscore and sevent");

	return 0;
}


#endif // CONTAINER_UNIT_TEST
