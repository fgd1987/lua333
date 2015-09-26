


require('stringpatch')

local parts = string.split('aabbcccbbddddd', 'bb')
for i, str in pairs(parts) do
    print(string.format('%d:%s', i, str))
end
print('')

local parts = string.split('eeee', 'bb')
for i, str in pairs(parts) do
    print(string.format('%d:%s', i, str))
end
print('')

local parts = string.split('bbeeeebb', 'bb')
for i, str in pairs(parts) do
    print(string.format('%d:%s', i, str))
end
print('')

local parts = string.split('eeeebb', 'bb')
for i, str in pairs(parts) do
    print(string.format('%d:%s', i, str))
end


print(string.cap('hello'))
