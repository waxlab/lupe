if not _G.arg then
  local arg, argc = {}, 0
  while argc < 1000 do
    local var = os.getenv(('LUA_ARG[%d]'):format(argc))
    if not var then break end
    arg[argc], argc = var, argc+1
  end
  _G.arg = arg
end

if os.getenv 'atmos_SHELL' then
  require 'atmos.coresh'
else
  require 'atmos.corecmd'
end
