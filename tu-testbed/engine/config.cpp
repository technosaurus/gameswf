// config.cpp	-- by Thatcher Ulrich <tu@tulrich.com> 22 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Configuration glue.  C++ interface to Lua scripting library.


#include "config.h"
extern "C" {
#include <lualib.h>
}


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
}


void	close()
// Close the config subsystem.
{
	// Nothing really to do here.
}


}; // end namespace config



//
// cvar
//


cvar::cvar( const char* name )
// Constructor; leaves existing value, if any (otherwise it's 'nil').
{
//	lua_pushstring( config::L, name );
//	m_lua_name_reference = lua_ref( config::L, 1 );
	m_name_id = config::get_symbol( name );
}


cvar::cvar( const char* name, const char* val )
// Constructor; initializes to given string value.
{
//	lua_pushstring( config::L, name );
//	m_lua_name_reference = lua_ref( config::L, 1 );
	m_name_id = config::get_symbol( name );
	*this = val;
}


cvar::cvar( const char* name, float val )
// Constructor; initializes to given float value.
{
//	lua_pushstring( config::L, name );
//	m_lua_name_reference = lua_ref( config::L, 1 );
	m_name_id = config::get_symbol( name );
	*this = val;
}


cvar::~cvar()
// Destructor; make sure our references are released.
{
	// drop lua reference to our name, so our name string can be
	// gc'd if not referenced elsewhere.
//	lua_unref( config::L, m_lua_name_reference );
//	m_lua_name_reference = LUA_NOREF;
}


const char*	cvar::get_name() const
// Return our name.  The char[] storage is valid at least as long
// as this variable is alive.
{
//	lua_getref( config::L, m_lua_name_reference );
	config::lua_push_symbol_name( m_name_id );
	return lua_tostring( config::L, -1 );
}


// cvar::get_name_reference() -- for comparisons or storage maybe?


cvar::operator float() const
// Convert the variable to a float and return it.
{
	lua_getglobals( config::L );
//	lua_getref( config::L, m_lua_name_reference );
	config::lua_push_symbol_name( m_name_id );
	lua_gettable( config::L, -2 );	// get the value of our variable from the global table.
	float	f = lua_tonumber( config::L, -1 );
	lua_pop( config::L, 2 );	// discard global table & the number result.

	return f;
}


void	cvar::operator=( float f )
// Assign a float to this lua variable.
{
	lua_getglobals( config::L );
//	lua_getref( config::L, m_lua_name_reference );
	config::lua_push_symbol_name( m_name_id );
	lua_pushnumber( config::L, f );
	lua_settable( config::L, -2 );
	lua_pop( config::L, 1 );	// discard the global table.
}


cvar::operator const char*() const
// Convert to a string.
//
// xxx there are garbage-collection issues here!  Returned string
// has no valid reference once stack is cleared!
// Possible fixes:
// - return some kind of proxy object that holds a locked Lua ref
// - return a C++ "string" value
// - hold a locked reference in this instance; drop it on next call to this conversion operator.
{
	lua_getglobals( config::L );
//	lua_getref( config::L, m_lua_name_reference );
	config::lua_push_symbol_name( m_name_id );
	lua_gettable( config::L, -2 );	// get the value of our variable from the global table.
	const char*	c = lua_tostring( config::L, -1 );
	// TODO: grab a locked reference to the string!
	lua_pop( config::L, 2 );	// discard global table & the string result.

	return c;
}


void	cvar::operator=( const char* s )
// Assign a string to this lua variable.
{
	lua_getglobals( config::L );
//	lua_getref( config::L, m_lua_name_reference );
	config::lua_push_symbol_name( m_name_id );
	lua_pushstring( config::L, s );
	lua_settable( config::L, -2 );
	lua_pop( config::L, 1 );	// discard the global table.
}

//	void	operator=( const cvar c );

