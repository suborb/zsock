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
 * $Id: zsock.h,v 1.7 2002-10-08 19:39:58 dom Exp $
 *
 */

#ifndef CYBIKO
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#ifndef SCCZ80
#include <errno.h>
#endif
#else
#include "cybiko.h"
#include "cybiko_error.h"
#endif

#include "portability.h"
#include "config.h"

#include <net/netstats.h>
#include <net/tcpsock.h>
#include <net/inet.h>
#include <net/hton.h>
#include <net/device.h>
#ifdef SCCZ80
#include <net/zsockerrs.h>
#endif
#include <net/zsfiles.h>

#include "nfuncs.h"




#define ZSOCKVERSION $0100       /* Version to check memory */
#define PACK_VERSION $0100
#define ZSOCK_INT    $0A15
#define ZSOCK0STORE  $468        /* Our byte of memory in bank 0 */
#define PKG_MAX       52

#define SANITY       0x73CA	 /* Socket sanity s(Z|128) */

#define INTMALLOC      1         /* Hack to avoid prototype in malloc.h */

#define FLUSHLIM      128        /* Amount in TCP buffer before flush(hack) */
#define UDPBUFSIZ     512        /* Standard size for UDP socket buffers */
#define MAXNAMESERV     2        /* Number of name servers */
#define MAXDOMSIZ      50        /* Max length of domain */

#define MAXPINGS        5        /* Max buffer slots of ping messages */



#define tcp_TIMEOUT          5   /* Various TCP timers */
#define tcp_LONGTIMEOUT     20
#define tcp_RETRANSMITTIME   5
#define RETRAN_STRAT_TIME  100



#define MAXVJSA	          5000   /* TCP VJ constants */
#define MAXVJSD           1000


/* Some moderately standard defines */
#ifndef EOF
#define EOF -1
#endif

#ifndef NULL
#define NULL    ((void *)0)
#endif



/* System Data Structure */
#ifdef Z80
#pragma asm
DEFVARS 0 {
	version ds.w	1
	page0	ds.b	1
	page1	ds.b	1
	page2	ds.b	1
}
#pragma endasm
#endif

struct sys {
	u16_t	   version;	/* Marker */
	u8_t	   page0;	/* 8192 - 16384 */
	u8_t	   page1;	/* 16384 - 32768 */
	u8_t	   page2;       /* 32768 - 49152 */
        tcpport_t      tcpport;
        tcpport_t      udpport;
        u16_t      icmpseq;
        u16_t      ipseq;
        ipaddr_t   myip;
        u8_t      *pktin;
	u8_t	   debug;
	u16_t	   overhead;
        TCPSOCKET *tcpfirst;
        UDPSOCKET *udpfirst;
	u16_t	   mss;
	u8_t	   numnameserv;
	ipaddr_t   nameservers[MAXNAMESERV];
	u8_t	   domainname[MAXDOMSIZ];
	TCPSOCKET *commssocket;
	int      (*usericmp)();	/* User ICMP (internal) */
	int	   catchall;	/* Package code to catch all */
	u32_t	   timeout;	/* For rexmit loop */
	u8_t	   pad;
};

extern struct pktdrive *device;      /* Pointer to device */
extern struct pktdrive z88slip;      /* Default device */
extern struct pktdrive z88ppp;       /* Default device */
extern struct sys sysdata;           /* System info */
extern struct sysstat_s netstats;    /* Netstat stats */



/* Message types that are sent to daemons with datahandlers */
#define handler_CLOSED  0
#define handler_ABORT   1
#define handler_OPEN    2       /* internal only */
#define handler_DATA    3       /* data waiting.. */


/* Message types */
#define MSG_PEEK   0x02         /* Look at what the incoming data is */


/* Structure for getting conn info */
struct sockinfo {
	u8_t	  protocol;
        ipaddr_t  local_addr;
        tcpport_t local_port;
        ipaddr_t  remote_addr;
        tcpport_t remote_port;
        u8_t      ttl;
};

/* Internal data representation for getxx services */
struct data_entry {
	u8_t	*name;
	tcpport_t port;
	u8_t	protocol;
};
