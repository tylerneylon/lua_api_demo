// apidemo.c
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

#include <lua.h>
#include <lauxlib.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define states_table_key     "ApiDemo.SavedStates"
#define demo_state_metatable "ApiDemo.LuaState"


// # The help string.

const char *help_string =
  "                                                                         \n"
  "-- writing values to the stack ----------------------------------------- \n"
  "                                                                         \n"
  "     lua_pushboolean(L, int)                                [-0 +1 -]    \n"
  " --- lua_pushfstring(L, str, ...)                           [-0 +1 m]    \n"
  " --- lua_pushinteger(L, lua_Integer)                        [-0 +1 -]    \n"
  "     lua_pushlstring(L, str, size_t)                        [-0 +1 m]    \n"
  "     lua_pushnil(L)                                         [-0 +1 -]    \n"
  "     lua_pushnumber(L, lua_Number)                          [-0 +1 -]    \n"
  "     lua_pushstring(L, str)                                 [-0 +1 m]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- stack manipulation -------------------------------------------------- \n"
  "                                                                         \n"
  " int lua_checkstack(L, int)           ensure stack capacity [-0 +0 m]    \n"
  " int lua_gettop(L)                    get stack size        [-0 +0 -]    \n"
  "     lua_insert(L, int i)             mv top -> i           [-1 +1 -]    \n"
  "     lua_pushvalue(L, int i)          cp i -> top           [-0 +1 m]    \n"
  "     lua_remove(L, int i)             rm i                  [-1 +0 -]    \n"
  "     lua_replace(L, int i)            rm i, mv top -> i     [-1 +0 -]    \n"
  "     lua_settop(L, int)               set stack size        [-? +? -]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- reading values from the stack --------------------------------------- \n"
  "                                                                         \n"
  " int lua_isboolean(L, int i)          is stack[i] a bool?   [-0 +0 -]    \n"
  " int lua_isfunction(L, int i)         is stack[i] a fn?     [-0 +0 -]    \n"
  " int lua_isnil(L, int i)              is stack[i] a nil?    [-0 +0 -]    \n"
  " int lua_isnone(L, int i)             nothing at stack[i]?  [-0 +0 -]    \n"
  " int lua_isnumber(L, int i)           is stack[i] a number? [-0 +0 -]    \n"
  " int lua_isstring(L, int i)           is stack[i] a string? [-0 +0 -]    \n"
  " int lua_istable(L, int i)            is stack[i] a table?  [-0 +0 -]    \n"
  " --- lua_isuserdata(L, int i)         is stack[i] a udata?  [-0 +0 -]    \n"
  "                                                                         \n"
  " int lua_toboolean(L, int i)          bool(stack[i])        [-0 +0 -]    \n"
  " l_I lua_tointeger(L, int i)          lua_Integer(stack[i]) [-0 +0 -]    \n"
  " str lua_tolstring(L, int, size_t *)  mem is owned by Lua   [-0 +0 -]    \n"
  " l_N lua_tonumber(L, int i)           lua_Number(stack[i])  [-0 +0 -]    \n"
  " str lua_tostring(L, int i)           mem is owned by Lua   [-0 +0 -]    \n"
  " --- lua_touserdata(L, int i)         returns void *        [-0 +0 -]    \n"
  "                                                                         \n"
  " int lua_type(L, int i)               LUA_T{NIL,TABLE,etc}  [-0 +0 -]    \n"
  " str lua_typename(L, int tp)          LUA_T{NIL,etc}->name  [-0 +0 -]    \n"
  " str luaL_typename(L, int i)          typename(stack[i]))   [-0 +0 -]    \n"
  "                                                                         \n"
  " int luaL_optint(L, int n, int d)     int(stack[n]) or d    [-0 +0 v]    \n"
  " --- luaL_optinteger(L, int n, l_I d) l_I(stack[n]) or d    [-0 +0 v]    \n"
  " --- luaL_optlong(L, int n, long d)   long(stack[n]) or d   [-0 +0 v]    \n"
  " l_N luaL_optnumber(L, int n, l_N d)  l_N(stack[n]) or d    [-0 +0 v]    \n"
  " str luaL_optstring(L, int n, str d)  str(stack[n]) or d    [-0 +0 v]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- table operations ---------------------------------------------------- \n"
  "                                                                         \n"
  "     lua_newtable(L)                  pushes {}             [-0 +1 m]    \n"
  " --- lua_createtable(L, int m, int n) m,n=arr,rec capacity  [-0 +1 m]    \n"
  "                                                                         \n"
  "     lua_settable(L, int i)           pops k,v; stk[i][k]=v [-2 +0 e]    \n"
  "     lua_setfield(L, int i, str k)    pops v; stk[i][k]=v   [-1 +0 e]    \n"
  "     lua_rawset(L, int i)             settable,no metacalls [-2 +0 e]    \n"
  "     lua_rawseti(L, int i, int n)     stk[i][n]=pop'd;no mt [-1 +0 e]    \n"
  "                                                                         \n"
  "     lua_gettable(L, int i)           pop k; push stk[i][k] [-1 +1 e]    \n"
  "     lua_getfield(L, int i, str k)    push stk[i][k]        [-0 +1 e]    \n"
  "     lua_rawget(L, int i)             gettable,no metacalls [-1 +1 -]    \n"
  "     lua_rawgeti(L, int i, int n)     push stk[i][n];no mt  [-0 +1 -]    \n"
  "                                                                         \n"
  " int lua_setmetatable(L, int i)       pop mt; mt(stk[i])=mt [-1 +0 -]    \n"
  " int lua_getmetatable(L, int i)       push mt(stk[i])if any [-1 +0|1 -]  \n"
  "                                                                         \n"
  " int lua_next(L, int i)               pop k/push k,v if any [-1 +0|2 e]  \n"
  " szt lua_objlen(L, int i)  Lua 5.1    #stk[i], assuming seq [-0 +0 -]    \n"
  " szt lua_rawlen(L, int i)  Lua 5.2+   #stk[i], assuming seq [-0 +0 -]    \n"
  "                                                                         \n"
  "     lua_setglobal(L, str name)       pops v; _G[name]=v    [-1 +0 e]    \n"
  "     lua_getglobal(L, str name)       pushes _G[name]       [-0 +1 e]    \n"
  "                                                                         \n"
  " int luaL_getmetafield(L, int i, str) +mt(stk[i])[s] if any [-1 +0|1 e]  \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- basic operators ----------------------------------------------------- \n"
  "                                                                         \n"
  "     lua_concat(L, int n)             str cat top n vals    [-n +1 e]    \n"
  " int lua_equal(L, int i, int j)       1 if stk[i] == stk[j] [-0 +0 e]    \n"
  " int lua_lessthan(L, int i, int j)    1 if stk[i] < stk[j]  [-0 +0 e]    \n"
  " int lua_rawequal(L, int i, int j)    equal?; no metacalls  [-0 +0 -]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- function calls ------------------------------------------------------ \n"
  "                                                                         \n"
  " --- lua_atpanic(L, lua_CFunciton f)  set panic fn; ret old [-0 +0 -]    \n"
  "     lua_call(L, int m, int n)        -/call f(m-args); +n  [-(m+1) +n e]\n"
  " int lua_pcall(L, int m, n, e)        call w/ errfn=stk[e]  [-(m+1)      \n"
  "                                      err:push msg; ret!=0        +n|1 e]\n"
  " --- lua_cpcall(L, l_CFn f, void *ud) call f(ud);+1 if err  [-0 +0|1 -]  \n"
  " int luaL_callmeta(L, int o, str s)   mt(stk[o])[s] if any  [-0 +0|1 e]  \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- error handling ------------------------------------------------------ \n"
  "                                                                         \n"
  " int lua_error(L)                     pop errmsg; throw it  [-1 +0 v]    \n"
  " --- luaL_error(L, str fmt, ...)      throw errmsg fmt      [-0 +0 v]    \n"
  "                                                                         \n"
  "     luaL_checkany(L, int n)          err if stk[n] is none [-0 +0 v]    \n"
  " int luaL_checkint(L, int n)          int(stk[n]) or err    [-0 +0 v]    \n"
  " --- luaL_checkinteger(L, int n)      l_I(stk[n]) or err    [-0 +0 v]    \n"
  " --- luaL_checklong(L, int n)         lng(stk[n]) or err    [-0 +0 v]    \n"
  " --- luaL_checklstring(L, n, szt *l)  str(stk[n]) or err    [-0 +0 v]    \n"
  " l_N luaL_checknumber(L, int n)       l_N(stk[n]) or err    [-0 +0 v]    \n"
  " str luaL_checkstring(L, int n)       str(stk[n]) or err    [-0 +0 v]    \n"
  "                                                                         \n"
  "     luaL_checktype(L, int n, int tp) err if tp(stk[n])!=tp [-0 +0 v]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- running Lua code ---------------------------------------------------- \n"
  "                                                                         \n"
  " --- lua_load(L, lua_Reader, void*, str) loads code;push fn [-0 +1 -]    \n"
  "                                      err:ret non0/push msg              \n"
  "                                                                         \n"
  " int luaL_loadfile(L, str filename)   loadfile; push as fn  [-0 +1 m]    \n"
  " int luaL_loadstring(L, str code)     load code; push as fn [-0 +1 m]    \n"
  "                                                                         \n"
  " int luaL_dofile(L, str filename)     load and run file     [-0 +? m]    \n"
  " int luaL_dostring(L, str code)       load and run code     [-0 +? m]    \n"
  "                                                                         \n"
  "                                                                         \n"
  "-- key ----------------------------------------------------------------- \n"
  "                                                                         \n"
  " Notation [-a +b X] means: 1st pops last a values, then pushes b values  \n"
  "                           X = -   never throws an error                 \n"
  "                           X = m   may throw a memory error              \n"
  "                           X = v   may throw an error by request         \n"
  "                           X = e   may throw any error                   \n"
  "                                                                         \n"
  " Notation [-a +b|c X] means pushes b values if retval is 0; c otherwise  \n"
  "                                                                         \n"
  "   --- before a function means it's not implemented in this demo api     \n"
  "                                                                         \n"
  " Abbreviations:                                                          \n"
  "   str = const char *                              szt = size_t          \n"
  "   l_I = lua_Integer (often int32 or int64)        stk = stack           \n"
  "   l_N = lua_Number  (often double)                 tp = type            \n"
  "                                                                         \n";


