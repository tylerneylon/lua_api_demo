// api_demo.c
//
// This is a Lua module written to simulate Lua's C API, except that it is run
// from within Lua itself. This is written for educational purposes.
//
// Implementation notes:
//
// This module maintains a table in the registry with the key
// "ApiDemo.SavedStates". The keys in this table are references controlled by
// luaL_{ref,unref}, and the values are used to save/load the Lua state being
// simulated.
//

#include "lua.h"
#include "lauxlib.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>


#define states_table_key     "ApiDemo.SavedStates"
#define demo_state_metatable "ApiDemo.LuaState"


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

static void print_stack(lua_State *L, int omit) {
  int n = lua_gettop(L) - omit;
  printf("stack:");
  for (int i = 1; i <= n; ++i) {
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
static void save_state(lua_State *L, int omit) {

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
  int num_items = lua_gettop(L) - 1 - omit;
  assert(num_items + omit >= 0);
  if (num_items < 0) num_items = 0;  // The omit value may have been high.
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
      // stack = [<state_to_save>, demo_state_data]
  lua_pop(L, 1);
      // stack = [<state_to_save>]

  // Clear current_state.
  current_state = NULL;
}


// Functions that simulate the C API.

static int demo_luaL_newstate(lua_State *L) {
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


// Macros that make it easy to wrap Lua C API functions.

#define fn_start(lua_fn_name)                                             \
  static int demo_ ## lua_fn_name(lua_State *L) {                         \
    FakeLuaState *demo_state =                                            \
        (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);

#define fn_end                        \
    print_stack(L, 0);                \
    save_state(L, 0);                 \
    return 0;                         \
  }

#define fn_end_number_out             \
    print_stack(L, 0);                \
    save_state(L, 0);                 \
    lua_pushnumber(L, out1);          \
    return 1;                         \
  }

#define fn_end_string_out             \
    print_stack(L, 0);                \
    save_state(L, 0);                 \
    if (out1) {                       \
      lua_pushstring(L, out1);        \
    } else {                          \
      lua_pushnumber(L, 0);           \
    }                                 \
    return 1;                         \
  }

#define fn_end_0_arg(lua_fn_name)     \
    load_state(L, demo_state);        \
    lua_fn_name(L);                   \
    fn_end

#define fn_end_1_arg(lua_fn_name)     \
    load_state(L, demo_state);        \
    lua_fn_name(L, arg1);             \
    fn_end

#define fn_end_2_arg(lua_fn_name)     \
    load_state(L, demo_state);        \
    lua_fn_name(L, arg1, arg2);       \
    fn_end

#define fn_end_1_arg_1_out(lua_fn_name)     \
    load_state(L, demo_state);              \
    int out1 = lua_fn_name(L, arg1);        \
    fn_end_number_out

#define fn_end_2_arg_1_out(lua_fn_name)     \
    load_state(L, demo_state);              \
    int out1 = lua_fn_name(L, arg1, arg2);  \
    fn_end_number_out

#define fn_end_1_arg_double_out(lua_fn_name)    \
    load_state(L, demo_state);                  \
    double out1 = lua_fn_name(L, arg1);         \
    fn_end_number_out

#define fn_end_1_arg_str_out(lua_fn_name)       \
    load_state(L, demo_state);                  \
    const char *out1 = lua_fn_name(L, arg1);    \
    fn_end_string_out

#define fn_end_0_arg_1_out(lua_fn_name)     \
    load_state(L, demo_state);              \
    int out1 = lua_fn_name(L);              \
    fn_end_number_out

#define fn_nothing_in(lua_fn_name)       \
    fn_start(lua_fn_name);               \
    fn_end_0_arg(lua_fn_name)

#define fn_nothing_in_int_out(lua_fn_name)     \
    fn_start(lua_fn_name);                     \
    fn_end_0_arg_1_out(lua_fn_name)

#define fn_int_in(lua_fn_name)       \
    fn_start(lua_fn_name);           \
    int arg1 = luaL_checkint(L, 2);  \
    fn_end_1_arg(lua_fn_name)

