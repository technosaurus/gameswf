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

lua_State*	g_luastate = NULL;
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
	g_luastate = lua_open( LUA_STACK_SIZE );

	// Init the standard Lua libs.
	lua_baselibopen( g_luastate );
	lua_iolibopen( g_luastate );
	lua_strlibopen( g_luastate );
	lua_mathlibopen( g_luastate );

	//
	// Attach the cfloat hooks
	//

	g_cfloat_tag = lua_newtag( config::g_luastate );

	lua_pushcfunction( config::g_luastate, cfloat_get );
	lua_settagmethod( config::g_luastate, config::g_cfloat_tag, "getglobal" );	// xxx is "getglobal" right?

	lua_pushcfunction( config::g_luastate, cfloat_set );
	lua_settagmethod( config::g_luastate, config::g_cfloat_tag, "setglobal" );	// xxx is "setglobal" right?

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

