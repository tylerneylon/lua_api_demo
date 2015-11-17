// api_demo.c
//
// This is a Lua module written to simulate Lua's C API, except that it is run
// from within Lua itself. This is written for educational purposes.
//

#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>

int fake_luaL_newstate(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  return 0;  // Number of values to return that are on the stack.
}

int fake_lua_pushnumber(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  return 0;  // Number of values to return that are on the stack.
}

int fake_lua_pushstring(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  return 0;  // Number of values to return that are on the stack.
}

int setup_globals(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  lua_register(L, "luaL_newstate", fake_luaL_newstate);
  lua_register(L, "lua_pushnumber", fake_lua_pushnumber);
  lua_register(L, "lua_pushstring", fake_lua_pushstring);
  return 0;  // Number of values to return that are on the stack.
}

int luaopen_api_demo(lua_State *L) {
  luaL_Reg fns[] = {
    {"setup_globals", setup_globals},
    {NULL, NULL}
  };
  luaL_register(L, "api_demo", fns);
  return 1;  // Number of Lua-facing return values on the Lua stack in L.
}
