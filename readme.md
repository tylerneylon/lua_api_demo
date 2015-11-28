# Lua API demo

This is a Lua module that can help you learn how use Lua's C API.

It is run from Lua, but purposefully mimics C style, usage, and behavior.
It's meant to be run from the Lua interpreter to illustrate the effects of each
API call as it's made. It will also work from a standard Lua script.

## Using the module

Below is an example interpreter usage, given with both the user-typed input and
the printed output. Ending semicolons in Lua are optional and usually omitted;
however, I'm using them here to keep the C-like lines more C-like.

    > api_demo = require 'api_demo'
    > api_demo.setup_globals()
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

## Building the module

So far this module has only been tested on Mac OS X.

This module must be compiled before it can be used. Run the `make` shell command
to build the file `api_demo.so`. A Lua interpreter or script run in the same
directory will find this file when the standard `require` function is used. For
global access, place this file, or a symlink to it, in any directory listed in
your Lua's `package.cpath`. For example, if you're running Lua 5.3, this would
work when run from this repo's directory:

    $ make
    $ sudo ln -s `pwd`/api_demo.so /usr/local/lib/lua/5.3/
