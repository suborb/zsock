/* int connect(int fd, struct sockaddr *addr,socklen_t addrlen) */

#include "socket.h"

int connect(int fd, struct sockaddr *addr, int addrlen)
{
    struct sockaddr_in *sin = addr;
    struct __socket    *sock;

    if ( sin->sin_family != PF_INET )
	return -1;


    if ( ( sock = socket_get(fd) ) == NULL )  /* Invalid socket */
	return -1;

    sock->socket = sock_open(sin->sin_addr,sin->sin_port,0,sock->protocol);

    if ( sock_waitopen(sock->socket) == -1 ) {
	sock_close(sock->socket);
	sock_shutdown(sock->socket);
	sock->socket = NULL;
	return -1;
    }

    return 0;
}


