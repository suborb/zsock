/* int send(int fd, void *buf, int len, int flags) */


#include "socket.h"


int send(int fd, void *buf, int len, int flags)
{
    struct __socket *sock = socket_get(fd);
    int  written,temp;

    if ( sock == NULL )
	return -1;

    if ( flags & MSG_DONTWAIT ) {
	return ( sock_write(sock->socket,buf,len) );
    } else {
	written = len;
	for ( ;; ) {
	    temp = sock_write(sock->socket,buf + written ,len - written);
	    if ( temp == -1 )
		return -1;
	    written += temp;
	    if ( written == len )
		return len;
	    GoTCP();
	}
    }
}
