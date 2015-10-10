module('Login', package.seeall)


function login(player)
    Pbtest.connect(player)

    Pbtest.send(player, 'login.LOGIN', {uid = player.uid, params = '3333'})
    local reply = Pbtest.recv(player, 'login.LOGIN')

    Pbtest.send(player, 'login.ENTER', {})
    local reply = Pbtest.recv(player, 'login.ENTER')


    --Pbtest.send(player, 'login.ENTER', {})
    --local reply = Pbtest.recv(player, 'login.ENTER')
end
