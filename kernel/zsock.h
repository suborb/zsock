/*
 *   Main ZSock Header File
 *
 *   $Id: zsock.h,v 1.2 2002-05-11 21:00:55 dom Exp $
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <net/netstats.h>
#include <net/tcpsock.h>
#include <net/inet.h>
#include <net/hton.h>
#include <net/device.h>
#include <net/zsockerrs.h>
#include <net/zsfiles.h>

#include "nfuncs.h"
#include "config.h"




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

enum { NO = 0, YES };
enum { FALSE = 0, TRUE };



/* System Data Structure */
#pragma asm
DEFVARS 0 {
	version ds.w	1
	page0	ds.b	1
	page1	ds.b	1
	page2	ds.b	1
}
#pragma endasm

struct sys {
	u16_t	  version;	/* Marker */
	u8_t	   page0;	/* 8192 - 16384 */
	u8_t	   page1;	/* 16384 - 32768 */
	u8_t	   page2;       /* 32768 - 49152 */
        u16_t      tcpport;
        u16_t      udpport;
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
	u8_t	   counter;	/* For rexmit loop */
	u8_t	   pad;
};

extern struct pktdrive *device;      /* Pointer to device */
extern struct pktdrive z88slip;      /* Default device */
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