#define fn_string_in(lua_fn_name)                \
    fn_start(lua_fn_name);                       \
    const char *arg1 = luaL_checkstring(L, 2);   \
    fn_end_1_arg(lua_fn_name)

#define fn_int_string_in(lua_fn_name)            \
    fn_start(lua_fn_name);                       \
    int arg1 = luaL_checkint(L, 2);              \
    const char *arg2 = luaL_checkstring(L, 3);   \
    fn_end_2_arg(lua_fn_name)

#define fn_string_int_in(lua_fn_name)            \
    fn_start(lua_fn_name);                       \
    const char *arg1 = luaL_checkstring(L, 2);   \
    int arg2 = luaL_checkint(L, 3);              \
    fn_end_2_arg(lua_fn_name)

#define fn_int_int_in(lua_fn_name)               \
    fn_start(lua_fn_name);                       \
    int arg1 = luaL_checkint(L, 2);              \
    int arg2 = luaL_checkint(L, 3);              \
    fn_end_2_arg(lua_fn_name)

#define fn_number_in(lua_fn_name)                \
    fn_start(lua_fn_name);                       \
    lua_Number arg1 = luaL_checknumber(L, 2);    \
    fn_end_1_arg(lua_fn_name)

#define fn_int_in_int_out(lua_fn_name)    \
    fn_start(lua_fn_name);                \
    int arg1 = luaL_checkint(L, 2);       \
    fn_end_1_arg_1_out(lua_fn_name)

#define fn_int_in_double_out(lua_fn_name)   \
    fn_start(lua_fn_name);                  \
    int arg1 = luaL_checkint(L, 2);         \
    fn_end_1_arg_double_out(lua_fn_name)

#define fn_int_string_in_int_out(lua_fn_name)    \
    fn_start(lua_fn_name);                       \
    int arg1 = luaL_checkint(L, 2);              \
    const char *arg2 = luaL_checkstring(L, 3);   \
    fn_end_2_arg_1_out(lua_fn_name)

#define fn_string_in_int_out(lua_fn_name)        \
    fn_start(lua_fn_name);                       \
    const char *arg1 = luaL_checkstring(L, 2);   \
    fn_end_1_arg_1_out(lua_fn_name)

#define fn_int_in_string_out(lua_fn_name)    \
    fn_start(lua_fn_name);                   \
    int arg1 = luaL_checkint(L, 2);          \
    fn_end_1_arg_str_out(lua_fn_name)


// Wrappers around C API functions defined using the above macros.

// Please keep these alphabetized by API function name.
fn_int_int_in          (lua_call);
fn_int_in_int_out      (lua_checkstack);
fn_int_in              (lua_concat);
fn_int_int_in          (lua_equal);
fn_int_string_in       (lua_getfield);
fn_string_in           (lua_getglobal);
fn_int_in_int_out      (lua_getmetatable);
fn_int_in              (lua_gettable);
fn_nothing_in_int_out  (lua_gettop);
// Defined below:       lua_error
fn_int_in              (lua_insert);
fn_int_in_int_out      (lua_isboolean);
fn_int_in_int_out      (lua_isfunction);
fn_int_in_int_out      (lua_isnil);
fn_int_in_int_out      (lua_isnone);
fn_int_in_int_out      (lua_isnoneornil);
fn_int_in_int_out      (lua_isnumber);
fn_int_in_int_out      (lua_isstring);
fn_int_in_int_out      (lua_istable);
fn_int_int_in          (lua_lessthan);
fn_nothing_in          (lua_newtable);
fn_int_in_int_out      (lua_next);
fn_int_in_int_out      (lua_objlen);
fn_int_in              (lua_pop);
fn_int_in              (lua_pushboolean);
fn_string_int_in       (lua_pushlstring);
fn_nothing_in          (lua_pushnil);
fn_number_in           (lua_pushnumber);
fn_string_in           (lua_pushstring);
fn_int_in              (lua_pushvalue);
fn_int_int_in          (lua_rawequal);
fn_int_in              (lua_rawget);
fn_int_int_in          (lua_rawgeti);
fn_int_in              (lua_rawset);
fn_int_int_in          (lua_rawseti);
fn_int_in              (lua_remove);
fn_int_in              (lua_replace);
fn_int_string_in       (lua_setfield);
fn_string_in           (lua_setglobal);
fn_int_in_int_out      (lua_setmetatable);
fn_int_in              (lua_settable);
fn_int_in              (lua_settop);
fn_int_in_int_out      (lua_toboolean);
fn_int_in_int_out      (lua_tointeger);
fn_int_in_double_out   (lua_tonumber);
fn_int_in_string_out   (lua_tostring);
fn_int_in_int_out      (lua_type);
fn_int_in_string_out   (lua_typename);

