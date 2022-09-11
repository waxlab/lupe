local core = require "atmos.core"
local waxpath = require "wax.path"
local target = ('%s/%%s'):format(waxpath.getcwd())
local vars = {
  lua_version = _VERSION:gsub('%S+%s','')
}

local mkdir do
  local mk = waxpath.mkdirs
  function mkdir(p) return mk(target:format(p),"777") end
end

local mkfile do
  local isfile=waxpath.isfile
  function mkfile(name, content)
    name = target:format(name)
    if not isfile(name)
      then
        local f = io.open(name, 'w')
        if f
          then f:write((content:gsub('$%(([%d%l_]+)%)',function(p) return vars[p] or 'nil' end)))
          else
            core.error("%s: couldn't write to file", name)
        end
      else
        core.warn("%s: file already exists")
    end
  end
end

mkdir 'src'
mkdir 'sub/scm'
mkdir 'sub/rock'
mkdir 'etc'
mkdir 'bin'

mkfile('atmos', [=[
-- List the Lua compatible versions. Leftmost has priority.
-- Ex: `lua = { "5.3", "5.2" }` will try 5.3 and then 5.2
lua = { "$(lua_version)" }

-- Configure SCM dependencies. By now only works on Git
--[[
scm = {
  "https://url", -- just the git url or...
  { as = "pkgname", ref="branch-or-tag", url },
}
--]]

-- List the Luarocks dependencies. It is used only on "atmos update"
--[[
rock = {
  name, -- ex: "luajson"
  { name, v=version, m=manifest }, -- ex: {"luajson", v="1.3.4.1", m="root"}
}
]]
]=])

mkfile('bin/main.lua', [=[
#!/usr/bin/env atmos
local ex = require "example"
ex.run()
]=])

waxpath.chmod('bin/main.lua','755')

mkfile('src/example.lua', [[
local module = {}
function module.run() print("hello world") end
return module
]])



