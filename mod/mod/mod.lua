module('Mod', package.seeall)
--[[


Mod.load('scene')
Mod.reload('scene')
Mod.reload()
Mod.call('update')


--]]


mod_table = {}

function test()
    print('test')
end

function load(mod_path)
    --TODO根据模块路径进行搜索
    local pats = string.split(mod_path, '/')
    local mod_name = pats[#pats]
    print(string.format('mod_name(%s)', mod_name))
    local files = Sys.listdir(mod_path)
    for _, file in pairs(files) do
        if file.type == 'file' then
            local file_path = string.format('%s/%s', mod_path, file.name)
            local index = string.find(file_path, '.lua$')
            if index and index + 3 == string.len(file_path) then
                local require_path = string.sub(file_path, 1, index - 1)
                print(string.format('scan file(%s)', file_path))
                local mod = require(require_path)
                print(mod)
            else
                print(string.format('ignore file(%s)!!!!!', file_path))
            end
        end
    end
    local mod = _G[string.cap(mod_name)]
    mod_table[#mod_table] = mod
    print('ee', mod, string.cap(mod_name))
end

function reload(mod_path)
end

function call(func_name, ...)
    for _, mod in pairs(mod_table) do
        print(mod)
        local func = mod[func_name]
        if func then
            func(unpack(arg))
        end
    end
end

function call(mod, func_name, ...)
    local func = mod[func_name]
    if func then
        func(unpack(arg))
    end
end
