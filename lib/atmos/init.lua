local atmos = {}
local config = {}
local wax = require 'wax'

--@ atmos.config(key : string) : atmos.config
--| Standard keys:
--| * lua_version : The current used Lua version.
--| * root : Full path of the project root directory
--| * file : Full path of the project config file
--| * ext : List of dependencies
--| * rocks : Luarocks rocks (same format as in rockspec `dependencies` key)
function atmos.config(key)
  if not config.file then
    local cfgfile = os.getenv 'ATMOS_CONFIG'
    assert(wax.loadfile(cfgfile, config))()
    config.lua_version = _VERSION:gsub('.* ([%d.]*)$','%1')
    config.root = os.getenv 'ATMOS_ROOT'
    config.file = cfgfile
  end
  return config[key]
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
