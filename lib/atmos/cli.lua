--|
--| usage:  atmos [<path> | <command>] [<args>]
--| 
--| To be considered a <path>, the argument must have a slash as in:
--|     "./"          current directory
--|     "rel/path"    relative path
--|     "/abs/path"   absolute path
--|
--| Commands are one of the following:
--|     init          create the structure in current directory
--|     update        check for dependency updates
--|     help          this message
--|

local atmos   = require 'atmos'
local waxpath = require 'wax.path'
local cat     = table.concat
local unpack  = table.unpack or unpack
local stderr = io.stderr
local target = ('%s/%%s'):format(waxpath.getcwd())

--
-- HELPER FUNCTIONS
--
local function clierror(msg, val)
  stderr:write('[ERROR]  ', msg:format(val), "\n\r")
  os.exit(1)
end

local function cliwarn(msg, val)
  stderr:write('[WARN] ', msg:format(val), "\n\r")
end

local function cfgsub(content)
  return (content:gsub('$%(([%d%l_]+)%)', atmos.config))
end

local function mkdir(p)
  return waxpath.mkdirs(target:format(p),"777")
end

local function mkfile(name, content, overwrite, mode)
  name = target:format(name)
  if waxpath.isfile(name) and not overwrite then
    return
  end
  local f = io.open(name, 'w+')
  if f
    then
      f:write(cfgsub(content))
      f:close()
      if mode then waxpath.chmod(name, mode) end
    else
      clierror("%s: couldn't write to file", name)
  end
end

--
-- COMMANDS
--

-- Provides help and routing according to the user choice
local command = {}

function command.init()
  mkdir 'etc'
  mkdir 'bin'
  mkdir 'ext'
  mkfile('atmospec.lua', cat({
    '-- List the Lua compatible versions. Leftmost has priority.',
    '-- Ex: `lua = { "5.3", "5.2" }` will try 5.3 and then 5.2',
    'lua = { "$(lua_version)" }','',
    '-- External dependencies',
    'ext = {',
    '  -- example:',
    '  -- packagename = "protocol://GIT_URL"',
    '  -- packagename = { "protocol://GIT_URL", reg="dev", dir="lib" }',
    '  -- packagename = { "protocol://GIT_URL", reg="dev", selfdir="lib" }',
    '  -- packagename = "relative/path"',
    '  -- packagename = "/abs/path"',
    '}','',
    '-- Luarocks dependencies',
    '-- Write down your Luarocks dependencies exactly as in rockspec',
    '-- "dependencies" entry',
    'rocks = {',
    '  -- "rockname > 1.0",',
    '  -- "otherrock ~= 0.4"',
    '}',
  },'\n'))

  mkfile('bin/main.lua', cat({
  '#!/usr/bin/env atmos',
  'local ex = require "example"',
  'ex.run()'},'\n'), false, '755')

  -- Do not create if directory was not initialized
  if not waxpath.isdir 'lib' then
    mkdir 'lib'
    mkfile('lib/example.lua', cat {
      'local module = {}',
      'function module.run() print("hello world") end',
      'return module', '\n' })

  end
end

function command.update()
  local wt = require 'wax.table'

  local rocks = assert(atmos.config('rocks'), '`rocks` config entry missing')
  local rsname = 'ext-dep-0.rockspec'
  mkdir 'ext/.luarocks'
  local rockspectpl = cat({
    'package = "ext"',
    'version = "dep-0"',
    'source = { url ="" }',
    'build = { type="builtin", modules={}}',
    'dependencies = %s',
  },"\n")


  mkfile('ext/.luarocks/'..rsname,
             rockspectpl:format(wt.tostring(rocks)),
             true)

  local luarocks_update = cat({
    'luarocks ',
      '--lua-version $(lua_version) ',
      '--tree $(root)/ext/.luarocks ',
      'make $(root)/ext/.luarocks/',rsname
  })

  os.execute(cfgsub(luarocks_update))
end

if command[arg[1]] then
  command[arg[1]]({ unpack(arg,2) })
  os.exit(0)
end
local note = require 'wax.note'
print(note.self())
