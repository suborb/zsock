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
 * $Id: socket.c,v 1.4 2002-05-13 20:00:48 dom Exp $
 *
 * API Routines
 */




#include "zsock.h"

/* Some sccz80 magic */
#ifdef SCCZ80
#ifdef OLDPACK
#pragma -shareoffset=14
#else
#pragma -shareoffset=10
#endif

#pragma -shared-file
#endif


extern ipaddr_t resolve_i(char *);
extern int reverse_addr_lookup_i(ipaddr_t, char *);

/*
 *      Write a stream of bytes to a socket
 */

int  sock_write(s,dp,len)
        TCPSOCKET       *s;
        u8_t            *dp;
        u16_t           len;
{
	return (sock_write_i(s,dp,len));
}

/*
 *	Write a byte to a socket..
 */

int  sock_putc(x,s)
	u8_t	x;
	TCPSOCKET *s;
{
	return(sock_write_i(s,&x,1));
}


/*
 *      Write a line of text to a socket
 */

int  sock_puts(s,dp)
        TCPSOCKET *s;
        u8_t    *dp;
{
        return (sock_write_i(s,dp,strlen(dp)));
}

/*
 *	Flush socket
 */

void  sock_flush(s)
	TCPSOCKET *s;
{
	if (s->ip_type == prot_TCP)
		tcp_flush(s);
}



/*
 *      Read from a socket
 */

int  sock_recv(s,dp,len,flags)
        TCPSOCKET       *s;
        u8_t            *dp;
        u16_t           len;
	u8_t		flags;
{
	return (sock_read_i(s,dp,len,flags));
}

int  sock_read(s,dp,len)
        TCPSOCKET       *s;
        u8_t            *dp;
        u16_t           len;
{
	return (sock_read_i(s,dp,len,0));
}

/*
 *      Close a socket
 */

void  sock_close(s)
        TCPSOCKET       *s;
{
	sock_close_i(s);
}

/*
 *      Abort a socket
 *      For UDP we just close...
 */

void  sock_abort(t)
        TCPSOCKET       *t;
{
	TCPSOCKET *s=t;
	switch (s->ip_type) {
		case prot_UDP:
                	udp_close((UDPSOCKET *)s);
			break;
		case prot_TCP:
		case CONN_CLOSED:
                	tcp_abort(s);
	}
}


/*
 *      Check whether data is ready..
 *
 *      Returns amount of data there..
 *
 *	Sockets are arranged the same so can do this..
 */

int sock_dataready(s)
	TCPSOCKET *s;
{
	return (sock_dataready_i(s));
}

/*
 * Query whether a socket is established
 */

