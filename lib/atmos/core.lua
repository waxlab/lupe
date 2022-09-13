local waxpath = require 'wax.path'

local core = {}
local vars = {
  lua_version = _VERSION:gsub('%S+%s','')
}

-- Initialize global args if not initialized
-- Useful for the sbi (shebang interface) and cli
if not _G.arg then
  local arg, argc = {}, 0
  while argc < 1000 do
    local var = os.getenv(('LUA_ARG[%d]'):format(argc))
    if not var then break end
    arg[argc], argc = var, argc+1
  end
  _G.arg = arg
end

local target = ('%s/%%s'):format(waxpath.getcwd())


local stderr = io.stderr

function core.error(msg, val)
  stderr:write('[ERROR]  ', msg:format(val), "\n\r")
  os.exit(1)
end

function core.warn(msg, val)
  stderr:write('[WARN] ', msg:format(val), "\n\r")
end


function core.mkdir(p)
  return waxpath.mkdirs(target:format(p),"777")
end

function core.parsevars(content)
  return
    (content
      :gsub('$%(([%d%l_]+)%)',
            function(p)
              return vars[p] or 'nil'
            end))
end

function core.mkfile(name, content, overwrite, mode)
  name = target:format(name)
  if waxpath.isfile(name) and not overwrite then
    return
  end
  local f = io.open(name, 'w')
  if f
    then
      f:write(core.parsevars(content))
      if mode then waxpath.chmod(name, mode) end
    else
      core.error("%s: couldn't write to file", name)
  end
end

return core