fn_int_string_in_int_out  (luaL_argerror);
fn_int_string_in_int_out  (luaL_callmeta);
fn_int_in                 (luaL_checkany);
fn_int_in_int_out         (luaL_checkint);
fn_int_in_double_out      (luaL_checknumber);
fn_int_in_string_out      (luaL_checkstring);
fn_int_int_in             (luaL_checktype);
fn_string_in_int_out      (luaL_dofile);
fn_string_in_int_out      (luaL_dostring);
fn_int_string_in_int_out  (luaL_getmetafield);
fn_string_in_int_out      (luaL_loadfile);
fn_string_in_int_out      (luaL_loadstring);
// Defined below:          luaL_optint
// Defined below:          luaL_optnumber
// Defined below:          luaL_optstring
fn_int_in_string_out      (luaL_typename);
fn_int_string_in_int_out  (luaL_typerror);


// Function wrappers that need special-case code.

// This is a special case function as it doesn't return; yet we'd still like to
// leave in a valid state as the encompassing Lua environment may continue to
// run.
int demo_lua_error(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  load_state(L, demo_state);
  print_stack(L, 1);  // 1 --> tail values to omit
  save_state(L, 1);   // 1 --> tail values to omit
  return lua_error(L);
}

int demo_lua_tolstring(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int arg1 = luaL_checkint(L, 2);
  load_state(L, demo_state);
  const char *out1 = lua_tolstring(L, arg1, NULL);  // NULL --> *len
  print_stack(L, 0);  // 0 --> tail values to omit
  save_state(L, 0);   // 0 --> tail values to omit
  lua_pushstring(L, out1);
  return 1;  // Number of values to return that are on the stack.
}

int demo_luaL_optint(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int arg1 = luaL_checkint(L, 2);
  int arg2 = luaL_checkint(L, 3);
  load_state(L, demo_state);
  int out1 = luaL_optint(L, arg1, arg2);
  print_stack(L, 0);  // 0 --> tail values to omit
  save_state(L, 0);   // 0 --> tail values to omit
  lua_pushnumber(L, out1);
  return 1;  // Number of values to return that are on the stack.
}

int demo_luaL_optnumber(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int arg1 = luaL_checkint(L, 2);
  double arg2 = luaL_checknumber(L, 3);
  load_state(L, demo_state);
  double out1 = luaL_optnumber(L, arg1, arg2);
  print_stack(L, 0);  // 0 --> tail values to omit
  save_state(L, 0);   // 0 --> tail values to omit
  lua_pushnumber(L, out1);
  return 1;  // Number of values to return that are on the stack.
}

int demo_luaL_optstring(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int arg1 = luaL_checkint(L, 2);
  const char *arg2 = luaL_checkstring(L, 3);
  load_state(L, demo_state);
  const char *out1 = luaL_optstring(L, arg1, arg2);
  print_stack(L, 0);  // 0 --> tail values to omit
  save_state(L, 0);   // 0 --> tail values to omit
  lua_pushstring(L, out1);
  return 1;  // Number of values to return that are on the stack.
}

int demo_lua_pcall(lua_State *L) {
  FakeLuaState *demo_state =
      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
  int arg1 = luaL_checkint(L, 2);
  int arg2 = luaL_checkint(L, 3);
  int arg3 = luaL_checkint(L, 4);
  load_state(L, demo_state);
  int out1 = lua_pcall(L, arg1, arg2, arg3);
  print_stack(L, 0);  // 0 --> tail values to omit
  save_state(L, 0);   // 0 --> tail values to omit
  lua_pushnumber(L, out1);
  return 1;  // Number of values to return that are on the stack.
}

