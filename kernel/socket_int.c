/*
 *      Socket (internal) Routines For ZSock
 *
 * 	These are routines that are called by code
 *	in ZSock itself when using unified SOCKET
 *	appearance.
 *
 *	The package code..just calls these when necessary...
 *	Only a few here (mostly used by daemons)
 */


#include "zsock.h"




/*
 *      Write a stream of bytes to a socket
 */

int  sock_write_i(TCPSOCKET *s,u8_t *dp,u16_t len) 
{
    switch (s->ip_type) {
    case prot_UDP:
	return (udp_write((UDPSOCKET *)s,dp,len));
    case prot_TCP:
	return (tcp_write(s,dp,len));
    }
    return_c EPROTONOSUPPORT;
}


/*
 *      Write a line of text to a socket
 */

int  sock_puts_i(TCPSOCKET *s,u8_t *dp) 
{
    return (sock_write_i(s,dp,strlen(dp)));
}

/*
 *      Close a socket
 */

void  sock_close_i(TCPSOCKET *s)
{
    switch (s->ip_type) {
    case prot_UDP:
	udp_close((UDPSOCKET *)s);
	break;
    case prot_TCP:
    case CONN_CLOSED:
	tcp_close(s);
    }
}

int  sock_dataready_i(TCPSOCKET *s)
{
    switch (s->ip_type) {
    case prot_UDP:
    case prot_TCP:
    case CONN_CLOSED:
	return_nc s->recvoffs-s->recvread;
    }
    return_c EPROTONOSUPPORT;
}

int  sock_read_i(
		 TCPSOCKET       *s,
		 u8_t            *dp,
		 u16_t           len,
		 u8_t            flags)
{
    switch (s->ip_type) {
    case prot_UDP:
	return_nc(udp_read((UDPSOCKET *)s,dp,len,flags));
    case prot_TCP:
    case CONN_CLOSED:
	return_nc(tcp_read(s,dp,len,flags));
    }
    return_c EPROTONOSUPPORT;
}


/* Kill daemon sockets matching myport we supply a base socket address */

int kill_socket(tcpport_t myport,TCPSOCKET *s)
{
	TCPSOCKET *sp;
	while (1) {
	    if ( s == NULL ) 
		return;
	    sp = s->next;
	    if (s->myport == htons(myport) ) { //&& s->hisaddr == 0L ) {
		switch(s->ip_type) {
		case prot_UDP:
		    if (s->datahandler) {
#ifdef Z80
			HCALL(EOF,0,0,0,s);
#else
			(s->datahandler)(EOF,0,0,0,s);
#endif
		    } else {
			udp_close((UDPSOCKET *)s);
		    }
		    break;
		case prot_TCP:
		    tcp_abort(s);
		}
	    }
	    s=sp;
	}
}

