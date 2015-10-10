module('Player', package.seeall)


function init(player)
    log('player init')
end


function close(player)
    log('player close')
end

function reply(player, msg)
    log('safsdafdsaf %d', player.srvid)
    Gatesrv.reply(player.srvid, player.uid, msg)
end