// setup_globals is a single Lua-facing function to register all our C-API-like
// functions in a single go.

#define register_fn(lua_fn_name) \
  lua_register(L, #lua_fn_name, demo_ ## lua_fn_name)

#define register_const(const_name)             \
  lua_pushnumber(L, (lua_Number)const_name);   \
  lua_setglobal(L, #const_name);

static int setup_globals(lua_State *L) {
  register_fn(luaL_newstate);

  // Please keep these alphabetized.
  register_fn(lua_call);
  register_fn(lua_checkstack);
  register_fn(lua_concat);
  register_fn(lua_equal);
  register_fn(lua_getfield);
  register_fn(lua_getglobal);
  register_fn(lua_getmetatable);
  register_fn(lua_gettable);
  register_fn(lua_gettop);
  register_fn(lua_error);
  register_fn(lua_insert);
  register_fn(lua_isboolean);
  register_fn(lua_isfunction);
  register_fn(lua_isnil);
  register_fn(lua_isnone);
  register_fn(lua_isnoneornil);
  register_fn(lua_isnumber);
  register_fn(lua_isstring);
  register_fn(lua_istable);
  register_fn(lua_lessthan);
  register_fn(lua_newtable);
  register_fn(lua_next);
  register_fn(lua_objlen);
  register_fn(lua_pcall);
  register_fn(lua_pop);
  register_fn(lua_pushboolean);
  register_fn(lua_pushlstring);
  register_fn(lua_pushnil);
  register_fn(lua_pushnumber);
  register_fn(lua_pushstring);
  register_fn(lua_pushvalue);
  register_fn(lua_rawequal);
  register_fn(lua_rawget);
  register_fn(lua_rawgeti);
  register_fn(lua_rawset);
  register_fn(lua_rawseti);
  register_fn(lua_remove);
  register_fn(lua_replace);
  register_fn(lua_setfield);
  register_fn(lua_setglobal);
  register_fn(lua_setmetatable);
  register_fn(lua_settable);
  register_fn(lua_settop);
  register_fn(lua_toboolean);
  register_fn(lua_tointeger);
  register_fn(lua_tolstring);
  register_fn(lua_tonumber);
  register_fn(lua_tostring);
  register_fn(lua_type);
  register_fn(lua_typename);

  register_fn(luaL_argerror);
  register_fn(luaL_callmeta);
  register_fn(luaL_checkany);
  register_fn(luaL_checkint);
  register_fn(luaL_checknumber);
  register_fn(luaL_checkstring);
  register_fn(luaL_checktype);
  register_fn(luaL_dofile);
  register_fn(luaL_dostring);
  register_fn(luaL_getmetafield);
  register_fn(luaL_loadfile);
  register_fn(luaL_loadstring);
  register_fn(luaL_optint);
  register_fn(luaL_optnumber);
  register_fn(luaL_optstring);
  register_fn(luaL_typename);
  register_fn(luaL_typerror);

  // Set up C-like constants.
  lua_pushnumber(L, 0);
  lua_setglobal(L, "NULL");

  register_const(LUA_ERRRUN);
  register_const(LUA_ERRSYNTAX);
  register_const(LUA_ERRMEM);
  register_const(LUA_ERRERR);
  register_const(LUA_ERRFILE);

  register_const(LUA_TNONE);
  register_const(LUA_TNIL);
  register_const(LUA_TBOOLEAN);
  register_const(LUA_TLIGHTUSERDATA);
  register_const(LUA_TNUMBER);
  register_const(LUA_TSTRING);
  register_const(LUA_TTABLE);
  register_const(LUA_TFUNCTION);
  register_const(LUA_TUSERDATA);
  register_const(LUA_TTHREAD);

  register_const(LUA_REGISTRYINDEX);
  register_const(LUA_GLOBALSINDEX);

  register_const(LUA_MULTRET);

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
