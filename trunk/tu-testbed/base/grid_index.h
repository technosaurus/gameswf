// grid_index.h	-- Thatcher Ulrich <tu@tulrich.com> 2004

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Simple grid-based spatial index container.

#ifndef GRID_INDEX_H
#define GRID_INDEX_H


#include "base/tu_config.h"
#include "base/container.h"


//
// grid_index_point
//


template<class coord_t>
struct index_point
// Simple point class for spatial index.
{
	index_point() {}
	index_point(coord_t x_in, coord_t y_in)
		:
		x(x_in),
		y(y_in)
	{
	}

	bool	operator==(const index_point<coord_t>& pt) const
	{
		return x == pt.x && y == pt.y;
	}

//data:
	coord_t	x, y;
};


template<class coord_t>
struct index_box
// Simple bounding box class.
{
	index_box() {}
	index_box(const index_point<coord_t>& min_max_in)
		:
		min(min_max_in),
		max(min_max_in)
	{
	}
	index_box(const index_point<coord_t>& min_in, const index_point<coord_t>& max_in)
		:
		min(min_in),
		max(max_in)
	{
	}

	coord_t	get_width() const { return max.x - min.x; }
	coord_t	get_height() const { return max.y - min.y; }

	void	expand_to_enclose(const index_point<coord_t>& loc)
	{
		if (loc.x < min.x) min.x = loc.x;
		if (loc.y < min.y) min.y = loc.y;
		if (loc.x > max.x) max.x = loc.x;
		if (loc.y > max.y) max.y = loc.y;
	}

	bool	contains_point(const index_point<coord_t>& loc) const
	{
		if (loc.x >= min.x && loc.x <= max.x && loc.y >= min.y && loc.y <= max.y)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

//data:
	index_point<coord_t>	min;
	index_point<coord_t>	max;
};


template<class coord_t, class payload_t>
struct grid_entry_point
// Holds one entry for a grid point cell.
{
	index_point<coord_t>	location;
	payload_t	value;

	grid_entry_point<coord_t, payload_t>*	m_next;
};


template<class coord_t, class payload>
struct grid_index_point
// Grid-based container for points.
{
	typedef index_point<coord_t>	point_t;
	typedef index_box<coord_t>	box_t;
	typedef grid_entry_point<coord_t, payload>	grid_entry_t;

	grid_index_point(const box_t& bound, int x_cells, int y_cells)
		:
		m_bound(bound),
		m_x_cells(x_cells),
		m_y_cells(y_cells)
	{
		assert(x_cells > 0 && y_cells > 0);
		assert(bound.min.x <= bound.max.x);
		assert(bound.min.y <= bound.max.y);

		// Allocate the grid.
		m_array = new grid_entry_t*[x_cells * y_cells];
		memset(&m_array[0], 0, sizeof(grid_entry_t*) * x_cells * y_cells);
	}

	~grid_index_point()
	{
		delete [] m_array;
	}

	const box_t&	get_bound() const { return m_bound; }

	struct iterator
	{
		grid_index_point*	m_index;
		box_t	m_query;
		index_box<int>	m_query_cells;
		int	m_current_cell_x, m_current_cell_y;
		grid_entry_t*	m_current_entry;

		iterator()
			:
			m_index(NULL),
			m_query(point_t(0, 0), point_t(0, 0)),
			m_query_cells(index_point<int>(0, 0), index_point<int>(0, 0)),
			m_current_cell_x(0),
			m_current_cell_y(0),
			m_current_entry(NULL)
		{
		}

		bool	at_end() const { return m_current_entry == NULL; }

		void	operator++()
		{
			if (at_end() == false)
			{
				advance();
			}
		}

		void	advance()
		// Point at next element in the iteration.
		{
			if (m_current_entry)
			{
				// Continue through current cell.
				m_current_entry = m_current_entry->m_next;

				if (at_end() == false)
				{
					return;
				}
			}
			assert(m_current_entry == NULL);

			// Done with current cell; go to next cell.
			m_current_cell_x++;
			while (m_current_cell_y <= m_query_cells.max.y)
			{
				for (;;)
				{
					if (m_current_cell_x > m_query_cells.max.x)
					{
						break;
					}

					m_current_entry = m_index->get_cell(m_current_cell_x, m_current_cell_y);
					if (m_current_entry)
					{
						// Found a valid cell.
						return;
					}

					m_current_cell_x++;
				}
				m_current_cell_x = m_query_cells.min.x;
				m_current_cell_y++;
			}

			assert(m_current_cell_x == m_query_cells.min.x);
			assert(m_current_cell_y == m_query_cells.max.y + 1);

			// No more valid cells.
			assert(at_end());
		}

		grid_entry_t&	operator*()
		{
			assert(at_end() == false && m_current_entry != NULL);
			return *m_current_entry;
		}
		grid_entry_t*	operator->() { return &(operator*()); }

		// @@ TODO Finish
		//
		//bool	operator==(const const_iterator& it) const
		//{
		//	if (at_end() && it.at_end())
		//	{
		//		return true;
		//	}
		//	else
		//	{
		//		return
		//			m_hash == it.m_hash
		//			&& m_index == it.m_index;
		//	}
		//}
		//
		//bool	operator!=(const const_iterator& it) const { return ! (*this == it); }
	};

	iterator	begin(const box_t& q)
	{
		iterator	it;
		it.m_index = this;
		it.m_query = q;
		it.m_query_cells.min = get_containing_cell_clamped(q.min);
		it.m_query_cells.max = get_containing_cell_clamped(q.max);

		assert(it.m_query_cells.min.x <= it.m_query_cells.max.x);
		assert(it.m_query_cells.min.y <= it.m_query_cells.max.y);

		it.m_current_cell_x = it.m_query_cells.min.x;
		it.m_current_cell_y = it.m_query_cells.min.y;
		it.m_current_entry = get_cell(it.m_current_cell_x, it.m_current_cell_y);

		// Make sure iterator starts valid.
		if (it.m_current_entry == NULL)
		{
			it.advance();
		}

		return it;
	}

	iterator	end() const
	{
		iterator	it;
		it.m_index = this;
		it.m_current_entry = NULL;

		return it;
	}

	void	add(const point_t& location, payload p)
	// Insert a point, with the given location and payload, into
	// our index.
	{
		index_point<int>	ip = get_containing_cell_clamped(location);

		grid_entry_t*	new_entry = new grid_entry_t;
		new_entry->location = location;
		new_entry->value = p;

		// Link it into the containing cell.
		int	index = get_cell_index(ip);
		new_entry->m_next = m_array[index];
		m_array[index] = new_entry;
	}

	void	remove(grid_entry_t* entry)
	// Removes the entry from the index, and deletes it.
	{
		assert(entry);

		index_point<int>	ip = get_containing_cell_clamped(entry->location);
		int	index = get_cell_index(ip);

		// Unlink matching entry.
		grid_entry_t**	prev_ptr = &m_array[index];
		grid_entry_t*	ptr = *prev_ptr;
		while (ptr)
		{
			if (ptr == entry)
			{
				// This is the one; unlink it.
				*prev_ptr = ptr->m_next;
				
				delete entry;

				return;
			}
			
			// Go to the next entry.
			prev_ptr = &ptr->m_next;
			ptr = *prev_ptr;
		}

		// Didn't find entry!  Something is wrong.
		assert(0);
	}

	iterator	find(const point_t& location, payload p)
	// Helper.  Search for matching entry.
	{
		for (iterator it = begin(box_t(location, location)); ! it.at_end(); ++it)
		{
			if (it->location == location && it->value == p)
			{
				// Found it.
				return it;
			}
		}

		// Didn't find it.
		assert(it.at_end());
		return it;
	}

private:
	
	grid_entry_t*	get_cell(int x, int y)
	{
		assert(x >= 0 && x < m_x_cells);
		assert(y >= 0 && y < m_y_cells);

		return m_array[x + y * m_x_cells];
	}

	int	get_cell_index(const index_point<int>& ip)
	{
		assert(ip.x >= 0 && ip.x < m_x_cells);
		assert(ip.y >= 0 && ip.y < m_y_cells);

		int	index = ip.x + ip.y * m_x_cells;

		return index;
	}

	index_point<int>	get_containing_cell_clamped(point_t p)
	// Get the indices of the cell that contains the given point.
	{
		index_point<int>	ip;
		ip.x = int(((p.x - m_bound.min.x) * coord_t(m_x_cells)) / (m_bound.max.x - m_bound.min.x));
		ip.y = int(((p.y - m_bound.min.y) * coord_t(m_y_cells)) / (m_bound.max.y - m_bound.min.y));

		// Clamp.
		if (ip.x < 0) ip.x = 0;
		if (ip.x >= m_x_cells) ip.x = m_x_cells - 1;
		if (ip.y < 0) ip.y = 0;
		if (ip.y >= m_y_cells) ip.y = m_y_cells - 1;

		return ip;
	}

//data:
	box_t	m_bound;
	int	m_x_cells;
	int	m_y_cells;
	grid_entry_t**	m_array;
};


//
// grid_index_box
//


//TODO
#if 0


template<class coord_t, class payload>
struct grid_entry_box
// Holds one entry for a grid cell.
{
	box<coord_t>	first;
	payload	second;

	grid_element<coord_t, payload>*	m_next;
};


template<class coord_t, class payload>
struct grid_index_box
// Grid-based container for boxes.
{
	typedef box<coord_t>	box_t;
	typedef grid_entry<coord_t, payload>	grid_entry_t;

	grid_index(const box_t& bounds, int x_cells, int y_cells);
	~grid_index();

	struct iterator
	{
		box<int>	m_query_bound;
		int	m_cell_x, m_cell_y;
		grid_entry_t*	m_current_entry;

		// ... etc ...

		bool	at_end() const { return m_current_entry == NULL; }
	};

	iterator	begin(const box_t& q)
	{
		// ... etc ...
	}

	void	add(const box_t& bound, payload p);
	void	delete(grid_entry_t* entry);

private:
	grid_entry_t*	m_array;
};

#endif // 0 TODO


#endif // GRID_INDEX_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

