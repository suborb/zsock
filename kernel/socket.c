/*
 *      Socket Routines For ZSock
 *      
 *      djm 5/12/99
 *
 *      This is an attempt to provide a unified interface
 *      so the app can be both UDP and TCP based..eg
 *      for internal daemons
 *
 *	djm 11/1/2000
 *	return_c sets carry flag on return
 *	return_nc clears carry flag on return
 *
 *	carry indicates an error at high level
 *
 *	Needless to say these are non-standard C!
 *
 *	12/2/2000 Optimized a little
 *
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
			return_nc 1;
		case prot_TCP:
			return_nc (s->state >= tcp_stateESTAB);
		case CONN_CLOSED:
			return_nc 0;
	}
	return_c EPROTONOSUPPORT;
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
			return_nc 0;
		case prot_TCP:
	 		return_nc( s->state >= tcp_stateFINWT1 && s->recvoffs == 0  );
		case CONN_CLOSED:
			return_nc 1;
	}
	return_c EPROTONOSUPPORT;
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
	return_c EPROTONOSUPPORT;
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
	return_c EPROTONOSUPPORT;
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
	return_c EPROTONOSUPPORT;
}

/*
 *	Set timeout on a socket
 *
 *	Sockets are arranged very similarly...
 */

void  sock_settimeout(s,time)
	TCPSOCKET *s;
	u16_t	time;
{
	switch (s->ip_type) {
		case prot_TCP:
		case prot_UDP:
			s->timeout=set_ttimeout(time);
			return_nc 0;
	}
	return_c EPROTONOSUPPORT;
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
			return_nc (chk_timeout(s->timeout));
	}
	return_c EPROTONOSUPPORT;
}

/*
 * 	User timeout routines
 */

u32_t user_settimeout(int secs)
{
	return_nc (set_ttimeout(secs));
}

u32_t user_setctimeout(int csecs)
{
	return_nc (set_timeout(csecs));
}

int user_chktimeout(u32_t time)
{
	return_nc (chk_timeout(time));
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
	return_nc (resolve_i(name));
}

int  reverse_addr_lookup(ipaddr_t addr, char *name)
{
	return_nc (reverse_addr_lookup_i(addr,name));
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
	return_nc (getxxbyname(ip_protocols,name));
#else
	return_nc ( getxxbyname(NULL,name));
#endif
}

u8_t *getprotobynumber(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_nc (getxxbyport(ip_protocols,port,store));
#else
	return_nc (getxxbyport(NULL,port,store));
#endif
}

/*
 *	Networks
 */

u8_t	getnetbyname(char *name)
{
#if 0
	return_nc (getxxbyname(ip_networks,name));
#else
	return_nc (getxxbyname(NULL,name));
#endif
}

u8_t *getnetbynumber(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_nc (getxxbyport(ip_networks,port,store));
#else
	return_nc (getxxbyport(NULL,port,store));
#endif
}

/*
 *	Services
 */

tcpport_t getservbyname(char *name)
{
#if 0
	return_nc (getxxbyname(ip_services,name));
#else
	return_nc (getxxbyname(NULL,name));
#endif
}

u8_t *getservbyport(port,store)
	tcpport_t port;
	u8_t	*store;
{
#if 0
	return_nc (getxxbyport(ip_services,port,store));
#else
	return_nc (getxxbyport(NULL,port,store));
#endif
}

u8_t getservprotobyname(char *name )
{
#if 0
        struct data_entry *search=ip_services;

        while (search->name) {
                if (!strcmp(search->name, name)) {
                        return_nc search->protocol;
                }
                search++;
        }
#endif
        return_nc 0;
}

u8_t getservprotobyport(port )
        tcpport_t port;
{
#if 0
        struct data_entry *search=ip_services;

        while (search->name) {
                if (search->port == port) {
                        return_nc search->protocol;
                }
                search++;
        }
#endif
	return_nc 0;
}

