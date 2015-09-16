module('Actor', package.seeall)

--创建
function create(entityid)
    local co = coroutine.create(function()
        local co = coroutine.running()
        EntityLoader.load(entityid, co)
        local data = yield co
        local actor = Actor.load(data)
        Entity.add_entity(actor)
    end)
    coroutine.resume(co)
end
