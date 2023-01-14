-- Simple functions to handle basic system commands usually used
-- via shell scripts;


local sh = {}

unpack = unpack or table.unpack

local DEBUG = os.getenv('DEBUG')

--$ sh.rexec(command: string, ...: string) : table, number
--| Execute command and returns its contents on a table and its numeric exit
function sh.rexec(cmd, ...)
	local cs = {...}
	if #cs > 0 then
		cmd = cmd:format(unpack(cs))
	end

	local proc = io.popen(cmd..';echo $?')
	local lp, lc
	local r = {}
	if proc then
		while true do
			lp = lc
			lc = proc:read()
			rc = 1
			if lc then
				if lp then r[rc] = lp end
				rc=rc+1
			else
				if lp then
					return r, lp
				else
					return r, 1
				end
			end
		end
	else
		return r, 1
	end

end

--$ sh.exec(command: string, ...: string)
--| Execute command and if it has errors abort Lua script
function sh.exec(cmd, ...)
	local cs = {...}
	if #cs > 0 then
		cmd = cmd:format(unpack(cs))
	end

	if DEBUG then print("EXEC "..cmd) end
	local proc = io.popen(cmd..';echo $?')
	local prev, curr = nil, nil
	if proc then
		while true do
			prev = curr
			curr  = proc:read()
			if curr then
				if prev then
					print(' | '..prev)
				end
			else
				if prev ~= "0" then
					print("\n\n//// Exit Status", prev,"\n\n")
					prev = prev or 1
					os.exit(prev)
				end
				return
			end
		end
	else
		os.exit(1)
	end
end

--$ sh.whereis(pattern: string, ... : string)
--| Find the command in system using the single field `pattern` filled with
--| subsequent string parameters.
--|
--| Ex:
--|
--| ```lua
--| sh.whereis('lua%s','51','5.1') -- matches `lua51` and `lua5.1`
--| sh.whereis('%ssed','','g') -- matches `sed` and `gsed`
--| ```
function sh.whereis(cmd, ...)
	local targets = {}
	local cs = {...}
	if #cs > 0 then
		for _,v in ipairs(cs) do
			table.insert(targets,cmd:format(v))
		end
	else
		targets[1] = cmd
	end

	for _,v in ipairs(sh.PATH) do
		for _,w in ipairs(targets) do
			local f = v .. '/' .. w
			local fh = io.open(f,'r')
			if fh ~= nil then
				io.close(fh)
				return f
			end
		end
	end
end

function sh.getpath()
	local path = {}
	local syspath = os.getenv("PATH")
	local P=1;
	for p in syspath:gmatch("([^:]+)") do
		path[P] = p
		P = P+1
	end
	return path
end

sh.PATH = sh.getpath()
sh.OS = sh.rexec("uname -s")[1];
sh.SED = sh.whereis("%ssed","","g")
sh.PWD = sh.rexec("realpath .")[1];
sh.TERM = os.getenv("TERM") or ""
return sh
