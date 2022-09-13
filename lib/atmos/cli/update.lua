local core = require 'atmos.core'
local waxpath = require 'atmos.core'
local conf = core.config()



local luarocks_update = table.concat {
  'luarocks ',
    '--lua-version $lua_version ',
    '--tree $atmos_root/dep/rocks ',
    'make dep/rock-dep-0.rockspec '
}


local rockspectpl = [[
package = "rock"
version = "dep-0"
source = { url ="" }
build = { type="builtin", modules={}}
dependencies = {
   "wax"
}
]]
core.mkfile('dep/rock-dep-0',rockspectpl,true)
os.execute(core.parsevars(luarocks_update))
