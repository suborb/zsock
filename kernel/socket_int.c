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

#include <strings.h>
#include <stdio.h>
#include <net/zsockerrs.h>
#include <net/hton.h>
#include "zsock.h"


extern ipaddr_t resolve_i(char *);
extern int reverse_addr_lookup_i(LWORD, char *);

/*
 *      Write a stream of bytes to a socket
 */

int  sock_write_i(s,dp,len)
        TCPSOCKET       *s;
        u8_t            *dp;
        u16_t           len;
{
	switch (s->ip_type) {
		case prot_UDP:
                	return (udp_write(s,dp,len));
		case prot_TCP:
                	return (tcp_write(s,dp,len));
	}
	return_c EPROTONOSUPPORT;
}


/*
 *      Write a line of text to a socket
 */

int  sock_puts_i(s,dp)
        TCPSOCKET *s;
        u8_t    *dp;
{
        return (sock_write_i(s,dp,strlen(dp)));
}

/*
 *      Close a socket
 */

void  sock_close_i(s)
        TCPSOCKET       *s;
{
	switch (s->ip_type) {
		case prot_UDP:
                	udp_close(s);
			break;
		case prot_TCP:
		case CONN_CLOSED:
                	tcp_close(s);
	}
}

int  sock_dataready_i(s)
        TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
		case prot_TCP:
		case CONN_CLOSED:
		        return_nc s->recvoffs-s->recvread;
	}
	return_c EPROTONOSUPPORT;
}


/* Kill daemon sockets matching myport we supply a base socket address */

int kill_socket(myport,s)
	tcpport_t myport;
	TCPSOCKET *s;
{
	TCPSOCKET *sp;
	while (1) {
		if (s == 0 ) return;
		sp=s->next;
		if (s->myport == htons(myport) ) { //&& s->hisaddr == 0L ) {
			switch(s->ip_type) {
				case prot_UDP:
					if (s->datahandler) {
						HCALL(EOF,0,0,0,s);
					} else {
						udp_close(s);
					}
					break;
				case prot_TCP:
					tcp_abort(s);
			}
		}
		s=sp;
	}
}

