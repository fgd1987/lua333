module('Mod', package.seeall)
--[[


Mod.load('scene')
Mod.reload('scene')
Mod.reload()
Mod.call('update')


--]]


mod_table = {}


function load(mod_path)
    require(mod_path)
    local mod_name = xx
    local mod = _G[string.cap(mod_name)]
    mod_table[mod_path] = mod
    call(mod, 'main')
end

function reload(mod_name)
    require(mod_name)
end


function call(func_name, ...)
    for _, mod in pairs(mod_table) do
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
