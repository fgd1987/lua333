module('Actor', package.seeall)

ActorEntity = {
    id = 0,
    name = 'ljw333',
    level = 1,
}

--保存
function save(actor)
    local data = Entity.save(actor, ActorEntity)
    EntityLoader.save(data)
end

--加载
function load(data)
    local actor = Entity.load(data, ActorEntity)
end
