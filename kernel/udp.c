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
 * $Id: udp.c,v 1.4 2002-05-13 20:00:48 dom Exp $
 *
 */




#include "zsock.h"



#define UDP_LENGTH      8

#define DEBUGPF(x)

extern void RegisterServicesUDP();


static UDPSOCKET    *udp_new_socket();

ipaddr_t udpbroadcast = IP_ADDR(255,255,255,255);

/*
 *      Initialise UDP services
 */

void udp_init()
{
        sysdata.udpport=1024;
        sysdata.udpfirst=0;
	service_registerudp();
}

/*
 *      Open a UDP connection to a host
 */

UDPSOCKET *udp_open(ipaddr_t ipdest,
		    tcpport_t lport,tcpport_t dport,
		    int (*datahandler)(),u8_t type)
{
        UDPSOCKET  *s;
	if (lport && (ipdest == 0 ) && CheckPort(sysdata.udpfirst,lport) )
		return_c(EADDRINUSE,NULL);
	if ( lport == 0 ) {
		if ( get_uniqport(sysdata.udpfirst,&sysdata.udpport) )
			return_c(EADDRINUSE,NULL);
		lport = sysdata.udpport;
	}
        if ( (s=udp_new_socket()) == 0 ) return_c(ENOMEM,NULL);  /* Find a new sock */
#if 0
        if (lport == 0 ) lport=++sysdata.udpport;
#endif
#ifdef ROUTING
	s->myaddr = ip_route_addr(ipdest);
#else
        s->myaddr=sysdata.myip;
#endif
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
        return_ncv(s);
}


void udp_close(UDPSOCKET *ds)
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


void udp_handler(ip_header_t *ip,u16_t length)
{
    u8_t         *dp;
    i16_t         len;
    udp_header_t *up;
    UDPSOCKET    *s;



    up = (udp_header_t *)((u8_t *)ip + (ip->version&15)*4);


    /* Calculate length of UDP data */
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
	    icmp_send(ip,DEST_UNREACH,PORT_UNREACH,NULL);
	return;        /* Not matched */
    }

    if ( up->cksum) 
	s->sockmode|=UDPMODE_CKSUM; /* Switch to cksum mode */
    if ( inet_cksum_pseudo(ip,up,prot_UDP,len)){
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
	dp = (void *) up;
	dp += UDP_LENGTH;
	if (s->datahandler) {
/* Datahandler installed, Call it */
#ifdef Z80
	    HCALL(dp,len,ip,up,s);
#else
	    s->datahandler(dp,len,ip,up,s);
#endif
	} else {
/* Copy it into the buffer, not bothering with overlapping data */
	    if ( len > s->recvsize ) len=s->recvsize;
	    memcpy(s->recvbuff,dp,len);
	    s->recvoffs=len;
	    s->recvread=0;
	}
    }
}

/*
 *      Read from UDP socket, returns number of bytes
 *      read
 */

int udp_read(UDPSOCKET *s, void *dp, int len,u8_t flags)
{
    int	i;
    if  (s->recvoffs && s->recvbuff ) {
	if ( len > (i=s->recvoffs-s->recvread) ) 
	    len = i;
	if (len) {
	    memcpy(dp,s->recvbuff+s->recvread,len);
	    if ( (flags&MSG_PEEK) == MSG_PEEK )
		return len;
	    s->recvread+=len;
	    if (s->recvread>=s->recvoffs) {
		s->recvoffs=0;
		s->recvread=0;
	    }
	}
	return(len);
    } 
    return 0;
}
                

int udp_write(UDPSOCKET *s, void *dp, int len)
{
    struct pktdef {
        ip_header_t  ip;
        udp_header_t udp;
        u8_t         data;
    } *pkt;


    if ( (pkt = pkt_alloc(sizeof (struct pktdef)-1 + len) ) == NULL ) 
	return_c(ENOMEM,-1);
#ifdef NETSTAT
    ++netstats.udp_sent;
    netstats.udp_sentlen+=len;
#endif
   

    pkt->ip.length = htons(sizeof(ip_header_t) + sizeof(udp_header_t)+len );

    /* udp header */
    pkt->udp.srcport = s->myport;
    pkt->udp.dstport = s->hisport;
    pkt->udp.cksum = 0;
    pkt->udp.length=htons(len+UDP_LENGTH);
    memcpy(&pkt->data,dp,len);

    pkt->ip.tos = s->tos;
    pkt->ip.source = s->myaddr;
    pkt->ip.dest = s->hisaddr;
    /* Do the UDP checksum if required */
    if (s->sockmode&UDPMODE_CKSUM)
            pkt->udp.cksum = inet_cksum_pseudo(&pkt->ip, &pkt->udp,prot_UDP, len+UDP_LENGTH )  ;
    ip_send((ip_header_t *)pkt,ntohs(pkt->ip.length),prot_UDP,s->ttl);
    return_ncv(len);
}




/* Create a new UDP socket */
static UDPSOCKET *udp_new_socket()
{
    UDPSOCKET *s;
    if ( ( s = calloc(sizeof(UDPSOCKET),1) ) == NULL ) 
	return NULL;
    s->ip_type = prot_UDP;
    /* Put this socket into the queue */
    s->next = sysdata.udpfirst;
    sysdata.udpfirst = s;
    return(s);
}



/* Check timeouts on UDP socket... */
void udp_check_timeouts()
{
        UDPSOCKET *s;
        for ( s = sysdata.udpfirst; s; s = s->next ) {
            if ( s->timeout && chk_timeout( s->timeout )) {
                /* Close the UDP socket */
                if ( s->datahandler ) {
#ifdef Z80
		    HCALL(EOF,0,0,0,s);
#else
		    s->datahandler(EOF,0,0,0,s);
#endif
		} else {
		    udp_close(s);
		}
                break;
            }
        }
}

/* Get a unique port for this protocol */
int get_uniqport(void *t,tcpport_t *pick)
{
    int	p = ++(*pick);

    while ( p != *pick ) {
	if ( CheckPort(t,p) == 0 ) {
	    *pick = p;
	    return 0;
	}
	p++;
	if ( p < 1024 )
	    p = 1024;
    }
}


/* Check whether a socket has this port */
int CheckPort(void *t, tcpport_t src)
{
	UDPSOCKET *s = t;
	while (s) {
		if (s->myport==htons(src)) return 1;
		s=s->next;
	} 
	return 0;
}

