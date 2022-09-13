require "atmos.core"

-- Provides help and routing according to the user choice
local actions = {
  create = 'create the structure in current directory',
  update = 'update the list of dependencies'
}

if not actions[ arg[1] ] then
  print[[Available commands are:]]
  for act, help in pairs(actions) do print(act, help) end
else
  require ( ('atmos.cli.%s'):format(arg[1]) )
end

os.exit(0)
