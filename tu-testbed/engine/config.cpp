// config.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Configuration glue.  C++ interface to Lua scripting library.


#include "config.h"
extern "C" {
#include <lualib.h>
}
#include "utility.h"


namespace config {


static const int	LUA_STACK_SIZE = 2048;


static bool	s_open = false;

lua_State*	L = NULL;
int	g_cfloat_tag = 0;


static int	cfloat_get( lua_State* L )
// Lua CFunction which returns the value of the cfloat passed in on
// the top of the Lua stack.  Attach this to the "getglobal" event for
// cfloats.
{
	// arg 1: varname
	// arg 2: raw var value (in this case the cfloat*)

	cfloat*	cf = static_cast<cfloat*>( lua_touserdata( L, 2 ) );
	lua_pushnumber( L, float( *cf ) );

	return 1;
}


static int	cfloat_set( lua_State* L )
// Lua CFunction which sets the value of the given cfloat to the given lua
// number.
{
	// arg 1: varname
	// arg 2: previous var value (in this case the cfloat*)
	// arg 3: new var value

	cfloat*	cf = static_cast<cfloat*>( lua_touserdata( L, 2 ) );
	*cf = lua_tonumber( L, 3 );

	return 0;
}


void	open()
// Initialize the config subsystem.  ...
{
	if ( s_open == true ) return;

	//
	// Initialize Lua.
	//
	L = lua_open( LUA_STACK_SIZE );

	// Init the standard Lua libs.
	lua_baselibopen( L );
	lua_iolibopen( L );
	lua_strlibopen( L );
	lua_mathlibopen( L );

	//
	// Attach the cfloat hooks
	//

	g_cfloat_tag = lua_newtag( config::L );

	lua_pushcfunction( config::L, cfloat_get );
	lua_settagmethod( config::L, config::g_cfloat_tag, "getglobal" );	// xxx is "getglobal" right?

	lua_pushcfunction( config::L, cfloat_set );
	lua_settagmethod( config::L, config::g_cfloat_tag, "setglobal" );	// xxx is "setglobal" right?

	// set tag methods for add, sub, mul, div, pow, unm, lt

	//	gettable{ min, max, default, comment }
	//	settable{ min, max, default, comment }

	s_open = true;
}


void	close()
// Close the config subsystem.
{
	// Nothing really to do here.
}


}; // end namespace config



//
// cglobal
//


cglobal::cglobal( const char* name )
// Constructor; leaves existing value, if any (otherwise it's 'nil').
{
	config::open();

	lua_pushstring( config::L, name );
	m_lua_name_reference = lua_ref( config::L, 1 );
}


cglobal::cglobal( const char* name, const cvalue& val )
// Constructor; initializes to given Lua value.
{
	config::open();

	lua_pushstring( config::L, name );
	m_lua_name_reference = lua_ref( config::L, 1 );
	*this = val;	// invoke operator=(const cvalue& val)
}


cglobal::cglobal( const char* name, const char* val )
// Constructor; initializes to given string value.
{
	config::open();

	lua_pushstring( config::L, name );
	m_lua_name_reference = lua_ref( config::L, 1 );
	*this = val;	// invoke operator=(const char*)
}


cglobal::cglobal( const char* name, float val )
// Constructor; initializes to given float value.
{
	config::open();

	lua_pushstring( config::L, name );
	m_lua_name_reference = lua_ref( config::L, 1 );
	*this = val;	// invoke operator=(float f)
}


cglobal::~cglobal()
// Destructor; make sure our references are released.
{
	// drop lua reference to our name, so our name string can be
	// gc'd if not referenced elsewhere.
	lua_unref( config::L, m_lua_name_reference );
	m_lua_name_reference = LUA_NOREF;
}


const char*	cglobal::get_name() const
// Return our name.  The char[] storage is valid at least as long
// as this variable is alive.
{
	lua_getref( config::L, m_lua_name_reference );
	return lua_tostring( config::L, -1 );
}


cglobal::operator float() const
// Convert the variable to a float and return it.
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	lua_gettable( config::L, -2 );	// get the value of our variable from the global table.
	float	f = lua_tonumber( config::L, -1 );
	lua_pop( config::L, 2 );	// discard global table & the number result.

	return f;
}


void	cglobal::operator=( float f )
// Assign a float to this lua variable.
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	lua_pushnumber( config::L, f );
	lua_settable( config::L, -3 );
	lua_pop( config::L, 1 );	// discard the global table.
}


