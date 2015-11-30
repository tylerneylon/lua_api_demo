--[[

demo_run.lua

This file is intended to demonstrate how to use the apidemo module.

--]]


-- Setup.
local apidemo = require 'apidemo'
apidemo.setup_globals()
L = luaL_newstate()

-- We'll use semicolons below to make the code more closely resemble C.

-- Writing values.
lua_pushnumber(L, 42);
lua_pushstring(L, "hello!");
lua_pushnil(L);
lua_pushboolean(L, 1);  -- Pushes `true`.

-- Stack manipulation.
print("stack size =", lua_gettop(L));
lua_insert(L, 1);  -- Move the last value to become the 1st.
lua_pop(L, 1);  -- Pop the last 1 value.
lua_pushvalue(L, 2);  -- Copy the 2nd value to the top of the stack.

-- Reading values.
print("Is 1st value a boolean?", lua_isboolean(L, 1));
print("What is 2nd value as a number?", lua_tonumber(L, 2));
print("What is the type of the 3rd value?", luaL_typename(L, 3));
print("What is the opt (def=3.1) 6th value?", luaL_optnumber(L, 6, 3.1));

-- Table ops.
lua_newtable(L);
lua_pushstring(L, "value1");
lua_setfield(L, -2, "key1");

-- Read a key/value pair.
lua_pushnil(L);
lua_next(L, -2);

-- See if there's another key/value pair.
lua_pop(L, 1);
print("lua_next returned", lua_next(L, -2));

-- Basic operator example.
lua_pushstring(L, "a");
lua_pushstring(L, "b");
lua_pushstring(L, "c");
lua_concat(L, 3);

-- Function call.
lua_getglobal(L, "print");
lua_pushstring(L, "I am a string to be printed!");
lua_call(L, 1, 0);  -- 1 input, 0 outputs

-- Compile and run some code.
luaL_loadstring(L, "print('I am printed from runtime-compiled code!')");
lua_call(L, 0, 0);  -- 0 inputs, 0 outputs
