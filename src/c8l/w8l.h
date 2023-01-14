/* Wax
 * A waxing Lua Standard Library
 *
 * Copyright (C) 2022 Thadeu A C de Paula
 * (https://github.com/w8lab/wax)
 *
 *
 * This header stores macros that simplifies Lua data handling.
 * Using these macros you will write less code, avoid common mistakes
 * and, even you have a specialized code, you can use these as
 * reference of how to deeper stuffs are done
 */


#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <errno.h>


#define Lua static int
#define LuaReg static const luaL_Reg

/*
 * w8l_pairXY(x,y) add keys to tables where:
 * X represents the key type and its x value
 * Y represents the content type and its y value,
 *
 * Example Lua     ----->  Example Lume
 * t["hello"]=10           w8l_pair_si(L, "hello","10");
 * t[10] = "hello"         w8l_pair_is(L, 10, "hello");
 */

#define w8l_pair_sb(lua_State, key, val) \
	iw8l_sethashtkey((lua_State),(key),boolean,(val))
#define w8l_pair_si(lua_State, key, val) \
	iw8l_sethashtkey((lua_State),(key),integer,(val))
#define w8l_pair_sn(lua_State, key, val) \
	iw8l_sethashtkey((lua_State),(key),number, (val))
#define w8l_pair_ss(lua_State, key, val) \
	iw8l_sethashtkey((lua_State),(key),string, (val))

#define w8l_pair_in(lua_State, key, val) \
	iw8l_pair((lua_State),integer,(key),number, (val))
#define w8l_pair_ii(lua_State, key, val) \
	iw8l_pair((lua_State),integer,(key),integer,(val))
#define w8l_pair_is(lua_State, key, val) \
	iw8l_pair((lua_State),integer,(key),string, (val))
#define w8l_pair_ib(lua_State, key, val) \
	iw8l_pair((lua_State),integer,(key),boolean,(val))



/* Creates an userdata metatable and set its functions */
#if ( LUA_VERSION_NUM < 502 )
	#define w8l_newuserdata_mt(lua_State, udataname, funcs) (\
		luaL_newmetatable((lua_State), (udataname)),            \
		lua_pushvalue((lua_State), -1),                         \
		lua_setfield((lua_State),-2,"__index"),                 \
		luaL_register((lua_State) ,NULL, (funcs))               \
	)
#else
	#define w8l_newuserdata_mt(lua_State, udataname, funcs) (\
		luaL_newmetatable((lua_State), (udataname)),            \
		lua_pushvalue((lua_State), -1),                         \
		lua_setfield((lua_State),-2,"__index"),                 \
		luaL_setfuncs((lua_State), (funcs), 0)                  \
	)
#endif


/*
 * ERROR MACROS
 * Error macro that throws lua error
 * Only can be catched with pcall
 */

#define w8l_error(lua_State, msg) { \
	lua_pushstring((lua_State),(msg)); \
	lua_error(lua_State); \
}


#define w8l_assert(lua_State, cond, msg) \
	if (!(cond)) { \
		w8l_error(lua_State, msg); \
	}



/*
 * FAIL MACROS
 * Make function return immediately condition is fullfilled and
 * return default fail values (boolean false or nil)
 */
#define w8l_failnil_m(lua_State, cond, msg)           \
	if ((cond)) {                                       \
		lua_pushnil((lua_State));                         \
		lua_pushstring((lua_State), (const char *)(msg)); \
		return 2;                                         \
	}


#define w8l_failnil(L, cond) \
	w8l_failnil_m(L, cond, strerror(errno))


#define w8l_failboolean_m(lua_State, cond, msg)       \
	if ((cond)) {                                       \
		lua_pushboolean(lua_State,0);                     \
		lua_pushstring((lua_State), (const char *)(msg)); \
		return 2;                                         \
	}


#define w8l_failboolean(L, cond) \
	w8l_failboolean_m(L, cond, strerror(errno))

/*
 * POLYFILL MACROS
 * Abstraction over Lua versions
 */
#if ( LUA_VERSION_NUM < 502 )
	#define w8l_rawlen(lua_State, index) \
		lua_objlen((lua_State), (index))

	#define w8l_export(lua_State, name, luaL_Reg) \
		luaL_register((lua_State), (name), (luaL_Reg))
#else
	#define w8l_rawlen(lua_State, index) \
		lua_rawlen((lua_State), (index))

	#define w8l_export(lua_State, name, luaL_Reg) \
		luaL_newlib((lua_State), (luaL_Reg))

#endif


/*
 * DEFINITIONS
 */

#if defined(_WIN32) || defined(WIN64)
	#define PLAT_WINDOWS
#elif __unix__
	#define PLAT_POSIX
#endif


#ifndef PATH_MAX
	#ifdef NAME_MAX
		const int PATH_MAX = NAME_MAX;
	#elif MAXPATHLEN
		const int PATH_MAX = MAXPATHLEN;
	#endif
#endif



/*
 * INTERNAL MACROS
 * The following are macro helpers intended for internal use only.
 * They are not documented and can be changed in future.
 */

/* For all Lua types */
#define iw8l_pair(lua_State, ktype, k, vtype, v) (\
	lua_push ## ktype((lua_State),(k)), \
	lua_push ## vtype((lua_State),(v)), \
	lua_settable((lua_State),(-3))      \
)

/* Only for string keys */
#define iw8l_sethashtkey(lua_State, k, vtype, v) (\
	lua_push ## vtype((lua_State),(v)), \
	lua_setfield(L, -2, k)              \
)

/* vim: set fdm=indent fdn=1 ts=2 sts=2 sw=2: */
