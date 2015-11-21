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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>


#define states_table_key     "ApiDemo.States"
#define demo_state_metatable "ApiDemo.Metatable"


// Internal typedefs.

typedef struct {
  int ref;
} FakeLuaState;


// Internal globals.

static FakeLuaState *current_state;


// Internal functions.

static void print_item(lua_State *L, int i, int as_key);

static int is_identifier(const char *s) {
  while (*s) {
    if (!isalnum(*s) && *s != '_') return 0;
    ++s;
  }
  return 1;
}

static int is_seq(lua_State *L, int i) {
      // stack = [..]
  lua_pushnil(L);
      // stack = [.., nil]
  int keynum = 1;
  while (lua_next(L, i)) {
      // stack = [.., key, value]
    lua_rawgeti(L, i, keynum);
      // stack = [.., key, value, t[keynum]]
    if (lua_isnil(L, -1)) {
      lua_pop(L, 3);
      // stack = [..]
      return 0;
    }
    lua_pop(L, 2);
      // stack = [.., key]
    keynum++;
  }
      // stack = [..]
  return 1;
}

static void print_seq(lua_State *L, int i) {
  printf("{");

  for (int k = 1;; ++k) {
        // stack = [..]
    lua_rawgeti(L, i, k);
        // stack = [.., t[k]]
    if (lua_isnil(L, -1)) break;
    if (k > 1) printf(", ");
    print_item(L, -1, 0);  // 0 --> as_key
    lua_pop(L, 1);
        // stack = [..]
  }
        // stack = [.., nil]
  lua_pop(L, 1);
        // stack = [..]

  printf("}");
}

static void print_table(lua_State *L, int i) {
  // Ensure i is an absolute index as we'll be pushing/popping things after it.
  if (i < 0) i = lua_gettop(L) + i + 1;

  const char *prefix = "{";
  if (is_seq(L, i)) {
    print_seq(L, i);  // This case includes all empty tables.
  } else {
        // stack = [..]
    lua_pushnil(L);
        // stack = [.., nil]
    while (lua_next(L, i)) {
      printf("%s", prefix);
        // stack = [.., key, value]
      print_item(L, -2, 1);  // 1 --> as_key
      printf(" = ");
      print_item(L, -1, 0);  // 0 --> as_key
      lua_pop(L, 1);  // So the last-used key is on top.
        // stack = [.., key]
      prefix = ", ";
    }
        // stack = [..]
    printf("}");
  }
}

static void print_item(lua_State *L, int i, int as_key) {
  int ltype = lua_type(L, i);
  // Set up first, last and start and end delimiters.
  const char *first = (as_key ? "[" : "");
  const char *last  = (as_key ? "]" : "");
  switch(ltype) {

    case LUA_TNIL:
      printf("nil");  // This can't be a key, so we can ignore as_key here.
      return;

    case LUA_TNUMBER:
      printf("%s%g%s", first, lua_tonumber(L, i), last);
      return;

    case LUA_TBOOLEAN:
      printf("%s%s%s", first, lua_toboolean(L, i) ? "true" : "false", last);
      return;

    case LUA_TSTRING:
      {
        const char *s = lua_tostring(L, i);
        if (is_identifier(s) && as_key) {
          printf("%s", s);
        } else {
          printf("%s'%s'%s", first, s, last);
        }
      }
      return;

    case LUA_TTABLE:
      printf("%s", first);
      print_table(L, i);
      printf("%s", last);
      return;

    case LUA_TFUNCTION:
      printf("%sfunction:", first);
      break;

    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
      printf("%suserdata:", first);
      break;

    case LUA_TTHREAD:
      printf("%sthread:", first);
      break;

    default:
      printf("<internal_error_in_print_stack_item!>");
      return;
  }

  // If we reach here, then we've got a type that we print as a pointer.
  printf("%p%s", lua_topointer(L, i), last);
}

static void print_stack(lua_State *L) {
  int n = lua_gettop(L);
  printf("stack:");
  for (int i = 1; i <=n; ++i) {
    printf(" ");
    print_item(L, i, 0);  // 0 --> as_key
  }
  if (n == 0) printf(" <empty>");
  printf("\n");
}

