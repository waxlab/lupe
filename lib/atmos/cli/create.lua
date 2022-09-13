local core = require "atmos.core"
local waxpath = require "wax.path"


core.mkdir 'dep'
core.mkdir 'etc'
core.mkdir 'bin'

core.mkfile('atmos', [=[
-- List the Lua compatible versions. Leftmost has priority.
-- Ex: `lua = { "5.3", "5.2" }` will try 5.3 and then 5.2
lua = { "$(lua_version)" }

-- Folder for dependencies.
dep = {
-- example:
-- packagename = "protocol://GIT_URL"
-- packagename = { "protocol://GIT_URL", reg="dev", dir="lib" }
-- packagename = { "protocol://GIT_URL", reg="dev", selfdir="lib" }
-- packagename = "relative/path"
-- packagename = "/abs/path"
}
]=])

core.mkfile('bin/main.lua', [=[
#!/usr/bin/env atmos
local ex = require "example"
ex.run()
]=], false, '755')

-- Do not create if directory was not initialized
if not waxpath.isdir 'lib' then
  core.mkdir 'lib'
  core.mkfile('lib/example.lua', [[
local module = {}
function module.run() print("hello world") end
return module
]])
end
