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
 * $Id: tcp_int.c,v 1.4 2002-05-13 20:00:48 dom Exp $
 *
 */


#include "zsock.h"

extern char version[];


static int     service_quote(u8_t *addr,u16_t code,TCPSOCKET *s);
static void    service_udpecho(u8_t *addr,i16_t len,
			       ip_header_t *ip, 
			       udp_header_t *up,
			       UDPSOCKET *s);
static int     service_echo(u8_t *addr,u16_t code,TCPSOCKET *s);
static int     service_finger(u8_t *addr,u16_t code,TCPSOCKET *s)  ;



void service_registertcp()
{
    tcp_listen(0L,17,service_quote,0,0);
    tcp_listen(0L,7,service_echo,0,0);
    tcp_listen(0L,79,service_finger,0,0);
}

void service_registerudp()
{
    udp_open(0,7,0,service_udpecho,0);
}



/*
 *      Our datahandlers are called with three parameters
 *      datahandler(socket, addr, code)
 *
 *      if addr == 0 then obey code
 *      if addr != 0 then data length code at address addr
 *
 *	These now use generic sock_* routines so with
 *	a different wrapper can work for UDP as well
 */

/*
 *      Here's the first service - a simple qotd message
 */

static int service_quote(u8_t *addr,u16_t code,TCPSOCKET *s)      
{
	//printk("Quote called %d %d %d\n",addr,code,s);
    if ( (addr == 0) && code == handler_OPEN) {
	/* We've just been opened, so send something quick! */
	sock_puts_i(s,version);
	sock_close_i(s);
    }
    return(code);
}

/*
 * All echo does is the chuck the data back out that it got, so when
 * called, if addr && code we can write it back again..cheap and
 * easy function!
 */

static void service_udpecho(u8_t *addr,i16_t len,
			    ip_header_t *ip,
			    udp_header_t *up,
			    UDPSOCKET *s)	
{
    UDPSOCKET *n;

    if ( (n=udp_open(ip->source,htons(s->myport),htons(up->srcport),0,0) ) == 0 ) 
	return;
    service_echo(addr,len,(TCPSOCKET *)n);
    udp_close(n);
}



static int service_echo(u8_t *addr,u16_t code,TCPSOCKET *s) 
{
	//printk("Echo called: %d type=%d port =%d %d %d\n",s,s->ip_type,htons(s->hisport),addr,code);
    if ( addr && code ) {
	sock_write_i(s,addr,code);
    }
    return(code);
}
                

/*
 *      Another embarassingly simple server
 *      This one is for finger, all it does is reply that no one
 *      is logged in..
 */

static int service_finger(u8_t *addr,u16_t code,TCPSOCKET *s)  
{
    u16_t    count;

    count = code;
    if ( addr && code ) {
	while ( count--) {
	    if (*addr == 13 || *addr==10 ) {
		sock_puts_i(s,"No one logged on\n\r");
		sock_close_i(s);
		break;
	    } else {
		addr++;
	    }
	}
    }
    return(code);
}


