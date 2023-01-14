local config = {}

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

local configfile = 'etc/config.lua'
config = readconf(configfile)
assert(config.rockspec, 'rockspec entry not found in config.lua')
config.rockspec = ('etc/rockspec/%s'):format(config.rockspec)
return config