int  sock_opened(t)
	TCPSOCKET *t;
{
	TCPSOCKET *s=t;
	switch (s->ip_type) {
		case prot_UDP:
			return_ncv(1);
		case prot_TCP:
			return_ncv (s->state >= tcp_stateESTAB);
		case CONN_CLOSED:
			return_ncv(0);
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* 
 * Query whether a socket is closed
 */

int  sock_closed(t)
	TCPSOCKET *t;
{
	TCPSOCKET *s=t;
	switch (s->ip_type) {
		case prot_UDP:
			return_ncv(0);
		case prot_TCP:
	 		return_ncv( s->state >= tcp_stateFINWT1 && s->recvoffs == 0  );
		case CONN_CLOSED:
			return_ncv(1);
	}
	return_c(EPROTONOSUPPORT,-1);
}

/*
 *	Open a socket for listening..
 */

void  *sock_pair_listen(ipaddr,lport,dport,datahandler,protocol)
	ipaddr_t ipaddr;
	tcpport_t lport;
	tcpport_t dport;
	int (*datahandler)();
	u8_t protocol;
{
	switch (protocol) {
		case prot_UDP:
			return(udp_open(ipaddr,lport,dport,datahandler,1));
		case prot_TCP:
			return(tcp_listen(ipaddr,lport,datahandler,1,0));
	}
	return_c(EPROTONOSUPPORT,NULL);
}

void  *sock_listen(ipaddr,lport,datahandler,protocol)
	ipaddr_t ipaddr;
	tcpport_t lport;
	int (*datahandler)();
	u8_t protocol;
{
	switch (protocol) {
		case prot_UDP:
			return(udp_open(ipaddr,lport,0,datahandler,1));
		case prot_TCP:
			return(tcp_listen(ipaddr,lport,datahandler,1,0));
	}
	return_c(EPROTONOSUPPORT,NULL);
}

/*
 *	Open a socket for connectioin
 */

void  *sock_open(ipaddr,dport,datahandler,protocol)
	ipaddr_t ipaddr;
	tcpport_t dport;
	int (*datahandler)();
	u8_t protocol;
{
	switch (protocol) {
		case prot_UDP:
			return(udp_open(ipaddr,0,dport,datahandler,1));
		case prot_TCP:
			return(tcp_open(ipaddr,0,dport,datahandler,1));
	}
	return_c(EPROTONOSUPPORT,NULL);
}

/*
 *	Set timeout on a socket
 *
 *	Sockets are arranged very similarly...
 */

int  sock_settimeout(s,time)
	TCPSOCKET *s;
	u16_t	time;
{
	switch (s->ip_type) {
		case prot_TCP:
		case prot_UDP:
			s->timeout=set_ttimeout(time);
			return_ncv(0);
	}
	return_c(EPROTONOSUPPORT,-1);
}
		
/*
 *	Check a timeout on a socket
 *
 *	Returns 1 if timed out..
 */

int  sock_chktimeout(t)
	TCPSOCKET *t;
{
	TCPSOCKET *s=t;
	switch (s->ip_type) {
		case prot_TCP:
		case prot_UDP:
			return_ncv (chk_timeout(s->timeout));
	}
	return_c(EPROTONOSUPPORT,-1);
}

/*
 * 	User timeout routines
 */

u32_t user_settimeout(int secs)
{
	return_ncv (set_ttimeout(secs));
}

u32_t user_setctimeout(int csecs)
{
	return_ncv (set_timeout(csecs));
}

int user_chktimeout(u32_t time)
{
	return_ncv (chk_timeout(time));
}


/*
 *	Shutdown a socket after it's been closed
 *	Only called by process with no daemon
 *	Called after sock_close so udp no effect..
 */

void  sock_shutdown(t)
	TCPSOCKET *t;
{
	TCPSOCKET *s=t;
	if	(s->ip_type == CONN_CLOSED ) {
		tcp_shutdown(s);
	} else  if (s->ip_type == prot_TCP ) {
	/* TCP socket not really closed yet 
  	 * so abort and then shutdown..
  	 */
		tcp_abort(s);
		tcp_shutdown(s);
	}
	return_nc;
}

/*
 *	Resolving functions
 */

ipaddr_t  resolve(char *name)
{
	return_ncv (resolve_i(name));
}

int  reverse_addr_lookup(ipaddr_t addr, char *name)
{
	return_ncv (reverse_addr_lookup_i(addr,name));
}



/*
 *	Hooks to the internal routines for the API
 *
 *	djm 8/1/2000
 */


/*
 *	Structures containing our info
 */

extern struct data_entry *ip_protocols;
extern struct data_entry *ip_services;
extern struct data_entry *ip_networks;

extern tcpport_t getxxbyname(struct data_entry *,u8_t *);
extern u8_t *getxxbyport(struct data_entry *, tcpport_t, u8_t *);


/*
 *	Protocols
 */

u8_t getprotobyname(char *name)
{
#if 0
	return_ncv (getxxbyname(ip_protocols,name));
#else
	return_ncv ( getxxbyname(NULL,name));
#endif
}

u8_t *getprotobynumber(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_ncv (getxxbyport(ip_protocols,port,store));
#else
	return_ncv (getxxbyport(NULL,port,store));
#endif
}

/*
 *	Networks
 */

u8_t	getnetbyname(char *name)
{
#if 0
	return_ncv (getxxbyname(ip_networks,name));
#else
	return_ncv (getxxbyname(NULL,name));
#endif
}

u8_t *getnetbynumber(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_ncv (getxxbyport(ip_networks,port,store));
#else
	return_ncv (getxxbyport(NULL,port,store));
#endif
}

/*
 *	Services
 */

tcpport_t getservbyname(char *name)
{
#if 0
	return_ncv (getxxbyname(ip_services,name));
#else
	return_ncv (getxxbyname(NULL,name));
#endif
}

u8_t *getservbyport(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_ncv (getxxbyport(ip_services,port,store));
#else
	return_ncv (getxxbyport(NULL,port,store));
#endif
}

u8_t getservprotobyname(char *name )
{
#if 0
        struct data_entry *search=ip_services;

        while (search->name) {
                if (!strcmp(search->name, name)) {
                        return_ncv search->protocol;
                }
                search++;
        }
#endif
        return_ncv(0);
}

u8_t getservprotobyport(port )
        tcpport_t port;
{
#if 0
        struct data_entry *search=ip_services;

        while (search->name) {
                if (search->port == port) {
                        return_ncv search->protocol;
                }
                search++;
        }
#endif
	return_ncv(0);
}

size_t GetNetStats(buf,len)
	u8_t *buf;
	u16_t len;
{
	if (len == 0 ) 
	    len = sizeof(struct sysstat_s);
	if (buf) memcpy(buf,&netstats,len);
	return_ncv(len);
}


/*
 * Resolving functions 
 */

ipaddr_t inet_addr(u8_t *name)
{
	return_ncv(inet_addr_i(name));
}

u8_t *inet_ntoa(ipaddr_t ip, u8_t *buf)
{
	return_ncv(inet_ntoa_i(ip,buf));
}

/*
 * Malloc routines
 */

void tcp_free(void *ptr)
{
	free(ptr);
	return_nc;
}

void *tcp_malloc(size_t len)
{
	return_ncv(malloc(len));
}

void *tcp_calloc(size_t num, size_t sz)
{
	return_ncv(calloc(num,sz));
}

/*
 * Register catchall handler
 */

int tcp_RegCatchAll(func)
	int func;
{
	int func2=sysdata.catchall;
	sysdata.catchall=func;
	return_ncv(func2);
}

/*
 * Some more I thought of later...
 */

/* Set the sockets user pointer (daemon) */

int sock_setptr(s,ptr)
	TCPSOCKET *s;
	void	*ptr;
{
	switch (s->ip_type) {
		case prot_UDP:
			((UDPSOCKET *)s)->user=ptr;
			break;
		case prot_TCP:
			s->appptr=ptr;
			break;
		default:
			return_c(EPROTONOSUPPORT,-1);
	}
	return_ncv(0);
}

/* Get the sockets user pointer (daemon) */

void *sock_getptr(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_ncv( ((UDPSOCKET *)s)->user) ;
			break;
		case prot_TCP:
			return_ncv ( s->appptr ) ;
			break;
		default:
			return_c(EPROTONOSUPPORT,NULL);
	}
}

/* Set the handler for a socket (good for daemons) */

int sock_sethandler(s,f)
	TCPSOCKET *s;
	void	*f;	/* I know it's not really!! */
{
	switch (s->ip_type) {
		case prot_UDP:
		case prot_TCP:
		case CONN_CLOSED:
			s->datahandler=f;
			return_ncv ( 0 ) ;
			break;
		default:
			return_c(EPROTONOSUPPORT,-1);
	}
}

/* Resize the tcp input buffer (for slow readers) */

int sock_setrsize(s,sz)
	TCPSOCKET *s;
	int	sz;
{
	switch (s->ip_type) {
		case prot_TCP:
			return_ncv (tcp_recvresize(s,sz) ); 
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* Resize the tcp output buffer (for slow readers) */

int sock_setssize(s,sz)
	TCPSOCKET *s;
	int	sz;
{
	switch (s->ip_type) {
		case prot_TCP:
			return_ncv (tcp_sendresize(s,sz) ); 
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* Set the mode of a UDP socket to checksum or not etc */

int sock_setmode(s,mode)
	UDPSOCKET *s;
	int	mode;
{
	switch (s->ip_type) {
		case prot_UDP:
			s->sockmode=mode;
			return_ncv(0);
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* Wait for a socket to be open */

int sock_waitopen(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_ncv(1);
		case prot_TCP:
			if (s->state==tcp_stateCLOSED) return_c(ETIMEDOUT,-1);
			while (getk() != 3) {	/* ^C */
#ifdef BUSY_VERSION
				Interrupt();
#endif
				if (s->state>=tcp_stateFINWT1) return_ncv(0);
				if (s->state>=tcp_stateESTAB) return_ncv(1);
			}
			return_ncv(-1);
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* Wait for a socket to be closed */

int sock_waitclose(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_ncv(1);
		case prot_TCP:
			while (getk() != 3) {	/* ^C */
#ifdef BUSY_VERSION
				Interrupt();
#endif
				if (s->state>=tcp_stateFINWT1 ) return_ncv(1);
			}
			return_ncv(-1);	/* Error */
	}
	return_c(EPROTONOSUPPORT,-1);
}


int kill_daemon(port,protocol)
	tcpport_t port;
	unsigned char protocol;
{
	switch (protocol) {
		case prot_TCP:
			kill_socket(port,sysdata.tcpfirst);
			return_ncv(0);
		case prot_UDP:
			kill_socket(port,(TCPSOCKET *)(sysdata.udpfirst));
			return_ncv(0);
	}
	return_c(EPROTONOSUPPORT,-1);
}

/* Set type of service */

void sock_settos(s,tos)
	TCPSOCKET *s;
	u8_t	tos;
{
	switch (s->ip_type) {
		case prot_UDP:
		case prot_TCP:
			s->tos=tos;
	}
}

/* Set ttl on a socket */

void sock_setttl(s,ttl)
	TCPSOCKET *s;
	u8_t 	ttl;
{
	switch (s->ip_type) {
		case prot_UDP:
		case prot_TCP:
			s->ttl=ttl;
	}
}

int sock_getinfo(TCPSOCKET *t, struct sockinfo *sock)
{
	TCPSOCKET *s = t;
	switch (s->ip_type ) {
		case prot_UDP:
		case prot_TCP:
			sock->protocol = s->ip_type;
			sock->ttl = s->ttl;
#if 1
			memcpy(&sock->local_addr,&s->myaddr,12);
#else
			sock->local_addr = s->myaddr;
			sock->remote_addr = s->hisaddr;
			sock->remote_port = s->hisport;
			sock->local_port = s->myport;
			sock->remote_port = s->hisport;
#endif
			return_ncv(0);
	}
	return_c(EPROTONOSUPPORT,-1);
}
