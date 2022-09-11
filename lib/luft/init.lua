if not _G.arg then
  local arg, argc = {}, 0
  while argc < 1000 do
    local var = os.getenv(('LUA_ARG[%d]'):format(argc))
    if not var then break end
    arg[argc], argc = var, argc+1
  end
  _G.arg = arg
end
for i,v in pairs(arg) do print(i, v) end

if (os.getenv('LUFT_SHELL')) then
  require('luft.core.shell')
else
  require('luft.core.command')
end
