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
 * $Id: pppset.c,v 1.5 2002-06-01 21:43:18 dom Exp $
 *
 */


/* sccz80 magic */
#ifdef __Z88__
#pragma -shared-file
#ifdef OLDPACK
#pragma -shareoffset=14
#else
#pragma -shareoffset=10
#endif
#endif

#include "zsock.h"



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
