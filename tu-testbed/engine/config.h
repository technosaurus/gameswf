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

/*
	typedef int symbol;

	symbol	check_symbol( const char* name );	// symbol look-up, but don't create if it's not there.
	symbol	get_symbol( const char* name );	// symbol look-up; create if necessary.
	const char*	get_symbol_name( symbol id );
	void	lua_push_symbol_name( symbol id );
*/

	//
	// Globals to help interface w/ Lua.  User code usually won't be
	// interested in these.
	//
	extern lua_State*	L;	// Lua state.  Passed as a param to most lua_ calls.
	extern int	g_cfloat_tag;
};


class cfloat
// A class that holds a floating point value.  Accessible from Lua via
// a global variable.  Has the time efficiency of an ordinary C++
// float when accessed from C++, so it's OK to use these in inner
// loops.
{
public:
	cfloat( float val )
	// Constructs; does not assign to a Lua global variable.
	{
		m_value = val;
	}
	
	cfloat( const char* name, float val )
	// Constructs, and assigns to the Lua global variable whose name is given.
	{
		m_value = val;

		lua_push();
		lua_setglobal( config::L, name );
	}

	void	lua_push()
	{
		// Identify us to Lua as a cfloat.
		config::open();	// make sure Lua is set up.
		lua_pushuserdata( config::L, this );
		lua_settag( config::L, config::g_cfloat_tag );
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


// usage, very much like a C++ global float:
//
//	extern cfloat max_pixel_error;
//	declare_cfloat( max_pixel_error, 5.0f );	would this work?: cfloat name( #name ) = value;
//  declare_cfloat( max_pixel_error );
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


class cglobal;


class cvalue {
// Hook to anonymous Lua value.  Has smart-pointer-like semantics for
// holding a reference to its value (but relies on Lua gc, not
// reference counting).
public:
	cvalue( const cvalue& c );
	cvalue( const char* lua_constructor );
	~cvalue();
	static cvalue	lua_stacktop_reference();	// factory; creates a reference to the Lua stack top, and pops.

private:
	cvalue();	// Used only in cvalue::lua_stacktop_reference()
public:

	void	operator=( const char* str );
	void	operator=( float num );
	void	operator=( const cvalue& cval );

	operator	float() const;	// tonumber
	operator	const char*() const;	// tostring

	// Lua tables can be indexed by both strings and numbers.
	void	set( const char* index, cvalue val );
	void	set( float index, cvalue val );
	void	set( cvalue index, cvalue val );

	void	set( const char* index, float f );
	void	set( float index, float f );

	void	set( const char* index, const char* str );
	void	set( float index, const char* str );

	cvalue	get( const char* index );
	cvalue	get( float index );

//	const char*;

	// const char*	type();

	void	lua_push() const;	// push our value onto the Lua stack.

private:
	int	m_lua_ref;

	friend class cglobal;
};


// cvalue cv("point(10, 10)");
// cv.set("x", 20);
// printf("y = %f\n", (float) cv.get("y"));


class cglobal {
// Hook to Lua global variable.  cglobals are dynamically typed and
// have to do hash lookups & type coercion to access from the C++ side.
// Some possible gc issues with conversion to C strings.
public:
	cglobal( const char* name );
	cglobal( const char* name, const cvalue& val );
	cglobal( const char* name, const char* str );
	cglobal( const char* name, float val );
	~cglobal();

	const char*	get_name() const;

	// get_name_reference() -- for comparisons or storage maybe?

	operator float() const;
	operator const char*() const;

	void	operator=( float f );
	void	operator=( const char* s );
	void	operator=( cfloat c );

	// cvalue access
	operator cvalue() const;
	void	operator=( const cvalue& c );

	// TODO: vec3 access
	// TODO: actor, component access

	// cvar assignment
	void	operator=( cglobal c );

private:
	int	m_lua_name_reference;	// Lua reference to the name of this var, for faster lookup.
};


#define declare_cglobal( name, value ) cglobal name( #name, value )


// E.g:
//
// declare_cglobal( "varname", "string" );
// float f = cvar( "varname" );



// e.g.
//
//	declare_cglobal( snowflakes, 5.0f );
//	declare_cglobal( snowflake_image, "snowflake.jpg" );
//
//	snowflakes++;
//	snowflakes = 10;
//	for ( i = 0; i < snowflakes; i++ ) {	// hash lookup here
//		make_snowflake( snowflake_image );	// hash lookup here
//	}


//cvalue( "varname", "'string'" );	// evaluates 2nd arg and assigns to Lua global of first arg.
//cvalue( "point(10, 10)" );	// quick way to evaluate some Lua code.
//cvalue( "varname" );	// gets the value of a global, no?  cglobal(var) or config::get(var) could be shortcuts.
//// how to do "varname = val from c"?  config::set(var, float/string/cvalue); cset(var, val) == shorthand?
//declare_cvalue("varname", "'string'");
//declare_cstring(varname, "string");	// Will C preprocessor quote the embedded quotes???
//declare_cfloat(varname, 10.f);



#endif // CONFIG_H
