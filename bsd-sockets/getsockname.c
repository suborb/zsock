/* int getpeername(int fd, struct sockaddr *name, socklen_t *namelen) */


#include "socket.h"

int getsockname(int fd, struct sockaddr *name, int *namelen)
{
    struct sockaddr_in  *addr = name;
    struct sockinfo_t    info;
    struct __socket     *sock;

    if ( ( sock = socket_get(fd) ) == NULL )  /* Invalid socket */
	return -1;

    if ( sock_getinfo(sock->socket,&info) != 0 )
	return -1;

    addr->sin_family = PF_INET;
    addr->sin_port   = info.local_port;
    addr->sin_addr.s_addr = info.local_addr;
    *namelen = sizeof(struct sockaddr_in);
    return 0;
}


