/* int socket_read(int fd, void *buf, size_t count) */


#include "socket.h"


int socket_read(int fd, void *buf,size_t count)
{
    int              len;
    struct __socket *sock = socket_get(fd);

    if ( sock == NULL )    /* No socket */
	return -1;

    len = sock_read(sock->socket,buf,count);

    return len;
}