// # Internal typedefs.

typedef struct {
  int ref;
} FakeLuaState;


// # Internal globals, besides the help string.

static FakeLuaState *current_state;


// # Internal functions.

// ## Functions used to print the stack.

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

  int k;
  for (k = 1;; ++k) {
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

static char *get_fn_string(lua_State *L, int i) {
  static char fn_name[1024];

  // Ensure i is an absolute index as we'll be pushing/popping things after it.
  if (i < 0) i = lua_gettop(L) + i + 1;

  // Check to see if the function has a global name.
      // stack = [..]
  lua_getglobal(L, "_G");
      // stack = [.., _G]
  lua_pushnil(L);
      // stack = [.., _G, nil]
  while (lua_next(L, -2)) {
      // stack = [.., _G, key, value]
    if (lua_rawequal(L, i, -1)) {
      snprintf(fn_name, 1024, "function:%s", lua_tostring(L, -2));
      lua_pop(L, 3);
      // stack = [..]
      return fn_name;
    }
      // stack = [.., _G, key, value]
    lua_pop(L, 1);
      // stack = [.., _G, key]
  }
  // If we get here, the function didn't have a global name; print a pointer.
      // stack = [.., _G]
  lua_pop(L, 1);
      // stack = [..]
  snprintf(fn_name, 1024, "function:%p", lua_topointer(L, i));
  return fn_name;
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
      printf("%s%s%s", first, get_fn_string(L, i), last);
      return;

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
  int i;
  for (i = 1; i <= n; ++i) {
    printf(" ");
    print_item(L, i, 0);  // 0 --> as_key
  }
  if (n == 0) printf(" <empty>");
  printf("\n");
}


// ## Functions for loading and saving demo Lua states.

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
  int k;
  for (k = 1; k <= num_items; ++k) {
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
  int k;
  for (k = 1; k <= num_items; ++k) {
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

// ## Functions that simulate the C API.

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

// ### Macros to work with luaL_checkint and luaL_optint in Lua 5.3.

#if LUA_VERSION_NUM == 503
#define luaL_checkint(L, arg) (int)(luaL_checkinteger(L, arg))
#define luaL_optint(L, arg, d) (int)(luaL_optinteger(L, arg, d))
#endif

// ### Macros that make it easy to wrap Lua C API functions.

// A typical wrapper function will look something like this pseudocode:
//
// static int demo_lua_dosomething(lua_State *L) {
//  FakeLuaState *demo_state =
//      (FakeLuaState *)luaL_checkudata(L, 1, demo_state_metatable);
//  intype1 arg1 = luaL_checktype1(L, 2);
//  intype2 arg2 = luaL_checktype2(L, 3);
//  load_state(L, demo_state);
//  outtype out = lua_dosomething(L, arg1, arg2);
//  print_stack(L, 0);  // 0 --> omit 0 tail values
//  save_stack(L, 0);   // 0 --> omit 0 tail values
//  lua_pushouttype(L, out);
//  return 1;
// }
//
// In more natural language, the process works like this:
// 1. Extract inputs from L.
// 2. Load demo Lua state.
// 3. Run the API function.
// 4. Print the stack.
// 5. Save the demo Lua state.
// 6. Return any output values.
//
// Most of that code is identical for most API functions. The major differences
// are the types and numbers of the inputs and outputs. The macros below help
// simplify the creation of these wrapper functions. As an example, the API
// functions lua_pushstring can be wrapped with this simple macro call:
//
// fn_string_in(lua_pushstring);
//
// In general, the final macros take the format:
//
// fn_<intype1>_<intype2>_in[_<outtype>_out] (lua_fn_name);
//

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

// ### Wrappers around C API functions defined using the above macros.

// Please keep these alphabetized by API function name.
fn_int_int_in          (lua_call);
fn_int_in_int_out      (lua_checkstack);
fn_int_in              (lua_concat);
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
fn_nothing_in          (lua_newtable);
fn_int_in_int_out      (lua_next);
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

// Version-specific functions.

#if LUA_VERSION_NUM == 501
fn_int_int_in          (lua_equal);
fn_int_int_in          (lua_lessthan);
fn_int_in_int_out      (lua_objlen);
#else
fn_int_in_int_out      (lua_rawlen);
#endif

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

// ### Function wrappers that need special-case code.

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

// ### Define setup_globals.

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
  register_fn(lua_newtable);
  register_fn(lua_next);
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

// Version-specific functions.

#if LUA_VERSION_NUM == 501
  register_fn(lua_equal);
  register_fn(lua_lessthan);
  register_fn(lua_objlen);
#else
  register_fn(lua_rawlen);
#endif

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
#if LUA_VERSION_NUM == 501
  register_const(LUA_GLOBALSINDEX);
#endif

  register_const(LUA_MULTRET);

  return 0;  // Number of values to return that are on the stack.
}

int show_help(lua_State *L) {
  printf("%s", help_string);
  return 0;
}

// ## The main entry point, and only directly public-facing function.

int luaopen_apidemo(lua_State *L) {

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
    {"help",          show_help},
    {NULL, NULL}
  };
#if LUA_VERSION_NUM == 501
  luaL_register(L, "apidemo", fns);
#else
  luaL_newlib(L, fns);
#endif
      // stack = [apidemo]

  return 1;  // Number of Lua-facing return values on the Lua stack in L.
}
