require('pbc')

print(pbc.mappath('', ''))
pbc.import('proto/login.proto')
local msg = pbc.msgnew('login.ENTER')
msg.uid = 333
msg.params = '333'
print('debug_string', pbc.debug_string(msg))
print('bytesize', pbc.bytesize(msg))
local t = pbc.totable(msg)
print(t.uid)
print('over')
print(msg:tostring())
