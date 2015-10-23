module('Argparse', package.seeall)

--[
--
-- xx -x y --x=y
--
-- Argparse.add_arg('x')
-- Argparse.parse_argv()
-- local x = Argparse.get_arg('x')
--
--]]

local kv_argv = {}
local plain_argv = {}
local plain_args = ''

function parse_argv(args)
    local nextkey = nil
    for _, str in ipairs(args) do
        if string.find(str, '%-%-') then
            local index = string.find(str, '=')
            local key = string.sub(str, 3, index - 1)
            local val = string.sub(str, index + 1)
            kv_argv[key] = val
        elseif string.find(str, '%-') then
            nextkey = string.sub(str, 2)
        elseif nextkey then
            local key = nextkey
            local val = str
            nextkey = nil
        else
            table.insert(plain_argv, str)
            plain_args = string.format('%s %s', plain_args, str)
        end
    end
end

function get_arg(varname)
    return kv_argv[varname]
end

function get_plain_args()
    return plain_args
end

function test()
    parse_argv({'--cpp_out=hello', '-o', 'afaf', 'af', 'asfdaf'})
    print(get_plain_args())
end


