--|
--| usage:  lupe [<path> | <command>] [<args>]
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

local lupe   = require 'lupe'
local fs = require 'wax.fs'
local cat     = table.concat
local unpack  = table.unpack or unpack
local stderr = io.stderr
local target = ('%s/%%s'):format(fs.getcwd())

--
-- HELPER FUNCTIONS
--
local confsubst do
	local function c(k) return lupe[k] end
	function confsubst(s) return (s:gsub('$%(([%d%l_]+)%)', c)) end
end

local function clierror(msg, val)
	stderr:write('[ERROR]  ', msg:format(val), "\n\r")
	os.exit(1)
end

local function cliwarn(msg, val)
	stderr:write('[WARN] ', msg:format(val), "\n\r")
end

local function mkdir(p)
	return fs.mkdirs(target:format(p),"777")
end

local function mkfile(name, content, overwrite, mode)
	name = target:format(name)
	if fs.isfile(name) and not overwrite then
		return
	end
	local f = io.open(name, 'w+')
	if f
		then
			f:write(confsubst(content))
			f:close()
			if mode then fs.chmod(name, mode) end
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
		mkdir 'deps'
		mkfile('luperc.lua', cat({
		'-- List the Lua compatible versions. Leftmost has priority.',
		'-- Ex: `lua = { "5.3", "5.2" }` will try 5.3 and then 5.2',
		'lua = { "$(lua)" }','',
		'-- External dependencies',
		'deps = {',
		'  -- example:',
		'  -- packagename = "protocol://GIT_URL"',
		'  -- packagename = { "protocol://GIT_URL", ref="dev", dir="lib" }',
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
	'#!/usr/bin/env lupe',
	'local ex = require "example"',
	'ex.run()'},'\n'), false, '755')

	-- Do not create if directory was not initialized
	if not fs.isdir 'lib' then
		mkdir 'lib'
		mkfile('lib/example.lua', cat ({
			'local module = {}',
			'function module.run() print("hello world") end',
			'return module'}, '\n'))

	end
end


function command.update()
	lupe.readconf()
	local wt = require 'wax.table'
	local rocks = assert(lupe.rc.rocks, '`rocks` config entry missing')
	local rsname = 'ext-dep-0.rockspec'
	local rockspectpl = cat({
		'package = "ext"',
		'version = "dep-0"',
		'source = { url ="" }',
		'build = { type="builtin", modules={}}',
		'dependencies = %s',
	},"\n")

	mkdir 'deps/.rocks'
	mkfile(
		'deps/.rocks/'..rsname,
		rockspectpl:format(wt.tostring(rocks)),
		true
	)
	local rocks_update = cat({
		'luarocks ',
			'--lua-version $(lua) ',
			'--tree $(root)/deps/.rocks ',
			'make $(root)/deps/.rocks/',rsname
	})

	os.execute(confsubst(rocks_update))
end

if command[arg[1]] then
	command[arg[1]]({ unpack(arg,2) })
	os.exit(0)
end
local note = require 'wax.note'
print(note.self())
