#!/usr/bin/env lua
--| It is an automation system for development and code publishing
--|
--| * clean      Remove compile and test stage artifacts
--| * dockbuild  Build Docker instance for tests
--| * docklist   List available Docker confs
--| * dockrun    Run command on Docker test instance
--| * docktest   Run Lua deva files through the Docker test instance
--| * docmd      Retrieve list of code signatures in markdown
--| * help       Retrieve the list of documented items
--| * install    Install the rockspec for all Lua versions
--| * remove     Uninstall the rockspec for all Lua versions
--| * sparse     Run semantic parser for C
--| * test       Compile, and run Lua deva files

local command = {}
local sh = require 'etc.run.sh'
local config = require 'etc.run.config'
local luaVersions = {"5.1","5.2","5.3","5.4"}
local sepline = "*******************************"
local sepline2= "-------------------------------"
local docdir  = "./test"
local testdir = "./test"

local luabin = {
	["5.1"] = sh.whereis("lua%s","5.1","51"),
	["5.2"] = sh.whereis("lua%s","5.2","52"),
	["5.3"] = sh.whereis("lua%s","5.3","53"),
	["5.4"] = sh.whereis("lua%s","5.4","54"),
}


function test_compile(luaver)
	print(("Compiling for Lua %s"):format(luaver))
	print(sepline)
	sh.exec("luarocks --tree ./tree --lua-version %s make %s",luaver,config.rockspec)
end

function test_lua(luaver)
	local lbin  = luabin[luaver]
	local lpath = ("./tree/share/lua/%s/?.lua;./tree/share/lua/%s/?/init.lua"):format(luaver,luaver)
	local cpath = ("./tree/lib/lua/%s/?.so" ):format(luaver)

	local p = io.popen("find "..testdir.." -name '*.lua' 2> /dev/null","r")
	if p == nil then return end

	print(("\nTesting with Lua %s"):format(luaver))
	print(sepline)
	testnum = 0
	local file = p:read()
	while file do
		testnum = testnum + 1
		print(("\nTEST %s "):format(file))
		sh.exec(
			[[ TZ=UTC+0 %s -e 'package.path=%q package.cpath=%q' %q ]],
			lbin, lpath, cpath, file
		)
		file = p:read()
	end
	print( "\n".. sepline2
			 , "\n".. ("Total: %d tests"):format(testnum)
			 , "\n".. sepline2
			 , "\n\n\n")
end

function docker_names()
	local files = sh.rexec("ls etc/docker/*.dockerfile")
	local names = {}
	for i,dfile in ipairs(files) do
		names[dfile:gsub("^.*/",""):gsub(".dockerfile$","")] = 1
	end
	return names
end

function docker_image(name)
	name = name or "luawax_debian"
	if sh.OS ~= "Linux" then
		print("This command requires a Linux machine host")
		os.exit(1)
	end
	if not docker_names()[name] then
		command.docklist()
		os.exit(1)
	end

	local name = sh.rexec("docker images | grep %q | awk '{print $1}'",name)[1]
	if not name then
		print(("Try build first with:\n\n\t./run dockbuild %s\n"):format(arg[2]))
		os.exit(1)
	end
end


--
-- Public actions
-- Below functions are used as actions called directly from Ex:
-- ./run docklist

function command.clean()
	print("Cleaning project")
	sh.exec("rm -rf ./tree ./wax ./out ./lua ./luarocks ./lua_modules ./.luarocks")
	sh.exec("rm -rf ./lua ./luarocks ./lua_modules ./.luarocks")
	sh.exec("find ./src -name '*.o' -delete")
end


function command.help()
	cmd = ([[
		cat $(find %s -name '*.lua') \
		| grep '\--\$'|cut -d' ' -f2- 2> /dev/null | fzf
	]]):format(docdir)
	os.execute(cmd)
end


function command.docmd()
	cmd = ([[
		for i in $(find %s -name '*.lua'); do
			grep -A1 '^\(--\$\|--| #\)' $i | grep -- '^--'
		done | sed 's/^--\$\s*\(\([^(]\+\).*\)\s*$/###### \1/g;s/^--[}{|]\?\s*//g;s/^#/\n#/g'
	]]):format(docdir)
	os.execute(cmd)
end


function command.test()
	for i,luaver in ipairs(luaVersions) do
		test_compile(luaver)
		test_lua(luaver)
	end
end


function command.sparse()
	local cmd = table.concat {
		[[ sparse -Wsparse-error ]],
		[[ -Wno-declaration-after-statement ]],
		[[ -Wsparse-all ]],
		[[ -I/usr/include/lua%s ]],
		[[ -I./src ]],
		[[ -I./src/lib ]],
		[[ src/*c 2>&1 | grep -v "unknown attribute\|note: in included file" | tee /dev/stderr | wc -l ]]
	}
	print("\nRunning sparse")
	print(sepline)
	for _,luaver in ipairs(luaVersions) do
		print (("\n - Targeting C for Lua %s"):format(luaver))
		if (sh.rexec(cmd:format(luaver)))[1] ~= "0" then
			os.exit(1)
		end
	end
	print("")
	print(sepline,"\nSparsed OK! :)\n")
end


function command.docklist()
	print("\nAvailable docker confs:\n")
	for name,_ in pairs(docker_names()) do print(name) end
end


function command.dockbuild()
	local img = docker_image(arg[2])
	if img then
		sh.rexec([[docker rmi "%s:latest"]],img)
		sh.exec([[docker build -t "%s:latest" -f "etc/docker/%s.dockerfile" .]], imgname, imgname)
	else
		command.docklist()
	end
end


docker_run_cmd = [[docker run -ti --rm --mount=type=bind,source=%q,target=/devel %q %s]]
function command.dockrun()
	local img = docker_image(arg[2])

	local runcmd = docker_run_cmd:format(sh.PWD, img, "bash")
	os.execute(runcmd)
end


function command.docktest()
	local strgetimg = "docker images | grep %q | awk '{print $1}'"
	local strcmd    = [[bash -c "cd /devel && TERM=%q ./run test || exit 1; ./run clean"]]
	local strnotimg = "Try build first with:\n\n\t./run dockbuild %s\n"

	local imgname = arg[2] or "luawax_debian"

	if imgname and docker_names()[imgname] then
		imgname = sh.rexec(strgetimg, imgname)[1]
		if not imgname then
			print(strnotimg:format(arg[2]))
			os.exit(1)
		end
		local cmd = strcmd:format(sh.TERM) -- run inside docker
		sh.exec(docker_run_cmd:format(sh.PWD, imgname, cmd))
	end
end

function command.install()
	local cmd = 'luarocks --lua-version %q make %q'
	for k,_ in pairs(luabin) do
		os.execute(cmd:format(k, config.rockspec))
	end
end

function command.remove(rockspec)
	local cmd = 'luarocks --lua-version %q remove %q'
	for k,_ in pairs(luabin) do
		os.execute(cmd:format(k, rockspec or config.rockspec))
	end
end

if command[arg[1]] then
	command[arg[1]]( (table.unpack or unpack)(arg,2) )
else
	local f = io.open(arg[0])
	repeat
		line = f:read()
		if line and line:find('^%-%-|%s?') == 1 then
			print(line:sub(5))
		end
	until not line
	f:close()
end
