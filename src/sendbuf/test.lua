

require('sendbuf')
SendBuf.test()
local sockfd = 1
SendBuf.create(sockfd)

--写数据
local buf = SendBuf.alloc(sockfd, 100)
SendBuf.flush(sockfd, buf, 100)

--读数据
local rbuf = SendBuf.rptr(sockfd)
local rlen = SendBuf.rlen(sockfd)
print(string.format('rlen(%d)', rlen))

SendBuf.free(sockfd)
