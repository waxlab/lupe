-- This module acts like a help provider for commands and as a router that
-- requires the command specific module.
-- The command modules acts as a sequence of actions like a shellscript.
-- For common helper functions see the `atmos.core` module

local cmd = {
  init   = 'atmos.corecmd.initcmd',
  update = 'atmos.corecmd.update',
  scm    = 'atmos.corecmd.scm',
  rock   = 'atmos.corecmd.rock'
}

if not cmd[ arg[1] ] then
  print[[
  Available commands are:
  init   - create the structure in current directory
  update - update the list of dependencies
  scm    - handle scm dependencies
  rock   - handle luarocks dependencies
  ]]
else
  require ( cmd[ arg[1] ] )
end

