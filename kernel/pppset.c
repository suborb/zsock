/*
 *	Routines used by device drivers to set generic config
 *	and other things..
 *
 *	This file contains routines to:
 *
 *	- Set/read the IP address
 *	- Read the default domain
 *	- Set the Nameservers
 *
 *	djm 29/1/2000
 */

/* sccz80 magic */
#ifdef Z80
#pragma -shared-file
#ifdef OLDPACK
#pragma -shareoffset=14
#else
#pragma -shareoffset=10
#endif
#endif

#include "zsock.h"

ipaddr_t GetHostAddr(void);
u8_t *GetDomain(char *);
ipaddr_t SetHostAddr(ipaddr_t);
size_t SetNameServers(ipaddr_t, ipaddr_t);

/* Get Our Current IP address */

ipaddr_t GetHostAddr(void)
{
    return (sysdata.myip);
}

/* Get our current domain */

u8_t *GetDomain(u8_t *buffer)
{
    strcpy(buffer,sysdata.domainname);
    return (buffer);
}


/* Set our IP address */

ipaddr_t SetHostAddr(ipaddr_t newip)
{
    sysdata.myip = newip;
    return (sysdata.myip);
}

/* Set our DNS servers (up to 2) */

size_t SetNameServers(ipaddr_t ns1, ipaddr_t ns2)
{
    if ( ns1 ) {
	sysdata.numnameserv = 1;
	sysdata.nameservers[0] = ns1;
	if ( ns2 ) {
	    ++sysdata.numnameserv;
	    sysdata.nameservers[1] = ns2;
	}
	return (sysdata.numnameserv);
    }
    sysdata.numnameserv=0;
    return 0;
}
