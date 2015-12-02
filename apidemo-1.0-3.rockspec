package = "apidemo"
version = "1.0-3"
source = {
  url = "git://github.com/tylerneylon/lua_api_demo",
  tag = "v1.0-3"
}
description = {
  summary = "A module to teach or learn about Lua's C API",
  detailed = [[
    This is a Lua module that can help you learn how use Lua's C API.

    It is run from Lua, but purposefully mimics C style, usage, and behavior.
    It's meant to be run from the Lua interpreter to illustrate the effects of
    each API call as it's made. It will also work from a standard Lua script.
  ]],
  homepage = "https://github.com/tylerneylon/lua_api_demo",
  license = "public domain"
}
dependencies = {
  "lua >= 5.1"
}
build = {
  type = "builtin",
  modules = {
    apidemo = "apidemo.c"
  }
}
