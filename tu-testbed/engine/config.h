// config.h	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Configuration glue.  C++ interface to Lua scripting library, plus utilities.


#ifndef CONFIG_H
#define CONFIG_H


extern "C" {
#include <lua.h>
}


namespace config {
	void	open();
	void	close();
	// void	clear();

	//
	// Symbol table.
	//

	typedef int symbol;

	symbol	check_symbol( const char* name );	// symbol look-up, but don't create if it's not there.
	symbol	get_symbol( const char* name );	// symbol look-up; create if necessary.
	const char*	get_symbol_name( symbol id );
	void	lua_push_symbol_name( symbol id );

	//
	// Globals to help interface w/ Lua.  User code usually won't be
	// interested in these.
	//
	extern lua_State*	L;	// Lua state.  Passed as a param to most lua_ calls.
	extern int	g_cfloat_tag;
};


class cvar {
// Hook to Lua global variable.  cvars are dynamically typed and
// have to do hash lookups & type coercion to access from the C++ side.
// Some possible gc issues with conversion to C strings.
public:
	cvar( const char* name );
	cvar( const char* name, const char* val );
	cvar( const char* name, float val );
	~cvar();

	const char*	get_name() const;

	// get_name_reference() -- for comparisons or storage maybe?

	// float access
	operator float() const;
	void	operator=( float f );

	// string access
	operator const char*() const;
	void	operator=( const char* s );

	// TODO: vec3 access
	// TODO: actor, component access

	// cvar assignment
	void	operator=( const cvar c );

private:
//	int	m_lua_name_reference;	// Lua reference to the name of this var, for fast lookup.
	config::symbol	m_name_id;
};


// E.g:
//
// cvar( "varname", "string" );
// float f = cvar( "varname" );


#define declare_cvar( name, value ) cvar name( #name, value )


// e.g.
//
//	declare_cvar( snowflakes, 5.0f );
//	delcare_cvar( snowflake_image, "snowflake.jpg" );
//
//	snowflakes++;
//	snowflakes = 10;
//	for ( i = 0; i < snowflakes; i++ ) {	// hash lookup here -- but uses symbol ref, not raw string.
//		make_snowflake( snowflake_image );
//	}


class cfloat
// A class that holds a floating point value.  Accessible from Lua via
// a global variable.  Has the time efficiency of an ordinary C++
// float when accessed from C++, so it's OK to use these in inner
// loops.
{
public:
	cfloat( const char* name, float val )
	{
		m_value = val;

		// if ( name ) is already defined in Lua {
		//	if ( is a cfloat ) {
		//		error( "cfloat( name ) already defined!" );
		//	} else {
		//		value = tonumber( original value )
		//	}
		// }

		// make sure config::open() has been called.
		config::open();

		// assign us to a Lua global variable, and identify us to Lua as a cfloat.
		lua_pushuserdata( config::L, this );
		lua_settag( config::L, config::g_cfloat_tag );
		lua_setglobal( config::L, name );
	}

	~cfloat()
	{
		// do nothing?
		// set our global var to nil ???  To a Lua number equal to our value???
	}

	operator float()
	// Return our value.
	{
		return m_value;
	}

	void operator=( float f )
	// Set our value to f.
	{
		m_value = f;
	}

	// +=, -=, *=, /= etc

	// config::open() will hook in some handlers so Lua can have access to our value.
//private:
	float	m_value;
	// hold a name_id?
};


#define declare_cfloat( name, value ) cfloat name( #name, ( value ) )


// usage:
//
//	extern cfloat max_pixel_error;
//	declare_cfloat( max_pixel_error ) = 5.0f;
//
//	// Use it like an ordinary float.
//	max_pixel_error = 10.f;
//	printf( "%f\n", max_pixel_error );
//
//	// But, it can be accessed from Lua at nominal cost...
//
//	config.lua:
//		max_pixel_error = 5.0f;
//
//	Lua scripts, debugger or console can find and manipulate.


#endif // CONFIG_H