cglobal::operator const char*() const
// Convert to a string.
//
// xxx there are garbage-collection issues here!  Returned string
// has no valid reference once stack is cleared!
// Possible fixes:
// - return some kind of proxy object that holds a locked Lua ref
// - return a C++ "string" value; i.e. make a copy
// - hold a locked reference in this instance; drop it on next call to this conversion operator (blech).
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	lua_gettable( config::L, -2 );	// get the value of our variable from the global table.
	const char*	c = lua_tostring( config::L, -1 );
	// TODO: grab a locked reference to the string!  Or copy it!
	lua_pop( config::L, 2 );	// discard global table & the string result.

	return c;
}


void	cglobal::operator=( const cvalue& val )
// Assign a Lua value to this lua global variable.
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	val.lua_push();
	lua_settable( config::L, -3 );
	lua_pop( config::L, 1 );	// discard the global table.
}


void	cglobal::operator=( const char* s )
// Assign a string to this lua variable.
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	lua_pushstring( config::L, s );
	lua_settable( config::L, -3 );
	lua_pop( config::L, 1 );	// discard the global table.
}


cglobal::operator cvalue() const
// Return a reference to our value.
{
	lua_getglobals( config::L );
	lua_getref( config::L, m_lua_name_reference );
	cvalue	c = cvalue::lua_stacktop_reference();
	lua_pop( config::L, 1 );	// pop the global table.

	return c;
}


//	void	operator=( const cglobal c );



//
// cvalue
//


cvalue::cvalue( const char* lua_constructor )
// Evaluates the given code and takes a reference to the result.
{
	config::open();

	lua_dostring( config::L, lua_constructor );	// @@ could check for error return codes, presence of return value, etc.
	m_lua_ref = lua_ref( config::L, 1 );
}


cvalue::cvalue( const cvalue& c )
{
	lua_getref( config::L, c.m_lua_ref );
	m_lua_ref = lua_ref( config::L, 1 );
}


cvalue::cvalue()
// Creates an reference to nothing.  Use this only in special
// circumstances; i.e. when you're about to set m_lua_ref manually.
{
	config::open();
	m_lua_ref = LUA_NOREF;
}


cvalue	cvalue::lua_stacktop_reference()
// Factory function; pops the value off the top of the Lua stack, and
// return a cvalue that references the popped value.
{
	cvalue	c;
	c.m_lua_ref = lua_ref( config::L, 1 );
	return c;
}


cvalue::~cvalue()
// Drop our Lua reference, to allow our value to be gc'd.
{
	lua_unref( config::L, m_lua_ref );
	m_lua_ref = LUA_NOREF;
}


void	cvalue::lua_push() const
// Push our value onto the top of the Lua stack.
{
	assert( m_lua_ref != LUA_NOREF );
	lua_getref( config::L, m_lua_ref );
}


void	cvalue::operator=( const char* str )
// Transfer our reference to the given string.
{
	lua_unref( config::L, m_lua_ref );
	lua_pushstring( config::L, str );
	m_lua_ref = lua_ref( config::L, 1 );
}


void	cvalue::operator=( const cvalue& c )
// Reference the thing that c references.
{
	lua_unref( config::L, m_lua_ref );
	lua_getref( config::L, c.m_lua_ref );
	m_lua_ref = lua_ref( config::L, 1 );
}


cvalue::operator float() const
// Converts this Lua value to a number, and returns it.
{
	lua_getref( config::L, m_lua_ref );
	float	f = lua_tonumber( config::L, -1 );
	lua_pop( config::L, 1 );
	return f;
}


cvalue::operator const char*() const
// Converts this Lua value to a string, and returns it.
{
	lua_getref( config::L, m_lua_ref );
	const char*	str = lua_tostring( config::L, -1 );
	lua_pop( config::L, 1 );	// @@ I'm pretty sure this imperils the string, if we just had to do a tostring() conversion!  Look into this, and/or make a copy of the string.
	return str;
}


cvalue	cvalue::get( const char* index )
// Does a table lookup.  *this should be a Lua table, and index is its
// key.
{
	lua_getref( config::L, m_lua_ref );
	lua_pushstring( config::L, index );
	lua_gettable( config::L, -2 );
	cvalue	c = cvalue::lua_stacktop_reference();	// references the value on the top of the Lua stack.
	lua_pop( config::L, 1 );

	return c;
}

