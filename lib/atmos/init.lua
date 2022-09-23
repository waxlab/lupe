local wax = require 'wax'
local atmos = {
  root     = os.getenv 'ATMOS_ROOT',
  specfile = os.getenv 'ATMOS_SPEC',
  lua      = _VERSION:gsub('.* ([%d.]*)$','%1')
}


--$ atmos.spec : table
--| Standard keys:
--| * lua_version : The current used Lua version.
--| * root : Full path of the project root directory
--| * spec : Full path of the project atmospec file
--| * ext : List of dependencies
--| * rocks : Luarocks rocks (same format as in rockspec `dependencies` key)

--$ atmos.readspec()
--| Setup atmos using the atmospec file on already initialized dirs
--| Resulting data is stored under `atmos.spec` table.
--| If atmospec.lua is not found an error is thrown.
function atmos.readspec()
  if atmos.spec then return atmos.spec end
  atmos.spec = {}
  assert(wax.loadfile(atmos.specfile, atmos.spec))()
  return atmos.spec
end


-- Initialize global args if not initialized
-- Useful for the cli and sbi (shebang interface)
if not _G.arg then
  local arg, argc = {}, 0
  while argc < 1000 do
    local var = os.getenv(('LUA_ARG[%d]'):format(argc))
    if not var then break end
    arg[argc], argc = var, argc+1
  end
  _G.arg = arg
end

return atmos
