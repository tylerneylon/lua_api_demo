--[[

demo_run.lua

This file is intended to demonstrate how to use the api_demo module.

--]]


-- Setup.
local api_demo = require 'api_demo'
api_demo.setup_globals()
L = luaL_newstate()

-- We'll use semicolons below to make the code more closely resemble C.

-- How to push values onto the stack.
lua_pushnumber(L, 42);
lua_pushstring(L, "hello!");

