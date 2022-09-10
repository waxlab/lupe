if not _G.arg then
  local arg, argc = {}, 0
  while argc < 1000 do
    local var = os.getenv(('LUFT_ARG[%d]'):format(argc))
    if not var then break end
    arg[argc], argc = var, argc+1
  end
  _G.arg = arg
end

for i = -1, 5, 1 do
  local info = debug.getinfo(i,'S')
  print(('lua debug.getinfo(%d) .source=%q .shortsrc=%q')
    :format(
      i,
      info and info.source or '',
      info and info.short_src or ''
    )
  )
end


for i=0, #arg, 1 do print('lua envargs',i, arg[i]) end

