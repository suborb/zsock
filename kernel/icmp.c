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
 * $Id: icmp.c,v 1.5 2002-05-13 21:30:22 dom Exp $
 *
 * ICMP Routines
 */



#include "zsock.h"


static void           icmp_cancel(struct ip_header *ip,unsigned char type,u32_t new);

#ifdef Z80
static int __CALLEE__ icmp_fix_cksum(int cksum, int add);
#endif

/* An ICMP packet */
struct pktdef {
    ip_header_t    ip;
    icmp_header_t  icmp;       
};



/*
 * Register User ICMP Handler..eg for ping
 *
 * This is only used by app, so can get away with
 * removing the check..
 */


void *icmp_register(int (*fn)())
{
    sysdata.usericmp=fn;
    return(fn);
}

void icmp_deregister()
{
    sysdata.usericmp=0;
}

		    


/*
 *	Send an ICMP message to host ipaddr
 */

void icmp_send(ip_header_t *buf,u8_t type,u8_t code,u32_t *unused)	
{
    u16_t 	length;
    struct      pktdef *pkt;
  

/* Don't send ICMP in response to ICMP */
    if (buf->protocol == prot_ICMP){
#ifdef NETSTAT
	++netstats.icmp_icmp;
#endif
	return;
    }

    if ( (pkt = pkt_alloc(sizeof (struct pktdef)) ) == NULL) 
	return;

#ifdef NETSTAT
     ++netstats.icmp_sent;
     ++netstats.icmp_out[type];
#endif

    pkt->ip.length = htons(sizeof( struct pktdef) );

    memcpy(pkt->icmp.data,buf,28);


    pkt->ip.source  = buf->dest;
    pkt->ip.dest    = buf->source;
    /* Icmp header */
    pkt->icmp.type  = type;
    pkt->icmp.code  = code;
    pkt->icmp.cksum = 0;
    /* Do the arguments, 4 bytes long so fake as long! */
    if (unused) 
	pkt->icmp.unused = *unused;
    else 
	pkt->icmp.unused = 0L;
    /* ICMP checksum */
    pkt->icmp.cksum = htons(inet_cksum(&pkt->icmp,sizeof(struct icmp_header) ));
    ip_send((ip_header_t *)pkt,sizeof(struct pktdef),prot_ICMP,255);
}

/*
 *	Build a ping packet for host, with ident and sequence
 *	pointed to by unused
 *	Returns the time sent off to the interface
 *
 *	If len left blank then take default (48 data bytes)
 */

u32_t icmp_ping_pkt(ipaddr_t ipaddr,u32_t *unused,u16_t len)	
{
    u16_t	length;
    struct pktdef *pkt;
#ifndef Z80
    u8_t   *ptr;
    u8_t    data;
#endif

/* Sort the length out, we already have 28 bytes in the header */
    if ( len ) {
	length = len - 28 + sizeof(struct pktdef);
    } else { 
	length = sizeof(struct pktdef); 
	len=28; 
    }


    if ( (pkt = pkt_alloc(length) ) == NULL) 
	return 0L;

#ifdef NETSTAT
    ++netstats.icmp_sent;
    ++netstats.icmp_out[ECHO_REQUEST];
#endif

    pkt->ip.length = htons(length);
    pkt->ip.source = sysdata.myip;
    pkt->ip.dest   = ipaddr;
#ifdef Z80
    icmp_fill_ping(pkt->icmp.data,len);
#else
    ptr = pkt->icmp.data;
    data = 0x20;
    while (len--) {
        *ptr++ = data & 63;
        ++data;
    }
#endif
    /* Icmp header */
    pkt->icmp.type   = ECHO_REQUEST;
    pkt->icmp.code   = 0;
    pkt->icmp.cksum  = 0;
    pkt->icmp.unused = *unused;
    /* ICMP checksum */
    pkt->icmp.cksum  = htons(inet_cksum(&pkt->icmp,length-20 ));
    ip_send((ip_header_t *)pkt,length,prot_ICMP,255);
    return( current_time() );
}



void icmp_handler(void *buf,u16_t len)       
{
    u32_t          addr;
    u8_t	  *buf2;
    ip_header_t   *ip,*ret;
    icmp_header_t *icmp;

    ip   = buf;
    len  = htons(ip->length) - ((ip->version&15)*4);
    icmp = buf+ ((ip->version&15)*4);

    if ( inet_cksum(icmp,len) ) { 
#ifdef NETSTAT
	++netstats.icmp_badck;
#endif
	return;
    }
#ifdef NETSTAT
    ++netstats.icmp_inp[icmp->type];
#endif

    if (sysdata.usericmp) {
	if ( sysdata.usericmp(ip,icmp) == 0 ) 
	    return;
    }
    /* Point to the IP address in the returned packet */
    ret = (struct ip_header *) (icmp->data);
    switch (icmp->type) {
    case ECHO_REQUEST:        /* We've been asked for Ping */
	if ( (buf2 = pkt_alloc(htons(ip->length)) ) == NULL ) 
	    return;
#ifdef NETSTAT
	++netstats.icmp_sent;
	++netstats.icmp_out[ECHO_REPLY];
#endif
	icmp->type = ECHO_REPLY;
	addr = ip->dest;
	ip->dest = ip->source;
	ip->source = addr;
	icmp->cksum = 0;
	icmp->cksum = htons(inet_cksum(icmp,len));
	memcpy(buf2,buf,htons(ip->length));
	ip_send((ip_header_t *)buf2,htons(ip->length),prot_ICMP,255);
	break;
    case DEST_UNREACH:
	if ( icmp->code < 6 ) {
	    icmp_cancel(ret,1,0L);
	}
	break;
#ifdef NEED_QUENCH
    case QUENCH:
	icmp_cancel(ret,2,0L);
	break;
#endif
    case REDIRECT:
	if ( icmp->code < 4  )
	    icmp_cancel(ret,2,icmp->unused);
	break;
    }
    return;
}

static void icmp_cancel(struct ip_header *ip,unsigned char type,u32_t new)
{
    if ( ip->protocol == prot_TCP)
	tcp_cancel(ip,type,(ipaddr_t *)&new);
}


