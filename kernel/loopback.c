/*
 *	Routines to handle the loopback device
 *
 *	This is all very unpleasant..we stack things
 *	up in a queue and shovel them into the device
 *	As we need to..
 *
 *	djm Late Dec 1999 - Hacked from slip.c
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
