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

#include <stdio.h>
#include <string.h>
#include <net/hton.h>
#include <net/misc.h>
#include "zsock.h"



extern LWORD set_timeout(int);
extern LWORD resolve_i(char *);


struct pingbuf {
	BYTE	used;
	LWORD	sequence;
	LWORD	xmittime;
} pingstat[MAXPINGS];

u16_t	received;
u16_t	pingmin;
u16_t	pingav;
u16_t	pingmax;

static int PingHandler();



void Ping()
{
	char	buffer[80];
	char	buffer2[20];
	u16_t	i;
	u16_t	sequence;
	u16_t	ident;
	LWORD	timeout;
	ipaddr_t  addr;

	printf("Enter hostname/IP to ping\n");

	fgets_cons(buffer,79);

	if ( (addr=resolve_i(buffer) ) == NULL ) {
		printf("\nUnknown host\n");
		return;
	}


	memset(pingstat,0,sizeof(pingstat));

	received=ident=sequence=0;
	pingav=pingmin=pingmax=0;

	if ( RegisterICMPHandler(PingHandler) ==0 ) {
		printf("\nCan't register handler..\n");
		return;
	}

	printf("\nPING %s (%s): 56 data bytes\n",buffer,inet_ntoa_i(addr,buffer2));
	
	timeout=set_timeout(50);
	for (;;) {
		if (chk_timeout(timeout) ) {
			timeout=set_timeout(100);
			if ( SendPing(addr,&ident) ) 
				ident++;
		} 
		if (getk() == 27 ) break;
		BUSYLOOP();
	}

	DeRegisterICMPHandler();
/* Just to test the resolver.... */
	if (reverse_addr_lookup_i(addr,buffer) == 0 )
		inet_ntoa_i(addr,buffer);
	printf("\n--- %s ping statistics ---\n",buffer);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n",ident,received, 100-((received*100)/ident)  );
	printf("round-trip time min/avg/max= %d/%d/%d ms\n",pingmin,(pingav*10)/received,pingmax);
	return;
}

SendPing(addr,ident)
	ipaddr_t addr;
	LWORD	*ident;
{
	int	i;

	for	(i=0;i<MAXPINGS;i++) {
		if (pingstat[i].used == 0 ) {
			pingstat[i].xmittime=PkPing(addr,ident,36);
			if (pingstat[i].xmittime) {
				pingstat[i].sequence=*ident;
				pingstat[i].used=1;
				return 1;
			}
		}
	}
	printf("PING: no buffer space available\n");
	return 0;
}



int PingHandler(ip,icmp)
	struct ip_header *ip;
	struct icmp_header *icmp;
{
	char	buffer[18];
	LWORD	*ptr;
	LWORD	time;
	int	rtt;
	int	i;

	if	(icmp->type != ECHO_REPLY ) return 1;

	time=current();

	ptr=&icmp->unused;

	for (i=0;i<MAXPINGS;i++) {
		if (*ptr==pingstat[i].sequence) {
			pingstat[i].used=0;
			rtt=(time-pingstat[i].xmittime);
			pingav+=rtt;
			rtt*=10;
			printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n",htons(ip->length),inet_ntoa_i(ip->source,buffer),(int)(icmp->unused),ip->ttl, rtt );
			received++;
			if (rtt < pingmin || pingmin==0) pingmin=rtt;
			if (rtt >pingmax || pingmax==0) pingmax=rtt;
			return 0;
		}
	}
	return 1;
}


