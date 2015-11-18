// api_demo.c
//
// This is a Lua module written to simulate Lua's C API, except that it is run
// from within Lua itself. This is written for educational purposes.
//
// Implementation notes:
//
// This module maintains a table in the registry with the key "ApiDemo.States".
// The keys in this table are references controlled by luaL_{ref,unref}, and the
// values are used to save/load the Lua state being simulated.
//

#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>


#define states_table_key "ApiDemo.States"


// Internal typedefs.

typedef struct {
  int ref;
} FakeLuaState;


// Internal globals.

static FakeLuaState *current_state;


// Internal functions.

void print_stack_item(lua_State *L, int i) {
  int ltype = lua_type(L, i);
  switch(ltype) {

    case LUA_TNIL:
      printf("nil");
      return;

    case LUA_TNUMBER:
      printf("%g", lua_tonumber(L, i));
      return;

    case LUA_TBOOLEAN:
      printf("%s", lua_toboolean(L, i) ? "true" : "false");
      return;

    case LUA_TSTRING:
      printf("'%s'", lua_tostring(L, i));
      return;

    case LUA_TTABLE:
      // TODO HERE
      return;

    case LUA_TFUNCTION:
      printf("function:");
      break;

    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
      printf("userdata:");
      break;

    case LUA_TTHREAD:
      printf("thread:");
      break;

    default:
      printf("<internal_error_in_print_stack_item!>");
      return;
  }

  // If we reach here, then we've got a type that we print as a pointer.
  printf("%p", lua_topointer(L, i));
}

void print_stack(lua_State *L) {
  int n = lua_gettop(L);
  printf("stack:");
  for (int i = 1; i <=n; ++i) {
    printf(" ");
    print_stack_item(L, i);
  }
  if (n == 0) printf(" <empty>");
  printf("\n");
}

void load_state(lua_State *L, FakeLuaState *fake_state) {
}

// This function uses the global current_state as a save location.
// This can work because Lua is single threaded.
void save_state(lua_State *L) {
}

// This loads the states table onto the top of the stack.
// It creates the table and sets it in the registry if it doesn't exist yet.
void load_states_table(lua_State *L) {
  // The states table is stored in the registry with key states_table_key.
  // The registory is a table located at the pseudo-index LUA_REGISTRYINDEX.
      // stack = [..]
  lua_getfield(L, LUA_REGISTRYINDEX, states_table_key);
      // stack = [.., states_table | nil]
  if (lua_isnil(L, -1)) {  // The table doesn't exist yet, so let's create it.
      // stack = [.., nil]
    lua_pop(L, 1);
      // stack = [..]
    lua_newtable(L);
      // stack = [.., states_table = {}]
    lua_pushvalue(L, -1);
      // stack = [.., states_table, states_table]
    lua_setfield(L, LUA_REGISTRYINDEX, states_table_key);
  }
      // stack = [.., states_table]
}


// Functions that simulate the C API.

int fake_luaL_newstate(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  print_stack(L);  // TEMP
      // stack = []
  FakeLuaState *fake_state =
      (FakeLuaState *)lua_newuserdata(L, sizeof(FakeLuaState));
      // stack = [fake_L]
  load_states_table(L);
      // stack = [fake_L, states_table]
  lua_newtable(L);
      // stack = [fake_L, states_table, fake_state_data = {}]
  fake_state->ref = luaL_ref(L, 2);  // Set states_table[ref] = fake_state_data.
      // stack = [fake_L, states_table]
  lua_pop(L, 1);
      // stack = [fake_L]
  return 1;  // Number of values to return that are on the stack.
}

int fake_lua_pushnumber(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  print_stack(L);  // TEMP
  return 0;  // Number of values to return that are on the stack.
}

int fake_lua_pushstring(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  print_stack(L);  // TEMP
  return 0;  // Number of values to return that are on the stack.
}

#define register_fn(lua_fn_name) \
  lua_register(L, #lua_fn_name, fake_ ## lua_fn_name)

int setup_globals(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  print_stack(L);  // TEMP
  register_fn(luaL_newstate);
  register_fn(lua_pushnumber);
  register_fn(lua_pushstring);
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
