
require('system')
print(System.test())
for i = 1, 10000000 do
    System.errno()
end
--[[
local files = Sys.listdir('.')
for _, file in pairs(files) do
    print(file.name)
end
--]]
