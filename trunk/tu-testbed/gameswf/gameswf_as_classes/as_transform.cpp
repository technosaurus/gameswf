// as_transform.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf/gameswf_as_classes/as_transform.h"

namespace gameswf
{

	// Tranform( mc:MovieClip )
	void	as_global_transform_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			smart_ptr<as_transform>	obj;
			if ( character * movie = cast_to<character>( fn.arg(0).to_object() ) )
			{
				obj = new as_transform( movie );
			}
			fn.result->set_as_object_interface(obj.get_ptr());
		}
	}

	as_transform::as_transform( character * movie_clip ) :
		m_movie( movie_clip )
	{
	}

	//TODO: we should use a switch() instead of string compares.
	bool	as_transform::set_member(const tu_stringi& name, const as_value& val)
	{
		if( name == "colorTransform" )
		{
			m_color_transform = cast_to<as_color_transform>( val.to_object() );
			m_movie->set_cxform( m_color_transform->m_color_transform );
			return true;
		}
		else if( name == "concatenatedColorTransform" )
		{
			//read-only
			return true;
		}
		else if( name == "matrix" )
		{
			assert( false && "todo" );
			return true;
		}
		else if( name == "concatenatedMatrix" )
		{
			//read-only
			return true;
		}
		else if( name == "pixelBounds" )
		{
			assert( false && "todo" );
			return true;
		}

		return as_object::set_member( name, val );
	}

	bool	as_transform::get_member(const tu_stringi& name, as_value* val)
	{
		if( name == "colorTransform" )
		{
			val->set_as_object_interface( m_color_transform.get_ptr() );
			return true;
		}
		else if( name == "concatenatedColorTransform" )
		{
			assert( false && "todo" );
			return true;
		}
		else if( name == "matrix" )
		{
			assert( false && "todo" );
			return true;
		}
		else if( name == "concatenatedMatrix" )
		{
			assert( false && "todo" );
			return true;
		}
		else if( name == "pixelBounds" )
		{
			assert( false && "todo" );
			return true;
		}

		return as_object::get_member( name, val );
	}

}
