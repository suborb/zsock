/*
 * Copyright (c) 1999-2002 Dominic Morris
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Dominic Morris.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the ZSock TCP/IP stack.
 *
 * $Id: socket_int.c,v 1.4 2002-06-08 16:26:03 dom Exp $
 *
 * Routines used by the internal daemons
 */



#include "zsock.h"



/*
 *      Write a stream of bytes to a socket
 */

int  sock_write_i(TCPSOCKET *s,u8_t *dp,u16_t len) 
{
    switch (s->ip_type) {
    case prot_UDP:
	return_ncv(udp_write((UDPSOCKET *)s,dp,len));
    case prot_TCP:
	return_ncv(tcp_write(s,dp,len));
    }
    return_c(EPROTONOSUPPORT,-1);
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
	return_ncv(s->recvoffs-s->recvread);
    }
    return_c(EPROTONOSUPPORT,-1);
}

int  sock_read_i(
		 TCPSOCKET       *s,
		 u8_t            *dp,
		 u16_t           len,
		 u8_t            flags)
{
    switch (s->ip_type) {
    case prot_UDP:
	return_ncv(udp_read((UDPSOCKET *)s,dp,len,flags));
    case prot_TCP:
    case CONN_CLOSED:
	return_ncv(tcp_read(s,dp,len,flags));
    }
    return_c(EPROTONOSUPPORT,-1);
}


/* Kill daemon sockets matching myport we supply a base socket address */

void kill_socket(tcpport_t myport,TCPSOCKET *s)
{
	TCPSOCKET *sp;
	for (;;) {
	    if ( s == (TCPSOCKET *)NULL ) 
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

