# Lua API demo

This is a Lua module that can help you learn how use Lua's C API.

It is run from Lua, but purposefully mimics C style, usage, and behavior.
It's meant to be run from the Lua interpreter to illustrate the effects of each
API call as it's made. It will also work from a standard Lua script.

## Using the module

Below is an example interpreter usage, given with both the user-typed input and
the printed output. Ending semicolons in Lua are optional and usually omitted;
however, I'm using them here to keep the C-like lines more C-like.

    > apidemo = require 'apidemo'
    > apidemo.setup_globals()
    > L = luaL_newstate()
    > lua_pushnumber(L, 42);
    stack: 42
    > lua_getglobal(L, "print");
    stack: 42 function:0x7fb178e013d0
    > lua_pushstring(L, "hello from the api!");
    stack: 42 function:0x7fb178e013d0 'hello from the api!'
    > lua_call(L, 1, 0);
    hello from the api!
    stack: 42

## Installing and building the module

This module has been tested on Mac OS X and Ubuntu.

### Using luarocks

If you use [luarocks](https://luarocks.org/), you can install the module by
running:

    $ sudo luarocks install apidemo

### Using make

If you don't use luarocks, you can manually build and install, although the
current Makefile has only been tested on Mac OS X.
Run the `make` shell command
to build the file `apidemo.so`. A Lua interpreter or script run in the same
directory will find this file when the standard `require` function is used. For
global access, place this file, or a symlink to it, in any directory listed in
your Lua's `package.cpath`. For example, if you're running Lua 5.3, the
following shell commands will work when run from this repo's directory:

    $ make
    $ sudo ln -s `pwd`/apidemo.so /usr/local/lib/lua/5.3/

## API quick reference

This module contains a help string that documents the behavior of both this
module and of a large subset of Lua's C API.

```
-- writing values to the stack ----------------------------------------- 
                                                                         
     lua_pushboolean(L, int)                                [-0 +1 -]    
 --- lua_pushfstring(L, str, ...)                           [-0 +1 m]    
 --- lua_pushinteger(L, lua_Integer)                        [-0 +1 -]    
     lua_pushlstring(L, str, size_t)                        [-0 +1 m]    
     lua_pushnil(L)                                         [-0 +1 -]    
     lua_pushnumber(L, lua_Number)                          [-0 +1 -]    
     lua_pushstring(L, str)                                 [-0 +1 m]    
                                                                         
                                                                         
-- stack manipulation -------------------------------------------------- 
                                                                         
 int lua_checkstack(L, int)           ensure stack capacity [-0 +0 m]    
 int lua_gettop(L)                    get stack size        [-0 +0 -]    
     lua_insert(L, int i)             mv top -> i           [-1 +1 -]    
     lua_pushvalue(L, int i)          cp i -> top           [-0 +1 m]    
     lua_remove(L, int i)             rm i                  [-1 +0 -]    
     lua_replace(L, int i)            rm i, mv top -> i     [-1 +0 -]    
     lua_settop(L, int)               set stack size        [-? +? -]    
                                                                         
                                                                         
-- reading values from the stack --------------------------------------- 
                                                                         
 int lua_isboolean(L, int i)          is stack[i] a bool?   [-0 +0 -]    
 int lua_isfunction(L, int i)         is stack[i] a fn?     [-0 +0 -]    
 int lua_isnil(L, int i)              is stack[i] a nil?    [-0 +0 -]    
 int lua_isnone(L, int i)             nothing at stack[i]?  [-0 +0 -]    
 int lua_isnumber(L, int i)           is stack[i] a number? [-0 +0 -]    
 int lua_isstring(L, int i)           is stack[i] a string? [-0 +0 -]    
 int lua_istable(L, int i)            is stack[i] a table?  [-0 +0 -]    
 --- lua_isuserdata(L, int i)         is stack[i] a udata?  [-0 +0 -]    
                                                                         
 int lua_toboolean(L, int i)          bool(stack[i])        [-0 +0 -]    
 l_I lua_tointeger(L, int i)          lua_Integer(stack[i]) [-0 +0 -]    
 str lua_tolstring(L, int, size_t *)  mem is owned by Lua   [-0 +0 -]    
 l_N lua_tonumber(L, int i)           lua_Number(stack[i])  [-0 +0 -]    
 str lua_tostring(L, int i)           mem is owned by Lua   [-0 +0 -]    
 --- lua_touserdata(L, int i)         returns void *        [-0 +0 -]    
                                                                         
 int lua_type(L, int i)               LUA_T{NIL,TABLE,etc}  [-0 +0 -]    
 str lua_typename(L, int tp)          LUA_T{NIL,etc}->name  [-0 +0 -]    
 str luaL_typename(L, int i)          typename(stack[i]))   [-0 +0 -]    
                                                                         
 int luaL_optint(L, int n, int d)     int(stack[n]) or d    [-0 +0 v]    
 --- luaL_optinteger(L, int n, l_I d) l_I(stack[n]) or d    [-0 +0 v]    
 --- luaL_optlong(L, int n, long d)   long(stack[n]) or d   [-0 +0 v]    
 l_N luaL_optnumber(L, int n, l_N d)  l_N(stack[n]) or d    [-0 +0 v]    
 str luaL_optstring(L, int n, str d)  str(stack[n]) or d    [-0 +0 v]    
                                                                         
                                                                         
-- table operations ---------------------------------------------------- 
                                                                         
     lua_newtable(L)                  pushes {}             [-0 +1 m]    
 --- lua_createtable(L, int m, int n) m,n=arr,rec capacity  [-0 +1 m]    
                                                                         
     lua_settable(L, int i)           pops k,v; stk[i][k]=v [-2 +0 e]    
     lua_setfield(L, int i, str k)    pops v; stk[i][k]=v   [-1 +0 e]    
     lua_rawset(L, int i)             settable,no metacalls [-2 +0 e]    
     lua_rawseti(L, int i, int n)     stk[i][n]=pop'd;no mt [-1 +0 e]    
                                                                         
     lua_gettable(L, int i)           pop k; push stk[i][k] [-1 +1 e]    
     lua_getfield(L, int i, str k)    push stk[i][k]        [-0 +1 e]    
     lua_rawget(L, int i)             gettable,no metacalls [-1 +1 -]    
     lua_rawgeti(L, int i, int n)     push stk[i][n];no mt  [-0 +1 -]    
                                                                         
 int lua_setmetatable(L, int i)       pop mt; mt(stk[i])=mt [-1 +0 -]    
 int lua_getmetatable(L, int i)       push mt(stk[i])if any [-1 +0|1 -]  
                                                                         
 int lua_next(L, int i)               pop k/push k,v if any [-1 +0|2 e]  
 szt lua_objlen(L, int i)             #stk[i], assuming seq [-0 +0 -]    
                                                                         
     lua_setglobal(L, str name)       pops v; _G[name]=v    [-1 +0 e]    
     lua_getglobal(L, str name)       pushes _G[name]       [-0 +1 e]    
                                                                         
 int luaL_getmetafield(L, int i, str) +mt(stk[i])[s] if any [-1 +0|1 e]  
                                                                         
                                                                         
-- basic operators ----------------------------------------------------- 
                                                                         
     lua_concat(L, int n)             str cat top n vals    [-n +1 e]    
 int lua_equal(L, int i, int j)       1 if stk[i] == stk[j] [-0 +0 e]    
 int lua_lessthan(L, int i, int j)    1 if stk[i] < stk[j]  [-0 +0 e]    
 int lua_rawequal(L, int i, int j)    equal?; no metacalls  [-0 +0 -]    
                                                                         
                                                                         
-- function calls ------------------------------------------------------ 
                                                                         
 --- lua_atpanic(L, lua_CFunciton f)  set panic fn; ret old [-0 +0 -]    
     lua_call(L, int m, int n)        -/call f(m-args); +n  [-(m+1) +n e]
 int lua_pcall(L, int m, n, e)        call w/ errfn=stk[e]  [-(m+1)      
                                      err:push msg; ret!=0        +n|1 e]
 --- lua_cpcall(L, l_CFn f, void *ud) call f(ud);+1 if err  [-0 +0|1 -]  
 int luaL_callmeta(L, int o, str s)   mt(stk[o])[s] if any  [-0 +0|1 e]  
                                                                         
                                                                         
-- error handling ------------------------------------------------------ 
                                                                         
 int lua_error(L)                     pop errmsg; throw it  [-1 +0 v]    
 --- luaL_error(L, str fmt, ...)      throw errmsg fmt      [-0 +0 v]    
                                                                         
     luaL_checkany(L, int n)          err if stk[n] is none [-0 +0 v]    
 int luaL_checkint(L, int n)          int(stk[n]) or err    [-0 +0 v]    
 --- luaL_checkinteger(L, int n)      l_I(stk[n]) or err    [-0 +0 v]    
 --- luaL_checklong(L, int n)         lng(stk[n]) or err    [-0 +0 v]    
 --- luaL_checklstring(L, n, szt *l)  str(stk[n]) or err    [-0 +0 v]    
 l_N luaL_checknumber(L, int n)       l_N(stk[n]) or err    [-0 +0 v]    
 str luaL_checkstring(L, int n)       str(stk[n]) or err    [-0 +0 v]    
                                                                         
     luaL_checktype(L, int n, int tp) err if tp(stk[n])!=tp [-0 +0 v]    
 int luaL_typerror(L, int n, str msg) err;msg=expected type [-0 +0 v]    
                                                                         
                                                                         
-- running Lua code ---------------------------------------------------- 
                                                                         
 --- lua_load(L, lua_Reader, void*, str) loads code;push fn [-0 +1 -]    
                                      err:ret non0/push msg              
                                                                         
 int luaL_loadfile(L, str filename)   loadfile; push as fn  [-0 +1 m]    
 int luaL_loadstring(L, str code)     load code; push as fn [-0 +1 m]    
                                                                         
 int luaL_dofile(L, str filename)     load and run file     [-0 +? m]    
 int luaL_dostring(L, str code)       load and run code     [-0 +? m]    
                                                                         
                                                                         
-- key ----------------------------------------------------------------- 
                                                                         
 Notation [-a +b X] means: 1st pops last a values, then pushes b values  
                           X = -   never throws an error                 
                           X = m   may throw a memory error              
                           X = v   may throw an error by request         
                           X = e   may throw any error                   
                                                                         
 Notation [-a +b|c X] means pushes b values if retval is 0; c otherwise  
                                                                         
   --- before a function means it's not implemented in this demo api     
                                                                         
 Abbreviations:                                                          
   str = const char *                              szt = size_t          
   l_I = lua_Integer (often int32 or int64)        stk = stack           
   l_N = lua_Number  (often double)                 tp = type        
```
