--| Suppose that you call `require("moon.glow")`. This module adds a searcher
--| to find the required module in the following steps:
--|
--| 1. It searches for the local project files, i.e, under the `lib` directory:
--|   * `LUPE/lib/moon/glow.lua`
--|   * `LUPE/lib/moon/glow/init.lua`
--|
--| 2. If there is an `deps.moon` entry in the `luperc.lua` file with with
--| `etc.moon.subdir = "lib"` value (the default), so it will also look for:
--|   * `LUPE/deps/moon/lib/moon/glow.lua`,
--|   * `LUPE/deps/moon/lib/moon/glow/init.lua`,
--|   * `LUPE/deps/moon/lib/glow.lua`
--|   * `LUPE/deps/moon/lib/glow.lua`
--|
--| 3. Finally it will look for a module installed via Luarocks under the
--| `LUPE/deps/.luarocks` tree for the detected Lua version at the run time.


local lupe = require 'lupe'
local path = require 'wax.path'
lupe.readconf()

local dirpath = {
  ('%s/lib/%%s.lua'):format(lupe.root),
  ('%s/lib/%%s/init.lua'):format(lupe.root),
}
local depspath = {
  ('%s/deps/%%s/%%s/%%s.lua'):format(lupe.root),
  ('%s/deps/%%s/%%s/%%s/init.lua'):format(lupe.root),
}

package.path = table.concat({
  ('%s/deps/.rocks/share/lua/%s/?.lua'):format(lupe.root, lupe.lua),
  ('%s/deps/.rocks/share/lua/%s/?/init.lua'):format(lupe.root, lupe.lua),
  package.path
},';')

package.cpath = table.concat({
  ('%s/deps/.rocks/lib/lua/%s/?.so'):format(lupe.root, lupe.lua),
  package.cpath
},';')

local err_str = "\n\tno file '%s' (lupe)"

local searchers =
  lupe.lua == '5.1'
  and package.loaders
  or  package.searchers



local
function lupe_searcher(module)
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
  local deps = lupe.rc.deps[pack]

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

table.insert(searchers, 1, lupe_searcher)
