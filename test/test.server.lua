--

local fcgi = require "fcgi"
local net = require "net"
local trace = require "trace"

--

local rep = "\1\7\0\1\0\23\1\0Primary script unknown\x0A\0\1\6\0\1\0Q\7\0Status: 200 Not Found\13\x0AContent-type: text/html; charset=UTF-8\13\x0A\13\x0AHello, World!!!\x0A\0\0\0\0\0\0\0\1\3\0\1\0\8\0\0\0\0\0\0\0\0\0\0"

local listener = assert(net.ip4.tcp.socket())

assert(listener:set(net.f.SO_REUSEADDR, 1))
assert(listener:bind("127.0.0.1", 12345))
assert(listener:listen(128))

while true do
    print("w8 for clients ... ")

    local client = assert(listener:accept())

    print("client caught, fd:", client:fd())

    while true do
        local msg = assert(client:recv())

        trace(msg)
        trace(fcgi.unpack(msg))

        if #msg==0 then
            break
        else
            assert(client:send(rep))
            --print "out:"
            --trace(rep)
        end
    end

    client:close()
end
