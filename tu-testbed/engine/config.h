// config.h	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Configuration glue.  C++ interface to Lua scripting library.


#ifndef CONFIG_H
#define CONFIG_H


extern "C" {
#include <lua.h>
}


namespace config {
	void	open();
	void	close();

	extern lua_State*	g_luastate;
	extern int	g_cfloat_tag;
};


class cvar {
// Hook to Lua global variable.  cvars are dynamically typed and
// have to do hash lookups & type coercion to access from the C++ side.
// Some possible gc issues with conversion to C strings.
public:
	cvar( const char* name ) {
		m_lua_name = name;
	}

	cvar( const char* name, const char* val ) {
		m_lua_name = name;
		*this = val;
	}

	cvar( const char* name, float val ) {
		m_lua_name = name;
		*this = val;
	}

	const char*	get_name() { return m_lua_name; }

	operator float()
	// Convert the variable to a float and return it.
	{
		lua_getglobal( config::g_luastate, m_lua_name );
		float	f = lua_tonumber( config::g_luastate, 1 );
		lua_pop( config::g_luastate, 1 );
		return f;
	}

	void	operator=( float f )
	// Assign a float to this lua variable.
	{
		lua_pushnumber( config::g_luastate, f );
		lua_setglobal( config::g_luastate, m_lua_name );
	}

	operator const char*()
	// Convert to a string.
	//
	// xxx there are garbage-collection issues here!  Returned string
	// has no valid reference once stack is cleared!
	{
		lua_getglobal( config::g_luastate, m_lua_name );
		const char* c = lua_tostring( config::g_luastate, 1 );
		lua_pop( config::g_luastate, 1 );
		return c;
	}

	void	operator=( const char* s )
	// Assign a string to this lua variable.
	{
		lua_pushstring( config::g_luastate, s );
		lua_setglobal( config::g_luastate, m_lua_name );
	}

private:
	const char*	m_lua_name;	// name of Lua global variable.
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
//	for ( i = 0; i < snowflakes; i++ ) {	// hash lookup here.
//		make_snowflake( snowflake_image );
//	}


class cfloat
// A class that holds a floating point value.  Accessible from Lua via
// a global variable.  Has the time efficiency of an ordinary C++
// float when accessed from C++, so it's cool to use these in inner
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
		// }

		// make sure config::open() has been called.
		config::open();

		// assign us to a Lua global variable, and identify us to Lua as a cfloat.
		lua_pushuserdata( config::g_luastate, this );
		lua_settag( config::g_luastate, config::g_cfloat_tag );
		lua_setglobal( config::g_luastate, name );
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