size_t GetNetStats(buf,len)
	u8_t *buf;
	u16_t len;
{
	if (len == 0 ) 
	    len = sizeof(struct sysstat_s);
	if (buf) memcpy(buf,netstats,len);
	return_nc len;
}


/*
 * Resolving functions 
 */

ipaddr_t inet_addr(u8_t *name)
{
	return_nc(inet_addr_i(name));
}

u8_t inet_ntoa(ipaddr_t ip, u8_t *buf)
{
	return_nc(inet_ntoa_i(ip,buf));
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
	return_nc(malloc(len));
}

void *tcp_calloc(size_t num, size_t sz)
{
	return_nc(calloc(num,sz));
}

/*
 * Register catchall handler
 */

int tcp_RegCatchAll(func)
	int func;
{
	int func2=sysdata.catchall;
	sysdata.catchall=func;
	return_nc(func2);
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
			(UDPSOCKET *)s->user=ptr;
			break;
		case prot_TCP:
			s->appptr=ptr;
			break;
		default:
			return_c EPROTONOSUPPORT;
	}
	return_c 0;
}

/* Get the sockets user pointer (daemon) */

void *sock_getptr(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_nc (UDPSOCKET *)s->user ;
			break;
		case prot_TCP:
			return_nc ( s->appptr ) ;
			break;
		default:
			return_c EPROTONOSUPPORT;
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
			return_nc ( 0 ) ;
			break;
		default:
			return_c EPROTONOSUPPORT;
	}
}

/* Resize the tcp input buffer (for slow readers) */

int sock_setrsize(s,sz)
	TCPSOCKET *s;
	int	sz;
{
	switch (s->ip_type) {
		case prot_TCP:
			return_nc (tcp_recvresize(s,sz) ); 
	}
	return_c EPROTONOSUPPORT;
}

/* Resize the tcp output buffer (for slow readers) */

int sock_setssize(s,sz)
	TCPSOCKET *s;
	int	sz;
{
	switch (s->ip_type) {
		case prot_TCP:
			return_nc (tcp_sendresize(s,sz) ); 
	}
	return_c EPROTONOSUPPORT;
}

/* Set the mode of a UDP socket to checksum or not etc */

int sock_setmode(s,mode)
	UDPSOCKET *s;
	int	mode;
{
	switch (s->ip_type) {
		case prot_UDP:
			s->sockmode=mode;
			return_nc 0;
	}
	return_c EPROTONOSUPPORT;
}

/* Wait for a socket to be open */

int sock_waitopen(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_nc 1;
		case prot_TCP:
			if (s->state==tcp_stateCLOSED) return_c ETIMEDOUT;
			while (getk() != 3) {	/* ^C */
#ifdef BUSY_VERSION
				Interrupt();
#endif
				if (s->state>=tcp_stateFINWT1) return_nc 0;
				if (s->state>=tcp_stateESTAB) return_nc 1;
			}
			return_nc -1;
	}
	return_c EPROTONOSUPPORT;
}

/* Wait for a socket to be closed */

int sock_waitclose(s)
	TCPSOCKET *s;
{
	switch (s->ip_type) {
		case prot_UDP:
			return_nc 1;
		case prot_TCP:
			while (getk() != 3) {	/* ^C */
#ifdef BUSY_VERSION
				Interrupt();
#endif
				if (s->state>=tcp_stateFINWT1 ) return_nc 1;
			}
			return_nc -1;	/* Error */
	}
	return_c EPROTONOSUPPORT;
}


int kill_daemon(port,protocol)
	tcpport_t port;
	unsigned char protocol;
{
	switch (protocol) {
		case prot_TCP:
			kill_socket(port,sysdata.tcpfirst);
			return_nc 0;
		case prot_UDP:
			kill_socket(port,sysdata.udpfirst);
			return_nc 0;
	}
	return_c EPROTONOSUPPORT;
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
			return 0;
	}
	return_c -1;
}
