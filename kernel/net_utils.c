/*
 *	Utility Routines For ZSock
 *
 *	djm 29/1/2000
 *
 *	Contains inet_ntoa
 *		 inet_addr
 *
 * 	Here we go...
 */

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "zsock.h"


u8_t *inet_ntoa_i(ipaddr_t, u8_t *);
ipaddr_t __FASTCALL__ inet_addr_i(u8_t *cp);
static u16_t inet_aton(u8_t *, ipaddr_t *);

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Changed for Small C+ - because of we're romable we have to supply
 * a pointer to where we want to be filled.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)inet_ntoa.c 5.5 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */

u8_t *inet_ntoa_i(in,b)
        ipaddr_t in;
        u8_t *b;
{
        register u8_t *p;

        p = (u8_t *)&in;
        sprintf(b, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        return (b);
}

/*
 * Seriously(!) Hacked around by djm 24/2/99 to compile under Small C
 * Cut down, only accepts base 10 and assumes 4 digits..this halves
 * the size..sacrifice is probably worth it!
 */

/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 *
 * CHANGE: returns 0 on failure!
 */

ipaddr_t __FASTCALL__ inet_addr_i(cp)
        u8_t *cp;
{
        ipaddr_t val;

        if (inet_aton(cp, &val))
                return (val);
        return (0);
}

/* 
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * Mega cut down version for Small C+ - this one assumes you give
 * 4 digits, also assumes base 10 if not returns failure
 */

static u16_t inet_aton(cp, addr)
        unsigned char *cp;
        ipaddr_t *addr;
{
        register unsigned char c;
        unsigned char parts[4];
        u16_t val;
        unsigned char *pp;
        pp = parts;

        for (;;) {
                /*
                 * Collect number up to ``.''. always decimal
                 */
                val = 0;
                while ((c = *cp) != '\0') {
                        if (isdigit(c)) {
                                val = (val * 10) + (c - '0');
                                cp++;
                                continue;
                        }
                        break;
                }
                if (*cp == '.') {
                        /*
                         * Internet format:
                         *      a.b.c.d
                         *      a.b.c   (with c treated as 16-bits)
                         *      a.b     (with b treated as 24 bits)
                         */
                        if (pp >= parts + 3 || val > 0xff)
                                return (0);
                        *pp++ = (char) val, cp++;
                } else
                        break;
        }
        /*
         * Check for trailing characters.
         */
        if ( pp != parts+3 ) return 0;
        if (*cp && (!isascii(*cp) || !isspace(*cp)))
                return (0);
        if ( val > 0xff) return(0);
        if (addr) {
                pp=addr;
                *pp=parts[0];
                *(++pp)=parts[1];
                *(++pp)=parts[2];
                *(++pp)=parts[3];
                *pp=(char) val;
        }
        return (1);
}


/*
 *	The getxxbyyy routines
 *
 *	djm 8/1/2000
 */


/* Some of the common port numbers */
struct data_entry ip_services[] = {
	{ "echo",	 7 ,prot_TCP },
	{ "qotd",	17 ,prot_TCP },
//	{ "chargen",	19 ,prot_TCP },
        { "ftp",        21 ,prot_TCP },
        { "ftp-data",   20 ,prot_TCP },
        { "telnet",     23 ,prot_TCP },
        { "smtp",       25 ,prot_TCP },
	{ "tftp",	69 ,prot_UDP },
	{ "finger",	79 ,prot_TCP },
        { "www",        80 ,prot_TCP },
//	{ "pop-2",	109 ,prot_TCP },
        { "pop-3",      110 ,prot_TCP },
        { 0,           0,0 }
};

struct data_entry ip_protocols[] = {
	{ "ip",		0, 0 },
	{ "icmp",	1, 0 },
//	{ "igmp",	2, 0 },
	{ "tcp",	6, 0 },
	{ "udp", 	17,0 },
	{ 0,		0,0}
};

struct data_entry ip_networks[] = {
//	{ "arpa",	10 ,0 },
	{ "arpanet",	10 ,0 },
//	{ "loop",	127 , 0 },
	{ "loopback",	127 ,0 },
	{ 0, 		0,0 }
};






tcpport_t getxxbyname(struct data_entry *type, char *name )
{
        struct data_entry *search;
	search=type;

        while (search->name) {
                if (!strcmp(search->name, name)) {
                        return search->port;
                }
                search++;
        }
        return 0;
}

char *getxxbyport(type, port, store_in )
	struct data_entry *type;
        tcpport_t port;
        char *store_in;
{
        struct data_entry *search;
	search=type;

        while (search->name) {
                if (search->port == port) {
                        strcpy(store_in, search->name);
                        return store_in;
                }
                search++;
        }
        /* Didnt find - just return the value */
        sprintf(store_in, "%u", port);
        return store_in;
}




