/*
 * Copyright (c) 2002 Dominic Morris
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
 * $Id: arp.c,v 1.1 2002-10-08 19:39:53 dom Exp $
 *
 */


#include <sys/types.h>
#include <net/inet.h>
#include "arp.h"

#define ARP_TABLE_SIZE 10

struct arp_entry {
    ipaddr_t        ipaddr;
    struct eth_addr ethaddr;
    u8_t            time;
};


static struct arp_entry arp_table[ARP_TABLE_SIZE];

static u8_t   counter;


void arp_init()
{
    int   i;

    for ( i = 0; i < ARP_TABLE_SIZE; i++ ) {
	arp_table[i].ipaddr = 0x00000000UL;
    }
}


void arp_add_entry(ipaddr_t ipaddr, struct eth_addr *ethaddr)
{
    u8_t   oldest = 0;
    int    here,i,lru;

    /* Find somewhere to put it, also try to find the oldest */
    for ( i = 0, here = -1; i < ARP_TABLE_SIZE; i++ ) {
	if ( arp_table[i].ipaddr ) {
	    if ( arp_table[i].ipaddr == ipaddr ) {   /* Previous entry */
		memcpy(arp_table[i].ethaddr,ethaddr,6);
		arp_table[i].time = counter;
		return;
	    } else if ( counter - arp_table[i].time > oldest ) {
		oldest = counter - arp_table[i].time;
		here = i;
	    }				
	} else {
	    here = i;
	}
    }

    arp_table[here].ipaddr = ipaddr;
    memcpy(arp_table[here].ethaddr,ethaddr,6);
    arp_table[here].time = counter;
    return;
}

/* FIXME: Need to pass in netif structure & ethernet interface */
int uip_arpin(struct netif *netif,struct eth_addr *ethaddr,u8_t *buf, int len)
{
    struct arp_hdr  *hdr;

    if ( len < sizeof(struct arp_hdr) ) {
	return 0;
    }

    hdr = buf;

    switch ( htons(hdr->opcode) ) {
    case ARP_REQUEST:
	if ( hdr->dipaddr == netif->ip_addr ) {     /* They want our address */
	    hdr->opcode = htons(ARP_REPLY);
	    hdr->dipaddr = hdr->sipaddr;
	    hdr->sipaddr = netif->ip_addr;

	    memcpy(hdr->dhwaddr,hdr->shwaddr,6);
	    memcpy(hdr->shwaddr,ethaddr,6);
	    memcpy(hdr->ethhdr.dest,hdr->dhwaddr,6);
	    memcpy(hdr->ethhdr.src,ethaddr,6);
	    hdr->hwtype = htons(HWTYPE_ETHERNET);
	    hdr->hwlen  = htons(6);
	    
	    hdr->proto  = htons(ETHTYPE_IP);
	    hdr->protolen = htons(sizeof(ipaddr_t));
	    
	    hdr->ethhdr.type = htons(ETHTYPE_AP);

	    return sizeof(struct arp_hdr);
	}
	break;
    case ARP_REPLY:
	if ( hdr->dipaddr,netif->ip_addr ) {
	    arp_add_entry(hdr->sipaddr,hdr->shwaddr);
	}
	break;
    }

    return 0;
}

struct eth_addr *arp_lookup(ipaddr_t ipaddr)
{
    int   i;

    for ( i = 0; i < ARP_TABLE_SIZE; i++ ) {
	if ( ipaddr == arp_table[i].ipaddr ) {
	    return &arp_table[i].ethaddr;
	}
    }
    return NULL;
}
