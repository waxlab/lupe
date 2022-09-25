local wax = require 'wax'

--$ lupe variables
--| * `lua`     : The current used Lua version.
--| * `root`    : Full path of the project root directory
--| * `rcfile`  : Full path of the project luperc file
local lupe = {
  root   = os.getenv 'LUPE_ROOT',
  rcfile = os.getenv 'LUPE_RC',
  lua    = _VERSION:gsub('.* ([%d.]*)$','%1')
}


--$ lupe.rc : table
--| Standard keys:
--| * deps    : List of dependencies
--| * rocks   : Luarocks rocks (same format as in rockspec `dependencies` key)

--$ lupe.readconf()
--| Setup lupe using the luperc file on already initialized dirs
--| Resulting data is stored under `lupe.rc` table.
--| If luperc.lua is not found an error is thrown.
function lupe.readconf()
  if not lupe.rc then
    lupe.rc = {}
    assert(wax.loadfile(lupe.rcfile, lupe.rc))()
  end
  return lupe.rc
end

--$ lupe.etc(name: string) : string
--| Build a path for a file or directory in the `etc` folder under current
--| Lupe project.
function lupe.etc( name ) return ('%s/etc/%s'):format(lupe.root, name) end


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

return lupe
