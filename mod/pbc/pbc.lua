module('Pbc', package.seeall)

function import_dir(dir)
    pbc.mappath('', dir)
    local files = Sys.listdir(dir)
    for _, file in pairs(files) do
        if file.type == 'file' and string.find(file.name, '.proto$') then
            log('load proto(%s)', file.name)
            pbc.import(file.name)
        end
    end
end

