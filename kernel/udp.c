/*
 *      UDP Module for ZSock
 *
 *      Very, very crud(e)
 *
 *      djm 5/5/99
 *
 */

#include <stdio.h>
#include <net/hton.h>
#include "zsock.h"



#define UDP_LENGTH      8

#define DEBUGPF(x)


extern WORD TCPCheckSum();
extern void Move();
extern void *AllocatePkt(int);
extern void __FASTCALL__ *newsocket(int);
extern void __FASTCALL__ IPHeaderCheck(void *);
extern UDPSOCKET *NewUDPSocket();
extern void RegisterServicesUDP();
extern LWORD set_ttimeout(int);

ipaddr_t udpbroadcast = IP_ADDR(255,255,255,255);

/*
 *      Initialise UDP services
 */

UDPInit()
{
        sysdata.udpport=1024;
        sysdata.udpfirst=0;
        RegisterServicesUDP();
#ifdef MODEL2
        tftp_init();
#endif
}

/*
 *      Open a UDP connection to a host
 */

UDPSOCKET *udp_open(ipdest,lport,dport,datahandler,type)
        ipaddr_t ipdest;
        tcpport_t lport;
        tcpport_t dport;
        int (*datahandler)();
	BYTE	type;
{
        UDPSOCKET  *s;
	if (lport && (ipdest == 0 ) && CheckPort(sysdata.udpfirst,lport) )
		return_c EADDRINUSE;
        if ( (s=NewUDPSocket()) == 0 ) return_c (ENOMEM);  /* Find a new sock */
        if (lport == 0 ) lport=++sysdata.udpport;
        s->myaddr=sysdata.myip;
        s->myport=htons(lport);
        s->hisport=htons(dport);
        s->hisaddr=ipdest;
        s->datahandler=datahandler;
	s->handlertype=type;
	s->sockmode=UDPMODE_CKSUM;	/* Default to checksum */
	s->ttl=255;			/* UDP default TTL */
	s->tos=0;			/* UDP default tos */
        if (s->datahandler == NULL )
/* No data handler instigate receiver buffer */
                if ((s->recvbuff=malloc(UDPBUFSIZ))!=NULL) s->recvsize=UDPBUFSIZ;
        return_nc(s);
}


udp_close(UDPSOCKET *ds)
{
        UDPSOCKET **sp,*s=ds;;

	if (s->ip_type != prot_UDP) return;
	
        sp = &sysdata.udpfirst;
        for (;;) {
                s = *sp;
                if ( s == ds ) {
                        *sp = s->next;
/* Mung the identifier */
			s->ip_type=BADIPTYPE;
/* This looks bad, but our free() handles null pointers! */
                        free(s->user);
                        free(s->recvbuff);
                        free(ds);
                        break;
                }
                if ( s == NULL ) break;
                sp = &s->next;
        }
        return;
}




/* 
 * UDP handler, enters with pointer to IP header
 *
 * This routine called by the IP layer
 */


udp_handler(ip,length)
        struct ip_header *ip;
        WORD    length;
{
        BYTE    *dp;
        i16_t     len;
        struct udp_header *up;
        UDPSOCKET *s;

/* 
 * Get the length of the Internet Header and point to tp to UDP header
 *
 * This is a bit kludgey, but the best we can do without casts!
 */

        up = (BYTE *)ip + (ip->version&15)*4;

/*
 * Calculate length of TCP data
 */
        len = ntohs(ip->length) - (ip->version&15)*4;


/* Check active sockets */


        for ( s = sysdata.udpfirst ; s ; s = s->next ) 
                if ( s->ip_type == prot_UDP && s->hisport &&
                     up->dstport == s->myport &&
                     up->srcport == s->hisport &&
                     ip->source == s->hisaddr) break;
/* Now for passive */
        if ( s == NULL ) {
                for ( s = sysdata.udpfirst ; s ; s = s->next ) {
                   if ( s->ip_type == prot_UDP && (s->hisaddr == 0 || s->hisaddr==udpbroadcast) && up->dstport == s->myport ) {
/* Should make a new socket here really! */
/*
                        s->hisaddr=ip->source;
                        s->hisport=up->srcport;
                        if ( ip->dest != udpbroadcast )
                                s->myaddr=ip->dest;
*/
                        break;
                   }
                }
        }

/* Now to broadcast (sigh) */
        if ( s == NULL ) {
                for ( s = sysdata.udpfirst ; s ; s = s->next ) 
                        if (s->hisaddr == udpbroadcast && (up->dstport==s->myport) )break;
        }

        if ( s == NULL ) {
/* Must send port unreachable (as per RFC) (except broadcast) */
		if ( ip->dest != udpbroadcast )
			SendICMP(ip,DEST_UNREACH,PORT_UNREACH,0);
                return;        /* Not matched */
        }

        if ( up->cksum) 
		s->sockmode|=UDPMODE_CKSUM; /* Switch to cksum mode */
                if (TCPCheckSum(ip,prot_UDP,len)){
#ifdef NETSTAT
			++netstats.udp_badck;
#endif
#ifdef DEBUGTFTPD
			if (sysdata.debug)
				printf("Bad UDP checksum, rejecting pkt\n");
#endif
                        return; /* Bad cksum */
                }

        if ( ( len -=UDP_LENGTH ) >0 ) {
#ifdef NETSTAT
		netstats.udp_recvlen+=len;
#endif
                dp=up;
                dp+=UDP_LENGTH;
                if (s->datahandler) {
/* Datahandler installed, Call it */
				HCALL(dp,len,ip,up,s);
                } else {
/* Copy it into the buffer, not bothering with overlapping data */
                        if ( len > s->recvsize ) len=s->recvsize;
                        Move(dp,s->recvbuff,len);
                        s->recvoffs=len;
			s->recvread=0;
                }
        }
}

