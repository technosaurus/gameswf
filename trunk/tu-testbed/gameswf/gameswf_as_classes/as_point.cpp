// as_point.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf/gameswf_as_classes/as_point.h"

namespace gameswf
{

	// Point(x:Number, y:Number)
	void	as_global_point_ctor(const fn_call& fn)
	{
		if (fn.nargs == 2)
		{
			smart_ptr<as_point>	obj;
			obj = new as_point(fn.arg(0).to_float(), fn.arg(1).to_float());
			fn.result->set_as_object(obj.get_ptr());
		}
	}
	
	void	as_point_add(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_point* point = cast_to<as_point>(fn.this_ptr);
		if (point == NULL)
		{
			return;
		}

		if (fn.arg(0).to_object() == NULL)
		{
			return;
		}

		as_point* other_point = cast_to<as_point>(fn.arg(0).to_object());
		if (other_point)
		{
			smart_ptr<as_point>	obj;
			obj = new as_point( point->m_point.m_x + other_point->m_point.m_x, point->m_point.m_y + other_point->m_point.m_y );
			fn.result->set_as_object(obj.get_ptr());   
		}
	}

	void	as_point_subtract(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_point* point = cast_to<as_point>(fn.this_ptr);
		if (point == NULL)
		{
			return;
		}

		if (fn.arg(0).to_object() == NULL)
		{
			return;
		}

		as_point* other_point = cast_to<as_point>(fn.arg(0).to_object());
		if (other_point)
		{
			smart_ptr<as_point>	obj;
			obj = new as_point( point->m_point.m_x - other_point->m_point.m_x, point->m_point.m_y - other_point->m_point.m_y );
			fn.result->set_as_object(obj.get_ptr());   
		}
	}

	void	as_point_normalize(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_point* point = cast_to<as_point>(fn.this_ptr);
		if (point == NULL)
		{
			return;
		}

		float targeted_length = fn.arg(0).to_float();
		if (targeted_length != 0.0)
		{
			float ratio = targeted_length / sqrtf(point->m_point.m_x * point->m_point.m_x + point->m_point.m_y * point->m_point.m_y);

			point->m_point.m_x *= ratio;
			point->m_point.m_y *= ratio;
		}
	}

	as_point::as_point(float x, float y) :
	   m_point( x, y )
	{
		builtin_member("add", as_point_add);
		builtin_member("subtract", as_point_subtract);
		builtin_member("normalize", as_point_normalize);
	}

	//TODO: we should use a switch() instead of string compares.
	bool	as_point::set_member(const tu_stringi& name, const as_value& val)
	{
		if( name == "x" )
		{
			m_point.m_x = val.to_float();
			return true;
		}
		else if( name == "y" )
		{
			m_point.m_y = val.to_float();
			return true;
		}

		return as_object::set_member( name, val );
	}

	bool	as_point::get_member(const tu_stringi& name, as_value* val)
	{
		if( name == "x" )
		{
			val->set_double( m_point.m_x );
			return true;
		}
		else if( name == "y" )
		{
			val->set_double( m_point.m_y );
			return true;
		}
		else if( name == "length" )
		{
			val->set_double( m_point.get_length() );
			return true;
		}

		return as_object::get_member( name, val );
	}


	bool as_point::is_instance_of(const as_function& constructor) const
	{
		const as_c_function * function = cast_to<as_c_function>(&constructor);
		if( function && function->m_func == as_global_point_ctor )
		{
			return true;
		}

		return as_object::is_instance_of(constructor);
	}

};
