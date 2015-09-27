module('Log', package.seeall)

function log(tag, format, ...)
    print(string.format('[LOG][%s]'..format, tag, ...))
end

function error(tag, format, ...)
    print(string.format('[ERROR][%s]'..format, tag, ...))
end

function msg(tag, format, ...)
    print(string.format('[MSG][%s]'..format, tag, ...))
end

function warn(tag, format, ...)
    print(string.format('[WARN][%s]'..format, tag, ...))
end

