/*  int socket(int domain, int type, int protocol) */


#include "socket.h"


static struct __socket __socks[10];


int socket(int domain, int type, int protocol)
{
    u8_t  proto;
    int   i;

    if ( domain != PF_INET ) {   /* Only support PF_INET */
	return -1; 
    }

    switch ( type ) {
    case SOCK_STREAM:
	proto = prot_TCP;
	break;
    case SOCK_DGRAM:
	proto = prot_UDP;
	break;
    default:
	return -1;
    }

    for ( i = 0; i < sizeof(__socks)/sizeof(struct __socket); i++ ) {
	if ( __socks[i].protocol = 0 ) {
	    __socks[i].protocol = proto;
	    return i;
	}
    }
    return -1;
}


struct __socket *socket_get(int fd)
{
    struct __socket *sock = &__socks[fd];

    if ( sock->protocol == 0 )
	return NULL;

    return sock;
}
