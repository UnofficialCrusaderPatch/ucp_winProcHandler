"""Exercise the shipped framework loader/proxy path, not an unproxied export."""
import os
from pathlib import Path
import unittest

from lupa.lua54 import LuaRuntime as Lua54
from lupa.luajit21 import LuaRuntime as LuaJIT

ROOT = Path(__file__).resolve().parents[1]


class FrameworkProxyTests(unittest.TestCase):
    def test_cinterface_keeps_native_exports_read_only(self):
        proxy = Path(os.environ['UCP_FRAMEWORK_PROXIES']).read_text(encoding='utf-8')
        for runtime in (Lua54, LuaJIT):
            with self.subTest(runtime=runtime.__module__):
                lua = runtime(unpack_returned_tuples=True)
                lua.globals().root = ROOT.as_posix()
                lua.globals().proxies = lua.execute(proxy)
                lua.execute('''
local native={address_FillWithWindowProcCallback=101,funcAddress_WindowProc=102,
 funcAddress_RegisterProc=103,funcAddress_GetMainProc=104,funcAddress_CallNextProc=105}
package.preload['winProcHandler.dll']=function() return native end
local scans,writes=0,0
core={AOBScan=function() scans=scans+1; return 1000*scans end,
 writeCode=function(address,value)
  writes=writes+1
  assert((address==101 and value[1]==1000) or (address==2004 and value[1]==102))
 end}
local owner,options=dofile(root..'/init.lua')
options=options or {public={},proxy={}}
local module=proxies.ExtensionProxy(owner,options.proxy)
owner:enable({})
local api=module:cinterface()
assert(api.RegisterProc==103 and api.GetMainProc==104 and api.CallNextProc==105)
assert(not pcall(function() api.RegisterProc=0 end))
assert(module:cinterface().RegisterProc==103)
assert(scans==2 and writes==2)
''')


if __name__ == '__main__':
    unittest.main()
