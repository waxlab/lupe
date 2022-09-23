--| Suppose that you call `require("moon.glow")`. This module adds a searcher
--| to find the required module in the following steps:
--|
--| 1. It searches for the local project files, i.e, under the `lib` directory:
--|   * `ATMOS/lib/moon/glow.lua`
--|   * `ATMOS/lib/moon/glow/init.lua`
--|
--| 2. If there is an `deps.moon` entry in the `atmospec.lua` file with with
--| `etc.moon.subdir = "lib"` value (the default), so it will also look for:
--|   * `ATMOS/deps/moon/lib/moon/glow.lua`,
--|   * `ATMOS/deps/moon/lib/moon/glow/init.lua`,
--|   * `ATMOS/deps/moon/lib/glow.lua`
--|   * `ATMOS/deps/moon/lib/glow.lua`
--|
--| 3. Finally it will look for a module installed via Luarocks under the
--| `ATMOS/deps/.luarocks` tree for the detected Lua version at the run time.


local atmos = require 'atmos'
local path = require 'wax.path'
atmos.readspec()

local dirpath = {
  ('%s/lib/%%s.lua'):format(atmos.root),
  ('%s/lib/%%s/init.lua'):format(atmos.root),
}
local depspath = {
  ('%s/deps/%%s/%%s/%%s.lua'):format(atmos.root),
  ('%s/deps/%%s/%%s/%%s/init.lua'):format(atmos.root),
}

package.path = table.concat({
  ('%s/deps/.rocks/share/lua/%s/?.lua'):format(atmos.root, atmos.lua),
  ('%s/deps/.rocks/share/lua/%s/?/init.lua'):format(atmos.root, atmos.lua),
  package.path
},';')

package.cpath = table.concat({
  ('%s/deps/.rocks/lib/lua/%s/?.so'):format(atmos.root, atmos.lua),
  package.cpath
},';')

local err_str = "\n\tno file '%s' (atmos)"

local searchers =
  atmos.lua == '5.1'
  and package.loaders
  or  package.searchers



local
function atmos_searcher(module)
  local file
  local try = {}
  local modpath = module:gsub('%.','/')

  for _,m in ipairs(dirpath) do
    file = m:format(modpath)
    if path.isfile(file) then
      return function() return dofile(file) end
    end
    try[#try+1] = err_str:format(file)
  end

  local dotpos = module:find('.',0,true) or 0
  local pack = module:sub(0, dotpos - 1)
  local deps = atmos.spec.deps[pack]

  if deps then
    local subdir = type(deps) == 'table' and deps.subdir or 'lib'
    for _, m in ipairs(depspath) do
      file = m:format(pack, subdir, modpath)
      if path.isfile(file) then return function() return dofile(file) end end
      try[#try+1] = err_str:format(file)
    end

    -- shortened path where pack name part is not under subdir
    modpath = (dotpos > 0 and module:sub(dotpos+1) or ''):gsub('%.','/')
    if modpath ~= '' then
      for _, m in ipairs(depspath) do
        file = m:format(pack, subdir, modpath)
        if path.isfile(file) then return function() return dofile(file) end end
        try[#try+1] = err_str:format(file)
      end
    end
  end
  return table.concat(try)
end

table.insert(searchers, 1, atmos_searcher)
