/*
 *      TCP Internal Daemons
 *
 * 	6/12/99 (djm) Made them generic so at least echo *can*
 *	work for udp..but how to test?!?!?
 */

#include <net/hton.h>
#include <stdio.h>
#include "zsock.h"

extern char version[];



/*
 *      Short routine to register services
 */

int QuoteServer();
int EchoServer();
int FingerDaemon();
int UDPEchoServer();


RegisterServicesTCP()
{
        tcp_listen(0L,17,QuoteServer,0,0);
        tcp_listen(0L,7,EchoServer,0,0);
        tcp_listen(0L,79,FingerDaemon,0,0);
}

#ifdef UDP_INTERNAL
RegisterServicesUDP()
{
	udp_open(0,7,0,UDPEchoServer,0);
}
#endif



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

QuoteServer(addr,code,s)
        BYTE   *addr;
        WORD    code;
        TCPSOCKET *s;
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

#ifdef UDP_INTERNAL
UDPEchoServer(addr,len,ip,up,s)
	BYTE	*addr;
	WORD	len;
	struct ip_header *ip;
	struct udp_header *up;
	UDPSOCKET *s;
{
	UDPSOCKET *n;

	if ( (n=udp_open(ip->source,htons(s->myport),htons(up->srcport),0,0) ) == 0 ) return;
	EchoServer(addr,len,n);
	udp_close(n);
}
#endif



EchoServer(addr,code,s)
        BYTE   *addr;
        WORD    code;
        TCPSOCKET *s;
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

FingerDaemon(addr,code,s)
        BYTE   *addr;
        WORD    code;
        TCPSOCKET *s;
{
        WORD    count;

        count=code;
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
