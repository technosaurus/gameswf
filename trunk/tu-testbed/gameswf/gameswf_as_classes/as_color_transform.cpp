// as_color_transform.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf/gameswf_as_classes/as_transform.h"

namespace gameswf
{
	enum ColorTransformConstructorArgument
	{
		redMultiplier = 1, 
		greenMultiplier, 
		blueMultiplier,
		alphaMultiplier,
		redOffset, 
		greenOffset,
		blueOffset, 
		alphaOffset
	};

	// ColorTranform([redMultiplier:Number], [greenMultiplier:Number], [blueMultiplier:Number], [alphaMultiplier:Number], [redOffset:Number], [greenOffset:Number], [blueOffset:Number], [alphaOffset:Number])
	void	as_global_color_transform_ctor(const fn_call& fn)
	{
		smart_ptr<as_color_transform>	obj;
		obj = new as_color_transform();

		switch ( fn.nargs )
		{
			case alphaOffset:
				obj->m_color_transform.m_[ 3 ][ 1 ] = fn.arg(7).to_number();
			case blueOffset:
				obj->m_color_transform.m_[ 2 ][ 1 ] = fn.arg(6).to_number();
			case greenOffset:
				obj->m_color_transform.m_[ 1 ][ 1 ] = fn.arg(5).to_number();
			case redOffset:
				obj->m_color_transform.m_[ 0 ][ 1 ] = fn.arg(4).to_number();
			case alphaMultiplier:
				obj->m_color_transform.m_[ 3 ][ 0 ] = fn.arg(3).to_number();
			case blueMultiplier:
				obj->m_color_transform.m_[ 2 ][ 0 ] = fn.arg(2).to_number();
			case greenMultiplier:
				obj->m_color_transform.m_[ 1 ][ 0 ] = fn.arg(1).to_number();
			case redMultiplier:
				obj->m_color_transform.m_[ 0 ][ 0 ] = fn.arg(0).to_number();
		}

		 fn.result->set_as_object_interface(obj.get_ptr());
	}

	as_color_transform::as_color_transform()
	{
	}

	//TODO: we should use a switch() instead of string compares.
	bool	as_color_transform::set_member(const tu_stringi& name, const as_value& val)
	{
		if( name == "alphaOffset" )
		{
			m_color_transform.m_[ 3 ][ 1 ] = val.to_number();
			return true;
		}
		else if( name == "blueOffset" )
		{
			m_color_transform.m_[ 2 ][ 1 ] = val.to_number();
			return true;
		}
		else if( name == "greenOffset" )
		{
			m_color_transform.m_[ 1 ][ 1 ] = val.to_number();
			return true;
		}
		else if( name == "redOffset" )
		{
			m_color_transform.m_[ 0 ][ 1 ] = val.to_number();
			return true;
		}
		else if( name == "alphaMultiplier" )
		{
			m_color_transform.m_[ 3 ][ 0 ] = val.to_number();
			return true;
		}
		else if( name == "blueMultiplier" )
		{
			m_color_transform.m_[ 2 ][ 0 ] = val.to_number();
			return true;
		}
		else if( name == "greenMultiplier" )
		{
			m_color_transform.m_[ 1 ][ 0 ] = val.to_number();
			return true;
		}
		else if( name == "redMultiplier" )
		{
			m_color_transform.m_[ 0 ][ 0 ] = val.to_number();
			return true;
		}
		else if( name == "rgb" )
		{
			int rgb;

			rgb = static_cast<int>( val.to_number() );
			m_color_transform.m_[ 0 ][ 0 ] = 0.0f;
			m_color_transform.m_[ 1 ][ 0 ] = 0.0f;
			m_color_transform.m_[ 2 ][ 0 ] = 0.0f;
			m_color_transform.m_[ 3 ][ 0 ] = 0.0f;
			m_color_transform.m_[ 0 ][ 1 ] = static_cast<float>( ( rgb >> 16 ) & 0xFF );
			m_color_transform.m_[ 1 ][ 1 ] = static_cast<float>( ( rgb >> 8 ) & 0xFF );
			m_color_transform.m_[ 2 ][ 1 ] = static_cast<float>( rgb & 0xFF );
			m_color_transform.m_[ 3 ][ 1 ] = 255.0f;
			return true;
		}

		return as_object::set_member( name, val );
	}

	bool	as_color_transform::get_member(const tu_stringi& name, as_value* val)
	{
		if( name == "alphaOffset" )
		{
			val->set_double( m_color_transform.m_[ 3 ][ 1 ] );
			return true;
		}
		else if( name == "blueOffset" )
		{
			val->set_double( m_color_transform.m_[ 2 ][ 1 ] );
			return true;
		}
		else if( name == "greenOffset" )
		{
			val->set_double( m_color_transform.m_[ 1 ][ 1 ] );
			return true;
		}
		else if( name == "redOffset" )
		{
			val->set_double( m_color_transform.m_[ 0 ][ 1 ] );
			return true;
		}
		else if( name == "alphaMultiplier" )
		{
			val->set_double( m_color_transform.m_[ 3 ][ 0 ] );
			return true;
		}
		else if( name == "blueMultiplier" )
		{
			val->set_double( m_color_transform.m_[ 2 ][ 0 ] );
			return true;
		}
		else if( name == "greenMultiplier" )
		{
			val->set_double( m_color_transform.m_[ 1 ][ 0 ] );
			return true;
		}
		else if( name == "redMultiplier" )
		{
			val->set_double( m_color_transform.m_[ 0 ][ 0 ] );
			return true;
		}
		else if( name == "rgb" )
		{
			// TODO: Check behavior
			val->set_undefined();
			return true;
		}

		return as_object::get_member( name, val );
	}

}
