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
 * $Id: loopback.c,v 1.4 2002-05-13 20:00:48 dom Exp $
 *
 * Simple loopback network interface
 */



#include "zsock.h"



/*
 *	Variables used by this module
 */

struct _loopback {
    void *next;
    int   len;
};

static struct _loopback	*localfirst;
static struct _loopback	*locallast;

#ifdef Z80
int loopback_init()
{
    locallast = localfirst = NULL;
    return sizeof(struct _loopback);
}

void loopback_send(void *buf, u16_t length)
{
#pragma asm
        pop     de
        pop     bc
        pop     hl
        push    hl
        push    bc
        push    de

;Store a local packet in the queue
;Entry:   hl=addy of packet
;         bc=packet length

          dec  hl
          dec  hl
          dec  hl
          dec  hl
;Okay, have an address in hl for the packet now...
          ld   de,(_locallast)     ;old last position
          ld   (_locallast),hl     ;address of last packet
          ld   a,d
          or   e
          jp   z,localsend1         ;no last packet
          ex   de,hl
          ld   (hl),e
          inc  hl
          ld   (hl),d
          ex   de,hl
.localsend6
          ld   (hl),0              ;mark end of packet queue
          inc  hl                  ;
          ld   (hl),0
          inc  hl
          ld   (hl),c              ;length of packet
          inc  hl
          ld   (hl),b              ;hl is nonzero, exit OK
          ret
;This is is if the packet is first in queue
.localsend1
	  ld	(_localfirst),hl
          jr    localsend6 ;copy the packet
                              ;hl is non zero so exit OK
#pragma endasm
}

void loopback_recv()
{
#pragma asm
	ld	hl,(_localfirst)
	ld	a,h
	or	l
	ret	z
	ld	e,(hl)	;next packet
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	c,(hl)	;length packet
	inc	hl
	ld	b,(hl)
	ld	(_localfirst),de
	ld	a,d
	or	e
	jr	nz,getlocal1
;Last packet in queue, reset locallast
	ld	(_locallast),de
.getlocal1
	inc	hl
	push	hl
	push	bc
	call	_PktRcvIP
	pop	bc	;Leave packet on stack for _FreePacket
	call	_pkt_free
	pop	hl
#pragma endasm
}
#else

int loopback_init()
{
    locallast = localfirst = NULL;
    return sizeof (struct _loopback);
}

void loopback_send(void *buf, int length)
{
    struct _loopback *loop;
    struct _loopback *llast;

    loop = ( buf - sizeof(struct _loopback));
    llast = locallast;
    locallast = loop;
    if ( llast != NULL ) {
	llast->next = loop;
    } else {
	localfirst = loop;
    }
    loop->next = NULL;
    loop->len  = length;      
}

void loopback_recv()
{
    struct _loopback *loop;
    void             *pkt;

    loop - localfirst;

    if ( loop == NULL )
	return;

    localfirst = loop->next;

    if ( loop->next == NULL )
	locallast = NULL;

    pkt = loop + sizeof(struct _loopback);
    PktRcvIP(pkt,loop->len);
    pkt_free(pkt);
}
#endif
