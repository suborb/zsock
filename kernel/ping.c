/*
 *	Routine to Send a Ping to Specified Host
 *
 * 	This routine works by registering an ICMP handler
 *	to catch ICMP replies and then parsing the reply
 *	to match an output request..any traceroute routine
 *	could employ a similar method except by sending
 *	a UDP message.
 *
 *	djm 19/12/99
 *
 *	Although this is an "app" routine we utilise kernel
 *	routines directly.
 */


#include "zsock.h"




struct pingbuf {
	u8_t	used;
	u32_t	sequence;
	u32_t	xmittime;
} pingstat[MAXPINGS];

static u16_t	received;
static u16_t	pingmin;
static u16_t	pingav;
static u16_t	pingmax;


static int      ping_handler(ip_header_t *ip,icmp_header_t *icmp);
static int      ping_send(ipaddr_t addr,u32_t *ident);

void Ping()
{
    char	buffer[80];
    char	buffer2[20];
    u16_t	i;
    u16_t	sequence;
    u32_t	ident;
    u32_t	timeout;
    ipaddr_t  addr;
    
    printf("Enter hostname/IP to ping\n");

    fgets_cons(buffer,79);
    if ( (addr=resolve_i(buffer) ) == 0L ) {
	printf("\nUnknown host\n");
	return;
    }

    

    memset(pingstat,0,sizeof(pingstat));
    
    ident = sequence = 0;
    received = pingav = pingmin = pingmax = 0;
    
    if ( icmp_register(ping_handler) == NULL ) {
	printf("\nCan't register handler..\n");
	return;
    }
    
    printf("\nPING %s (%s): 56 data bytes\n",buffer,inet_ntoa_i(addr,buffer2));


    timeout = set_timeout(50);
    for (;;) {
	if (chk_timeout(timeout) ) {
	    timeout = set_timeout(100);
	    if ( ping_send(addr,&ident) ) 
		ident++;
	    else
		break;
	} 
#ifdef Z88
	if (getk() == 27 ) break;
#else
	if ( ident == 4 )
	    break;
#endif
	BUSYINT();
   }


    icmp_deregister();
    /* Just to test the resolver.... */
    if (reverse_addr_lookup_i(addr,buffer) == 0 )
	inet_ntoa_i(addr,buffer);
    printf("\n--- %s ping statistics ---\n",buffer);
    printf("%lu packets transmitted, %d packets received, %d%% packet loss\n",ident,received, 100-((received*100)/ident)  );
    printf("round-trip time min/avg/max= %d/%d/%d ms\n",pingmin,(pingav*10)/received,pingmax);
    return;
}

static int ping_send(ipaddr_t addr,u32_t *ident)	
{
    int	i;

    for	(i = 0; i < MAXPINGS; i++) {
	if (pingstat[i].used == 0 ) {
	    pingstat[i].xmittime = icmp_ping_pkt(addr,ident,36);
	    if (pingstat[i].xmittime) {
		pingstat[i].sequence = *ident;
		pingstat[i].used = 1;
		return 1;
	    }
	}
    }
    printf("PING: no buffer space available\n");
    return 0;
}



static int ping_handler(ip_header_t *ip,icmp_header_t *icmp)	
{
    char	buffer[18];
    u32_t	*ptr;
    u32_t	times;
    int	        rtt;
    int	        i;

    if (icmp->type != ECHO_REPLY ) 
	return 1;

    times = current_time();

    ptr = &icmp->unused;

    for (i = 0; i < MAXPINGS; i++) {
	if (*ptr == pingstat[i].sequence) {
	    pingstat[i].used = 0;
	    rtt = (times - pingstat[i].xmittime);
	    pingav += rtt;
	    rtt *= 10;
	    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n",htons(ip->length),inet_ntoa_i(ip->source,buffer),(int)(icmp->unused),ip->ttl, rtt );
	    received++;
	    if (rtt < pingmin || pingmin==0) 
		pingmin = rtt;
	    if (rtt >pingmax || pingmax==0) 
		pingmax = rtt;
	    return 0;
	}
    }
    return 1;
}


