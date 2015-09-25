require('pbc')

print(Pbc.mappath('', ''))
Pbc.import('proto/login.proto')
local msg = Pbc.msgnew('login.ENTER')
msg.uid = 333
msg.params = '333'
print(msg:debug_string())

local t = msg:totable()
print(t.uid)
print('over')
