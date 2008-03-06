// as_matrix.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf/gameswf_as_classes/as_matrix.h"
#include "gameswf/gameswf_as_classes/as_point.h"

namespace gameswf
{

	// Point(x:Number, y:Number)
	void	as_global_matrix_ctor(const fn_call& fn)
	{
		smart_ptr<as_matrix> matrix;

		matrix = new as_matrix;
		switch(fn.nargs)
		{
			default:

	// TODO: symbolic constants instead of 6,5 ,...
			case 6:
				if (fn.arg(5).m_type == as_value::NUMBER)
				{
					matrix->m_matrix.m_[1][2] = fn.arg(5).to_number();
				}

			case 5:
				if (fn.arg(4).m_type == as_value::NUMBER )
				{
					matrix->m_matrix.m_[0][2] = fn.arg(4).to_number();
				}

			case 4:
				if (fn.arg(3).m_type == as_value::NUMBER )
				{
					matrix->m_matrix.m_[1][1] = fn.arg(3).to_number();
				}

			case 3:
				if (fn.arg(2).m_type == as_value::NUMBER )
				{
					matrix->m_matrix.m_[1][0] = fn.arg(2).to_number();
				}

			case 2:
				if (fn.arg(1).m_type == as_value::NUMBER )
				{
					matrix->m_matrix.m_[0][1] = fn.arg(1).to_number();
				}

			case 1:
				if (fn.arg(0).m_type == as_value::NUMBER )
				{
					matrix->m_matrix.m_[0][0] = fn.arg(0).to_number();
				}

			case 0:
				break;
		}
		fn.result->set_as_object_interface(matrix.get_ptr());
	}

	void	as_matrix_translate(const fn_call& fn)
	{
		if (fn.nargs < 2)
		{
			return;
		}

		as_matrix* matrix = cast_to<as_matrix>(fn.this_ptr);
		if (matrix == NULL)
		{
			return;
		}

		if (fn.arg(0).m_type != as_value::NUMBER || fn.arg(1).m_type != as_value::NUMBER)
		{
			return;
		}

		gameswf::matrix temp;

		temp.concatenate_translation(fn.arg(0).to_number(), fn.arg(1).to_number());
		temp.concatenate(matrix->m_matrix);
		matrix->m_matrix = temp;
	}

	void	as_matrix_rotate(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_matrix* matrix = cast_to<as_matrix>(fn.this_ptr);
		if (matrix == NULL)
		{
			return;
		}

		if (fn.arg(0).m_type != as_value::NUMBER)
		{
			return;
		}

		gameswf::matrix rotation_matrix;

		rotation_matrix.set_scale_rotation( 1, 1, fn.arg(0).to_number() );

		rotation_matrix.concatenate( matrix->m_matrix );

		matrix->m_matrix = rotation_matrix;
	}

	void	as_matrix_scale(const fn_call& fn)
	{
		if (fn.nargs < 2)
		{
			return;
		}

		as_matrix* matrix = cast_to<as_matrix>(fn.this_ptr);
		if (matrix == NULL)
		{
			return;
		}

		if (fn.arg(0).m_type != as_value::NUMBER || fn.arg(1).m_type != as_value::NUMBER )
		{
			return;
		}

		gameswf::matrix scale_matrix;

		scale_matrix.set_scale_rotation(fn.arg(0).to_number(), fn.arg(1).to_number(), 0);

		scale_matrix.concatenate(matrix->m_matrix);

		matrix->m_matrix = scale_matrix;
	}

	void	as_matrix_concat(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_matrix* matrix = cast_to<as_matrix>(fn.this_ptr);
		if (matrix == NULL)
		{
			return;
		}

		if (fn.arg(0).to_object() == NULL)
		{
			return;
		}

		as_matrix* other_matrix = cast_to<as_matrix>(fn.arg(0).to_object());
		if (other_matrix)
		{
			gameswf::matrix
				temp;

			temp = other_matrix->m_matrix;
			temp.concatenate( matrix->m_matrix );
			matrix->m_matrix = temp;
			//matrix->m_matrix.concatenate( other_matrix->m_matrix );
		}
	}

	void as_matrix_transformPoint(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_matrix* matrix = cast_to<as_matrix>(fn.this_ptr);
		if (matrix == NULL)
		{
			return;
		}

		if ( fn.arg(0).to_object() == NULL )
		{
			return;
		}

		as_point* point = cast_to<as_point>(fn.arg(0).to_object());
		if (point)
		{
			smart_ptr<as_point>	result;
			result = new as_point(0.0f, 0.0f);
			matrix->m_matrix.transform( &result->m_point, point->m_point);
			fn.result->set_as_object_interface(result.get_ptr());
		}
	}

	as_matrix::as_matrix()
	{
		set_member("translate", as_matrix_translate);
		set_member("rotate", as_matrix_rotate);
		set_member("scale", as_matrix_scale);
		set_member("concat", as_matrix_concat);
		set_member("transformPoint", as_matrix_transformPoint);
	}

};
