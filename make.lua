local make, help = {},{}


-----------------------------------------------------------
-- HELPERS ------------------------------------------------
-----------------------------------------------------------
local _x = os.execute
local function x(cmd, ...)
  local cmd=cmd:format(...)
  print(cmd, ...)
  assert(_x(cmd))
end

local
function totable(str)

end
if _VERSION == "Lua 5.1" then
  function _load(str)
    local t = {}
    local fn, err = loadstring(str, nil)
    if err then return nil, err end
    setfenv(fn,t)()
    return t
  end
else
  local uvj = debug.upvaluejoin
  function _load(str)
    local t = {}
    local fn, err = load(str, nil, 't')
    if err then return nil, err end
    uvj(load(str, nil, 't'),1,function() return t end, 1)
    fn()
    return t
  end
end

local
function env(var, def)
  local val = os.getenv(var)
  if not val or val:len() < 1 then
    if not def then error('No env var "'..var..'"',2) end
    val = def
  end
  return val
end


local
function readconf(file, t)
  local f = io.open(file, 'r')
  if not f then
    if t then return t end
    error(('file %q not found'):format(file),2)
  end
  local res, err = _load( f:read('*a') )
  f:close()
  return err and error(file .. ' ' .. err) or res
end

-----------------------------------------------------------
-- INSTALL ------------------------------------------------
-----------------------------------------------------------
do

  help.install = 'Install the Lua files and compiled binaries and libraries'
  function make.install (config)
    local config = readconf('config.lua')
    if config.lib and #config.lib > 0 then
      x( 'cp -rf lib/* %q', env('INST_LUADIR') )
    end

    if config.clib and #config.clib > 0 then
      x( 'cp -rf out/lib/* %q', env('INST_LIBDIR') )
    end

    if config.bin and #config.bin > 0 then
      x( 'cp -rf bin/*lua %q', env('INST_BINDIR') )
    end

    if config.cbin and #config.cbin > 0 then
      x( 'cp -rf out/bin/* %q', env('INST_BINDIR') )
    end
  end
end

-----------------------------------------------------------
-- CLEAN --------------------------------------------------
-----------------------------------------------------------
function make.clean()
  x('rm -rf ./out ./tmp ./src/*.o')
end



-----------------------------------------------------------
-- BUILD --------------------------------------------------
-----------------------------------------------------------
do
  local cc = {}
  function cc.cc(o)
    return o.cc
        or env('CC','gcc')
  end

  function cc.flags(o)
    if o.flags then
      if type(o.flags) == 'table'
        then return table.concat(o.flags, ' ')
        else return tostring(o.flags)
      end
    end
    return env('CFLAGS', '-Wall -Wextra -O2 -fPIC -fdiagnostic-color=always')
  end

  function cc.src(o)
    local t = {}
    local f = type(o.src) == 'table' and o.src or { tostring(o.src) }

    for i,v in ipairs(f) do
      if v:find('%.%.')
        then error(('invalid filename: %q'):format(v))
      end
      t[#t+1] = 'src/'..v
    end

    return table.concat(t,' ')
  end


  function cc.libout(o)
    local dir = 'out/lib/'..(o.out:gsub('%.?[^.]+$',''):gsub('%.','/'))
    local file = o.out:gsub('.-%.?([^.]+)$','%1')..'.so'
    x('mkdir -p %q', dir)
    return dir..'/'..file
  end

  function cc.binout(o) return 'out/bin/'..o.out end


  local
  function clib(config)
    local cmd = '@cc @flags @src -so @libout'
    if config.clib and #config.clib > 0 then x('mkdir -p out/lib') end
    for _,o in ipairs(config.clib) do
      x((cmd:gsub('@(%w+)',function(p) return cc[p](o) end)))
    end
  end

  local
  function cbin(config)
    local cmd = '@cc @flags @src -o @binout'
    if config.cbin and #config.cbin > 0 then x('mkdir -p out/bin') end
    for _,o in ipairs(config.cbin) do
      x((cmd:gsub('@(%w+)',function(p) return cc[p](o) end)))
    end
  end

  help.build = "Compile C code"
  function make.build ()
    local config = readconf('config.lua')
    clib(config)
    cbin(config)
  end
end


-----------------------------------------------------------
-- EXECUTION ----------------------------------------------
-----------------------------------------------------------

if not make[arg[1]] then
  print('You need to use this script with one of the follow:')
  for cmd,_ in pairs(make) do print(cmd, help[cmd]) end
else
  make[arg[1]]()
end
