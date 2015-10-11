
require('system')
print(System.test())

--[[
local files = Sys.listdir('.')
for _, file in pairs(files) do
    print(file.name)
end
--]]
