// axial_box.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// simple AABB structure


#ifndef AXIAL_BOX_H
#define AXIAL_BOX_H


#include "engine/geometry.h"


struct axial_box
{
	axial_box();	// zero box
	axial_box(const vec3& min, const vec3& max);

	bool	is_valid() const;

	vec3	get_center() const { return (m_min + m_max) * 0.5f; }
	vec3	get_extent() const { return (m_max - m_min) * 0.5f; }

	const vec3&	get_min() const { return m_min; }
	const vec3&	get_max() const { return m_max; }

	// Get one of the 8 corner verts.
	vec3	get_corner(int i) const;

	void	set_min_max(const vec3& min, const vec3& max);

	void	set_center_extent(const vec3& center, const vec3& extent);

	// preserve center
	void	set_extent(const vec3& extent);

	// preserve extent
	void	set_center(const vec3& center);

	// adjust bounds along one axis.
	void	set_axis_min(int axis, float new_min);
	void	set_axis_max(int axis, float new_max);

	// Expand the box.
	void	set_enclosing(const vec3& v);

private:
	vec3	m_min, m_max;
};


inline	axial_box::axial_box()
// Construct a zero box.
{
	m_min = vec3::zero;
	m_max = vec3::zero;

	assert(is_valid());
}


inline	axial_box::axial_box(const vec3& min, const vec3& max)
// Init from extremes.
{
	set_min_max(min, max);
}


inline bool	axial_box::is_valid() const
// Return true if we're OK.
{
	return
		m_min.x <= m_max.x
		&& m_min.y <= m_max.y
		&& m_min.z <= m_max.z;
}


inline void	axial_box::set_min_max(const vec3& min, const vec3& max)
{
	m_min = min;
	m_max = max;
	
	assert(is_valid());
}


inline void	axial_box::set_center_extent(const vec3& center, const vec3& extent)
{
	set_min_max(center - extent, center + extent);
}


inline void	axial_box::set_extent(const vec3& extent)
{
	set_center_extent(get_center(), extent);
}


inline void	axial_box::set_center(const vec3& center)
{
	set_center_extent(center, get_extent());
}


inline vec3	axial_box::get_corner(int i) const
{
	assert(is_valid());
	assert(i >= 0 && i < 8);

	return vec3(
		i & 1 ? m_min.x : m_max.x,
		i & 2 ? m_min.y : m_max.y,
		i & 4 ? m_min.z : m_max.z);
}


inline void	axial_box::set_axis_min(int axis, float new_min)
{
	assert(is_valid());

	m_min.set(axis, new_min);

	assert(is_valid());
}


inline void	axial_box::set_axis_max(int axis, float new_max)
{
	assert(is_valid());

	m_max.set(axis, new_max);

	assert(is_valid());
}


// @@ should probably un-inline this...
inline void	axial_box::set_enclosing(const vec3& v)
// Ensure that the box encloses the point.
{
	m_min.x = fmin(m_min.x, v.x);
	m_min.y = fmin(m_min.y, v.y);
	m_min.z = fmin(m_min.z, v.z);
	m_max.x = fmax(m_max.x, v.x);
	m_max.y = fmax(m_max.y, v.y);
	m_max.z = fmax(m_max.z, v.z);

	assert(is_valid());
}


#endif // AXIAL_BOX_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
