/*
 *      IP Routines for Small C+ Demo TCP stack
 *
 *      $Id: ip.c,v 1.6 2002-06-08 16:26:03 dom Exp $
 */


#include "zsock.h"




#ifdef CYBIKO
static unsigned long loopbackip = 0x007f0100;
#else
static ipaddr_t loopbackip = IP_ADDR(127,0,0,1);
#endif





/* Decode a received packet, and pack of to the handlers */
void PktRcvIP(void *buf,u16_t len)
{
    ip_header_t *ip = buf;

    /* Check the IP version */
    if ( ((ip->version)&240) != 0x40 ) 
	return;

    if ( ip->dest != sysdata.myip && ip->dest !=loopbackip )
	return;
#ifdef NETSTAT
    ++netstats.ip_recvd;
    netstats.ip_recvlen+=len;
#endif

    /* We're not too fond of fragments */
    if (ip->frag && ip->frag != htons(IP_DF) )
	return;

    /* Checksum packet, if non-zero then it's duff */
    if ( ip_check_cksum(buf) ) {
#ifdef NETSTAT
	++netstats.ip_badck;
#endif
	return;
    }

    /* Check the length in header is the same as length received */
 
    if ( len != htons(ip->length) ) {
#ifdef NETSTAT
	++netstats.ip_badlen;
#endif
	return;
    }

#ifdef INTERFACES
    if ( netif_check_addr(ip->dest) == 0 ) {   /* Check interfaces */
	return;
    }
#endif


    /* FIXME: "Dec TTL" and check if it's for us */
    if ( ip->ttl == 1 ) {
	icmp_send(ip,DEST_UNREACH,PORT_UNREACH,NULL);
	return;
    }

#ifdef __Z88__
    if ( sysdata.catchall )
	if (PackageCall(buf,len,sysdata.catchall) ) 
	    return;
#endif

    switch (ip->protocol) {
        
    case prot_TCP:
#ifdef NETSTAT
	++netstats.tcp_recvd;
#endif
	tcp_handler(buf,len);
	break;

    case prot_ICMP:
#ifdef NETSTAT
	++netstats.icmp_recvd;
#endif
	icmp_handler(buf,len);
	break;

    case prot_UDP:
#ifdef NETSTAT
	++netstats.udp_recvd;
#endif
	udp_handler(buf,len);
	break;
    case prot_IGMP:
    default:
	icmp_send(ip,DEST_UNREACH,PROT_UNREACH,NULL);
	break;
    }
}

/*
 *
 *      Send an IP Packet - no check for local addresses or anything
 *      like that yet, just shovel it off to the SLIP driver
 *
 *	Check for myip and localhost added 19/12/99
 */

void ip_send(ip_header_t *ip,u16_t len,u8_t prot, u8_t ttl)        
{
    u8_t	binding;
#ifdef NETSTAT
    ++netstats.ip_sent;
#endif
    ip->version  = 0x45;
    ip->tos      = 0;
    ip->ident    = htons(sysdata.ipseq++);
    ip->frag     = 0;
    ip->protocol = prot;
    ip->ttl      = ttl;
    inet_cksum_set(ip);   /* Check sum it */
    /* Have to located correct interface then checksum the packet */

    if (ip->dest == sysdata.myip || ip->dest == loopbackip ) {
	loopback_send(ip,len);
    } else {       /* Send it out to the device */
	binding=PageDevIn();
	device->queuefn(ip,len);
	PageDevOut(binding);
    }
}








