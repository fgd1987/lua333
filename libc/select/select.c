

/*

	Select.add(sockfd)
	Select.remove(sockfd)
	local count = Select.poll()
	for i = 0, count do
		Select.get_event(i)
	end
*/

static int maxsockfd = 0;
fd_set read_fds;
fd_set write_fds;
fd_set error_fds;


static int lselect(lua_State *L) {
    int retval, j, numevents = 0;
    struct timeval *tvp
    memcpy(&_read_fds, &_read_fds, sizeof(fd_set));
    memcpy(&_write_fds, &_write_fds, sizeof(fd_set));
	memcpy(&_error_fds, &_error_fds, sizeof(fd_set));

    retval = select(maxsockfd + 1, &_read_fds, &_write_fds, &_error_fds, tvp);
    if (retval > 0) {
        for (j = 0; j <= maxfd; j++) {
            int mask = 0;
            aeFileEvent *fe = &eventLoop->events[j];

            if (fe->mask == AE_NONE) continue;
            if (fe->mask & AE_READABLE && FD_ISSET(j, &state->_rfds))
                mask |= AE_READABLE;
            if (fe->mask & AE_WRITABLE && FD_ISSET(j, &state->_wfds))
                mask |= AE_WRITABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

static int lremove(lua_State *L) {
	FD_CLR(sockfd, &read_fds);
	return 1;
}

static int ladd(lua_State *L) {
	FD_SET(sockfd , &read_fds);
	return 1;
}

static luaL_Reg lua_lib[] = {
    {"add", ladd},
    {NULL, NULL}
};

int luaopen_select(lua_State *L){
	luaL_register(L, "Select", (luaL_Reg*)lua_lib);
	return 1;
}

