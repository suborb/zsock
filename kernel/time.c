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
 * $Id: time.c,v 1.2 2002-05-13 20:00:48 dom Exp $
 *
 * Timer checking
 *
 */




#include "zsock.h"


#define OZDAY 100*60*60*24

/*
 * OZSPLIT.. is used as a bit of a fudge...
 * ..to handle midnight problems...
 */

#define OZSPLIT1 100*60*5

/*
 *	Set the timeout, entry in seconds, exit in
 *	oz units
 */

u32_t set_ttimeout(int secs)
{
	return (current_time()+((u32_t)(secs*100)));
}

u32_t set_timeout(int tensms)
{
	return (current_time()+tensms);
}



/*
 * Check the timer supplied to see if it has timed out
 * Return TRUE for timeout
 */

int chk_timeout(u32_t time)
{
	u32_t now;

	now=current_time();

/*
 * 	Bit of fudge time
 */
	if (time>OZDAY && now<OZSPLIT1) now+=OZDAY;


/*
 *	This will screw up big time over midnight...
 */
	if	(now > time ) return(1);
	return 0;
}

/*
 * Set various TCP timeout times 
 */

void SetTIMEOUTtime(TCPSOCKET *s)
{	
	s->timeout=set_ttimeout(tcp_TIMEOUT);
}

void SetLONGtimeout(TCPSOCKET *s)
{
	s->timeout=set_ttimeout(tcp_LONGTIMEOUT);
}