/*
 *      Read from UDP socket, returns number of bytes
 *      read
 */

int udp_read(UDPSOCKET *s, BYTE *dp, int len)
{
	int	i;
        if  (s->recvoffs && s->recvbuff ) {
                if ( len > (i=s->recvoffs-s->recvread) ) len = i;
		if (len) {
                	Move(s->recvbuff+s->recvread,dp,len);
			s->recvread+=len;
			if (s->recvread>=s->recvoffs) {
				s->recvoffs=0;
				s->recvread=0;
			}
		}
                return(len);
        } else 
                return 0;
}
                

int udp_write(UDPSOCKET *s, BYTE *dp, int len)
{
    struct pktdef {
        struct ip_header ip;
        struct udp_header udp;
        BYTE    data;
    } *pkt;


    if ( (pkt = AllocatePkt(sizeof (struct pktdef)-1 + len) ) == NULL ) return_c ENOMEM;
#ifdef NETSTAT
    ++netstats.udp_sent;
    netstats.udp_sentlen+=len;
#endif
   

    pkt->ip.length = htons(sizeof( struct ip_header) + sizeof(struct udp_header)+len );

    /* udp header */
    pkt->udp.srcport = s->myport;
    pkt->udp.dstport = s->hisport;
    pkt->udp.cksum = 0;
    pkt->udp.length=htons(len+UDP_LENGTH);
    Move(dp,&pkt->data,len);

    FillHeader((struct ip_header *)pkt,prot_UDP,s->ttl);
    pkt->ip.tos=s->tos;
    pkt->ip.source = s->myaddr;
    pkt->ip.dest = s->hisaddr;
/* Do the IP header */
    IPHeaderCheck(&pkt->ip);
/* Do the UDP header, perhaps this should be optional? */
    if (s->sockmode&UDPMODE_CKSUM)
            pkt->udp.cksum =TCPCheckSum(&pkt->ip, prot_UDP, len+UDP_LENGTH )  ;
    SendPacket(pkt,ntohs(pkt->ip.length));
    return_nc(len);
}



/*
 * Allocate some memory for a UDP Socket
 */

UDPSOCKET *NewUDPSocket()
{
        UDPSOCKET *s;
        if (s=calloc(sizeof(UDPSOCKET),1 ) == 0 ) return 0;
        s->ip_type=prot_UDP;
/* Put this socket into the queue */
        s->next=sysdata.udpfirst;
        sysdata.udpfirst=s;
        return(s);
}

/*
 * Set a timeout on a UDP socket
 */

void SetUDPTimeout(s,time)
        UDPSOCKET *s;
        WORD    time;
{
        s->timeout=set_ttimeout(time);
}

/*
 * Check timeouts on UDP socket...
 */

void CheckUDPtimeouts()
{
        UDPSOCKET *s;
        for ( s = sysdata.udpfirst; s; s = s->next ) {
            if ( s->timeout && chk_timeout( s->timeout )) {
                /* Close the UDP socket */
                if ( s->datahandler )
                        HCALL(EOF,0,0,0,s);
		else 
                	udp_close(s);
                break;
            }
        }
}


/*
 * Check whether a source port is unique
 */

int CheckPort(UDPSOCKET *t, tcpport_t src)
{
	UDPSOCKET *s=t;
	while (s) {
		if (s->myport==htons(src)) return 1;
		s=s->next;
	} 
	return 0;
}

