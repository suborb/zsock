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
 * $Id: generic.c,v 1.3 2002-05-13 21:30:22 dom Exp $
 *
 * Cross platform routines
 */



#include "zsock.h"
#include <sys/time.h>

/* Returns number of 10ms since start of day */
u32_t current_time()
{
    struct timeval tv;
    struct timezone tz;
    long  millisec;
    
    gettimeofday(&tv,&tz);
 
    millisec = tv.tv_sec * 100 + ( tv.tv_usec / 10000 );

    return millisec;
}

/* Return a sequence number of a TCP connection */
u32_t GetSeqNum()
{
    struct timeval tv;
    struct timezone tz;
    long  millisec;
    
    gettimeofday(&tv,&tz);

    return ( tv.tv_usec );   /* Not great, but okay-ish */
}


/* I think this is correct... */
u16_t inet_cksum_pseudo(ip_header_t *ip,void *phdr,u8_t protocol,u16_t length)      
{
    u16_t  acc;
    tcp_header_t *tcp = phdr;

    acc = inet_cksum(&protocol,1);

    acc += inet_cksum(&ip->source,8);
    acc += inet_cksum(tcp,length);

    return acc;
}

u16_t ip_check_cksum(ip_header_t *buf)
{
    u16_t cksum;
    cksum = inet_cksum(buf,(buf->version&15) << 2 );
    return cksum;
}

void inet_cksum_set(ip_header_t *buf)
{
    buf->cksum = 0;
    buf->cksum = htons(inet_cksum(buf,(buf->version&15) << 2 ));
}


u16_t inet_cksum(void *data, u16_t len)
{
    u16_t acc;
    u16_t *buf = data;

  
    for(acc = 0; len > 1; len -= 2) {
	acc += *buf;
	if(acc < *buf) {
	    /* Overflow, so we add the carry to acc (i.e., increase by
	       one). */
	    ++acc;
	}
	++buf;
    }

    /* add up any odd byte */
    if ( len == 1 ) {
	acc += htons(((u16_t)(*(u8_t *)buf)) << 8);
	if(acc < htons(((u16_t)(*(u8_t *)buf)) << 8)) {
	    ++acc;
	}
    }


    return htons(~acc);
}
