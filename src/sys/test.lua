
require('sys')
print(Sys.test())

--[[
local files = Sys.listdir('.')
for _, file in pairs(files) do
    print(file.name)
end
--]]
