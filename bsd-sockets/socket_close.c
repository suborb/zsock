/* int socket_read(int fd, void *buf, size_t count) */


#include "socket.h"


int socket_close(int fd)
{
    int              len;
    struct __socket *sock = socket_get(fd);

    if ( sock == NULL )    /* No socket */
	return -1;

    sock_close(sock->socket);
    if ( sock_waitclose(sock->socket) == -1 ) {
	return -1;
    }
    sock_shutdown(sock->socket);
    return 0;
}