// This loads the states table onto the top of the stack.
// It creates the table and sets it in the registry if it doesn't exist yet.
static void load_states_table(lua_State *L) {
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

static void load_state(lua_State *L, FakeLuaState *demo_state) {

  // We expect every load_state to be paired by a following save_state call.
  // If that expectation is not met, this assert may be triggered.
  assert(current_state == NULL);

  // Clear the stack and load the states table.
  lua_settop(L, 0);
      // stack = []
  load_states_table(L);
      // stack = [states_table]

  // Load the table for this demo_state.
  lua_rawgeti(L, 1, demo_state->ref);
      // stack = [states_table, demo_state_data]
  lua_remove(L, 1);
      // stack = [demo_state_data]

  // Load the state.
  lua_getfield(L, 1, "num_items");
      // stack = [demo_state_data, num_items]
  assert(lua_isnumber(L, 2));
  int num_items = lua_tointeger(L, 2);
  lua_pop(L, 1);
  for (int k = 1; k <= num_items; ++k) {
      // stack = [demo_state_data, <first k-1 items of saved stack>]
    lua_rawgeti(L, 1, k);
      // stack = [demo_state_data, <first k-1 items of saved stack>, item[k]]
  }
      // stack = [demo_state_data, <loaded state>]
  lua_remove(L, 1);
      // stack = [<loaded state>]

  // Set the current_state for later use.
  current_state = demo_state;
}

// This function uses the global current_state as a save location.
// This can work because Lua is single threaded.
static void save_state(lua_State *L) {

  // Load the states table.
      // stack = [<state_to_save>]
  load_states_table(L);
      // stack = [<state_to_save>, states_table]

  // Load the table for this demo_state (in current_state).
  lua_rawgeti(L, -1, current_state->ref);
      // stack = [<state_to_save>, states_table, demo_state_data]
  lua_remove(L, -2);
      // stack = [<state_to_save>, demo_state_data]

  // Save num_items using the key "num_items".
  int num_items = lua_gettop(L) - 1;
  assert(num_items >= 0);
  lua_pushnumber(L, num_items);
      // stack = [<state_to_save>, demo_state_data, num_items]
  lua_setfield(L, -2, "num_items");
      // stack = [<state_to_save>, demo_state_data]

  // Set each item by int keys, counting up from 1.
  for (int k = 1; k <= num_items; ++k) {
      // stack = [<state_to_save>, demo_state_data]
    lua_pushvalue(L, k);
      // stack = [<state_to_save>, demo_state_data, item[k]]
    lua_rawseti(L, -2, k);
  }

  // Clear the actual Lua stack.
  lua_settop(L, 0);
      // stack = []

  // Clear current_state.
  current_state = NULL;
}


// Functions that simulate the C API.

static int demo_luaL_newstate(lua_State *L) {
  print_stack(L);  // TEMP
      // stack = []
  FakeLuaState *demo_state =
      (FakeLuaState *)lua_newuserdata(L, sizeof(FakeLuaState));
      // stack = [demo_L]
  luaL_getmetatable(L, demo_state_metatable);
      // stack = [demo_L, mt]
  lua_setmetatable(L, 1);
      // stack = [demo_L]
  load_states_table(L);
      // stack = [demo_L, states_table]
  lua_newtable(L);
      // stack = [demo_L, states_table, demo_state_data = {}]
  lua_pushnumber(L, 0);
      // stack = [demo_L, states_table, demo_state_data, 0]
  lua_setfield(L, 3, "num_items");  // TODO Drop magic string.
      // stack = [demo_L, states_table, demo_state_data]
  demo_state->ref = luaL_ref(L, 2);  // Set states_table[ref] = demo_state_data.
      // stack = [demo_L, states_table]
  lua_pop(L, 1);
      // stack = [demo_L]
  return 1;  // Number of values to return that are on the stack.
}

static int demo_lua_pushnumber(lua_State *L) {

  // Read in inputs.
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  lua_Number n = luaL_checknumber(L, 2);

  // Simulate and print.
  load_state(L, demo_state);
  lua_pushnumber(L, n);
  print_stack(L);
  save_state(L);

  return 0;  // Number of values to return that are on the stack.
}

static int demo_lua_pushstring(lua_State *L) {

  // Read in inputs.
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  const char *s = luaL_checkstring(L, 2);

  // Simulate and print.
  load_state(L, demo_state);
  lua_pushstring(L, s);
  print_stack(L);
  save_state(L);

  return 0;  // Number of values to return that are on the stack.
}

static int demo_lua_insert(lua_State *L) {

  // Read in inputs.
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int i = luaL_checkint(L, 2);

  // Simulate and print.
  load_state(L, demo_state);
  lua_insert(L, i);
  print_stack(L);
  save_state(L);

  return 0;  // Number of values to return that are on the stack.
}

static int demo_lua_pop(lua_State *L) {

  // Read in inputs.
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int n = luaL_checkint(L, 2);

  // Simulate and print.
  load_state(L, demo_state);
  lua_pop(L, n);
  print_stack(L);
  save_state(L);

  return 0;  // Number of values to return that are on the stack.
}

static int demo_lua_getglobal(lua_State *L) {

  // Read in inputs.
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  const char *s = luaL_checkstring(L, 2);

  // Simulate and print.
  load_state(L, demo_state);
  lua_getglobal(L, s);
  print_stack(L);
  save_state(L);

  return 0;  // Number of values to return that are on the stack.
}

#define register_fn(lua_fn_name) \
  lua_register(L, #lua_fn_name, demo_ ## lua_fn_name)

static int setup_globals(lua_State *L) {
  printf("%s\n", __FUNCTION__);
  print_stack(L);  // TEMP
  register_fn(luaL_newstate);
  register_fn(lua_pushnumber);
  register_fn(lua_pushstring);
  register_fn(lua_insert);
  register_fn(lua_pop);
  register_fn(lua_getglobal);
  return 0;  // Number of values to return that are on the stack.
}

int luaopen_api_demo(lua_State *L) {

  // Set up the unique metatable for our userdata instances.
  // This table is empty and only used to verify that the userdata instances we
  // receive are valid.
      // stack = []
  luaL_newmetatable(L, demo_state_metatable);
      // stack = [mt = demo_state_metatable]
  lua_pop(L, 1);
      // stack = []

  // Register the public-facing Lua methods of our module.
  luaL_Reg fns[] = {
    {"setup_globals", setup_globals},
    {NULL, NULL}
  };
  luaL_register(L, "api_demo", fns);
      // stack = [api_demo]

  return 1;  // Number of Lua-facing return values on the Lua stack in L.
}
