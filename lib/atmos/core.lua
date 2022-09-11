local core = {}
local stderr, stdout = io.stderr, io.stdout

function core.error(msg, val)
  stderr:write('[WARN]  ', msg:format(val), "\n\r")
  os.exit(1)
end

function core.warn(msg, val)
  stderr:write('[ERROR] ', msg:format(val), "\n\r")
end

return core
