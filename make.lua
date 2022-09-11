local make, help = {},{}
local DEBUG = os.getenv('DEBUG')
local OBJ_EXTENSION = os.getenv('OBJ_EXTENSION')
local LIB_EXTENSION = os.getenv('LIB_EXTENSION')

-----------------------------------------------------------
-- HELPERS ------------------------------------------------
-----------------------------------------------------------
local _x = os.execute
local function x(cmd, ...)
  cmd=cmd:format(...)
  if DEBUG then print(cmd, ...) end
  assert(_x(cmd))
end
local function isdir(path)
  local p = io.popen(('file -i %q'):format(path),'r')
  if p then
    local mime = p:read('*a')
    mime = mime:gsub('^[^:]*:%s*',''):gsub('%s*;[^;]*$','')
    if mime == 'inode/directory' then
      return true
    end
  end
  return false
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

    uvj(fn,1,function() return t end, 1)
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
    local verbose = DEBUG and 'v' or ''
    config = readconf('config.lua')
    if isdir('lib') then
      x( 'cp -rf%s lib/* %q || :', verbose, env('INST_LUADIR') )
    end
    if isdir('bin') then
      x( 'cp -rf%s bin/* %q || :', verbose, env('INST_BINDIR') )
    end

    if config.clib and #config.clib > 0 then
      x( 'cp -rf%s out/lib/* %q', verbose, env('INST_LIBDIR') )
    end

    if config.cbin and #config.cbin > 0 then
      x( 'cp -rf%s out/bin/* %q', verbose, env('INST_BINDIR') )
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
    local debug = os.getenv("CC_DEBUG") and ' -g ' or ''
    if o.flags then
      if type(o.flags) == 'table'
        then return debug..table.concat(o.flags, ' ')
        else return debug..tostring(o.flags)
      end
    end
    return env('CFLAGS', debug..'-Wall -Wextra -O2 -fPIC -fdiagnostic-color=always')
  end


  function cc.debug(item)
    return DEBUG and '-g' or ''
  end

  function cc.src(item)
    local t, f = {}, nil
    f = type(item[2]) == 'table' and item[2] or { tostring(item[2]) }

    for i,v in ipairs(f) do
      if v:find('%.%.')
        then error(('invalid filename: %q'):format(v))
      end
      t[#t+1] = 'src/'..v
    end

    return table.concat(t,' ')
  end


  function cc.sharedflag()
    return env('LIBFLAG', '-shared')
  end

  function cc.incdir()
    local incdir = env('LUA_INCDIR',nil)
    return incdir and '-I'..incdir or ''
  end


  function cc.srcout(item)
    if item:find('%.%.') then
      error(('invalid filename: %q'):format(item))
    end
    return table.concat {
      ' -c src/',item,
      ' -o src/',(item:gsub('[^.]+$',OBJ_EXTENSION))
    }
  end

  function cc.libout(item)
    local path = 'out/lib/'..item[1]:gsub('%.','/') -- 'wax.x' to 'wax/x'
    x('mkdir -p %q', path:gsub('/[^/]*$',''))
    local libfile = path..'.'..LIB_EXTENSION
    return libfile;
  end

  function cc.binout(o) return 'out/bin/'..o[1] end


  local
  function clib(config)
    local cmd_obj   = '@cc @debug @flags @incdir @srcout'
    local cmd_shobj = '@cc @sharedflag -o @libout @src'
    if config.clib and #config.clib > 0 then
      x('mkdir -p out/lib')
      for _,item in ipairs(config.clib) do
        for s, src in ipairs(item[2]) do
          -- compile each .c to .o
          x((cmd_obj:gsub('@(%w+)', function(p) return cc[p](src) end)))
          -- replace the name from .c to .o
          item[2][s] = src:gsub('[^.]$',OBJ_EXTENSION)
        end
        x((cmd_shobj:gsub('@(%w+)',function(p) return cc[p](item) end)))
      end
    end
  end

  local
  function cbin(config)
    local cmd = '@cc @debug @incdir @flags @src -o @binout'
    if config.cbin and #config.cbin > 0 then
      x('mkdir -p out/bin')
      for _,o in ipairs(config.cbin) do
        x((cmd:gsub('@(%w+)',function(p) return cc[p](o) end)))
      end
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
