/*
 *
 *      Defs for Small C+ TCP stack
 *      These are standard defs such as structures..
 *
 *	This is used by the system only...
 *
 *	Do not include this yourself!!!
 *
 *	This file is a complete mess! 
 *
 */

/* Version string used to check memory stuff */

#define INTMALLOC 1
#define _KERNEL 1
#define ZSOCKVERSION $0100
#define PACK_VERSION $0100
#define ZSOCK_INT    $0A15
/* Where are magic byte is! */
#define ZSOCK0STORE $468
#define PKG_MAX 50

/* Socket sanity */

#define SANITY 0x73CA		/* s(Z|128) */

#include <net/tcpsock.h>
#include <net/inet.h>

#ifndef EOF
#define EOF -1
#endif

#define YES 1
#define NO  0

#define WORD    unsigned int
#define BYTE    unsigned char
#define LWORD   unsigned long
#define PACKETSIZE 600
#define FLUSHLIM 128
#define UDPBUFSIZ 512
#define MAXNAMESERV 2
#define MAXDOMSIZ 50

#include "funcs.h"


#define TRUE    1
#define FALSE   0
#ifndef NULL
#define NULL    (void *)0
#endif

/* Structure for the pkt drivers */

#include <net/device.h>
                
/* System Data */

#pragma asm
DEFVARS 0 {
	version ds.w	1
	page0	ds.b	1
	page1	ds.b	1
	page2	ds.b	1
}
#pragma endasm

struct sys {
	WORD	version;	/* Marker */
	BYTE	page0;		/* 8192 - 16384 */
	BYTE	page1;		/* 16384 - 32768 */
	BYTE	page2;		/* 32768 - 49152 */
        WORD    tcpport;
        WORD    udpport;
        WORD    icmpseq;
        WORD    ipseq;
        LWORD   myip;
        BYTE    *pktin;
	BYTE	debug;
	WORD	overhead;
        TCPSOCKET  *tcpfirst;
        UDPSOCKET  *udpfirst;
	WORD	mss;
	BYTE	numnameserv;
	LWORD	nameservers[MAXNAMESERV];
	BYTE	domainname[MAXDOMSIZ];
	TCPSOCKET *commssocket;
	int (*usericmp)();	/* User ICMP (internal) */
	int	catchall;	/* Package code to catch all */
	BYTE	counter;	/* For rexmit loop */
	BYTE	pad;
};

extern struct sys sysdata;

/* Stuff for netstat */

#include <net/netstats.h>

extern struct sysstat_s netstats;


/* Timeout for udp daemons.. */

#define TIMEOUT (-1)

#define tcp_TIMEOUT 5
#define tcp_LONGTIMEOUT 20
#define tcp_RETRANSMITTIME 5
#define RETRAN_STRAT_TIME 100

#define MAXVJSA	5000
#define MAXVJSD 1000


/*
 *      Defines for datahandler
 */


#define handler_CLOSED  0
#define handler_ABORT   1
#define handler_OPEN    2       /* internal only */
#define handler_DATA    3       /* data waiting.. */

/*
 * Number of pings to send out before barfing...
 * ..and complaining about buffer space..
 */

#define MAXPINGS 5

/*
 * Internal data representation for getxx services
 */

struct data_entry {
	u8_t	*name;
	tcpport_t port;
	u8_t	protocol;
};

/*
 * Where the config files are kept
 */

#define DOMAIN_FILE     ":ram.0/resolv.cfg"
#define HOSTNAME_FILE   ":ram.0/hostname"
#define DEVICE_FILE     ":ram.0/netdev"

/*
 * Include busy info
 */

#include "config.h"

#include <net/zsockerrs.h>
