/* int recv(int fd, void *buf, int len, int flags) */


#include "socket.h"


int recv(int fd, void *buf, int len, int flags)
{
    struct __socket *sock = socket_get(fd);
    int  written,temp;

    if ( sock == NULL )
	return -1;

    return ( sock_recv(sock->socket,buf,len,flags) );
}

