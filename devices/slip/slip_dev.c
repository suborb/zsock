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
 * $Id: slip_dev.c,v 1.3 2002-10-08 19:39:56 dom Exp $
 *
 */




#include <sys/types.h>
#include <net/inet.h>

/* Pick up busy style */
#include "config.h"

#ifdef PLUGIN
#define SLIPPKT _pktin
#include <net/device.h>
#else
#define SLIPPKT (_pktin)
#define PACKETSIZE 600
#include <net/device.h>
#endif

/*
 * Plugin has a buffer preset..
 */

#define MINHDR		20
#ifndef PLUGIN
#define MAXPKTSZ	576
#else
#define MAXPKTSZ	1500
#endif

/*
 * Slip escape sequences, these are sorted out by the preproc
 *
 * SP_END     = end of packet
 * SP_ESC     = byte stuff coming up
 * SP_ESC_END = preceded by ESC means END datatype
 * SP_ESC_ESC = preceded by ESC means ESC datatype
 */

#define SP_END          192
#define SP_ESC          219
#define SP_ESC_END      220
#define SP_ESC_ESC      221


#ifdef BUSY_VERSION
#define NUMCALLS 25
#else
#define NUMCALLS 1
#endif




extern int SlipInit();
extern void SlipSendPkt();
extern int SlipSend();
extern int SlipRead(void **pktin);
extern void *SlipReturnPkt();
extern void SlipOnline();
extern void SlipOffline();
extern void SlipStatus();

/*
 *	Variables used by this module
 */

	u8_t	slipstat;
	u16_t	sliplen;
	u16_t	slippos;
	u16_t	slippack;
	u16_t	sliplast;
	u8_t	slipesch;
	u16_t	inslippos;
	u16_t	insliplen;
	u8_t	inslipflag;
	u8_t	online;
#ifndef PLUGIN
	void	*pktin;
#endif

/*
 * Our little structure which contains all the info needed for the
 * SLIP driver!
 */
 

struct pktdrive z88slip = {
        "ZS0PKTDRV",                            /* Magic */
        "SLIP",                                 /* Type of driver */
#ifdef PLUGIN
	"ZSock SLIP device v1.0 djm 2000",
#else
        "ZSock Z88 SLIP Driver by Dom",         /* (C) etc */
#endif
        SlipInit,                       /* Initialisation Routine */
        SlipSendPkt,                    /* Add to out-queue */
        SlipSend,                       /* Send func */
        SlipRead,                        /* Read func */
	SlipOnline,			/* Online */
	SlipOffline,			/* Offline */
	SlipStatus,			/* Status */
};




/*
 * Below here are internal functions and data for the driver, the code
 * upstream should never call these directly"
 */

/*
 * Return where the packet is...
 */

void *SlipReturnPkt()
{
#ifdef PLUGIN
#asm
	ld	hl,_pktin
#endasm
#else
	if (pktin) return pktin;
	pktin = pkt_alloc(PACKETSIZE);
	return (pktin);
#endif
}

void SlipStatus()
{
#asm
	ld	hl,(_online)
	ld	h,0
#endasm
}



void SlipOnline()
{
#asm
	ld	a,1
	ld	(_online),a
#endasm
}

/*
 * Turn device offline
 * If flag != 0 then hangup line
 * If flag == 0 then don't hangup
 */

void SlipOffline(int flag)
{
#asm
	xor	a
	ld	(_online),a
#endasm
}



int SlipRead(void **packet)
{
#asm
	ld	hl,0	;no packet
	ld	a,(_online)
	and	a		;device offline
	ret	z
        ld      b,NUMCALLS
._slipread1
        push    bc
        call    inchar
        pop     bc
	jr      nc,gotpacket
        djnz    _slipread1
        ld      hl,0
	ret
; This is where we have to say where the packet starts
.gotpacket
	ex	de,hl	;get len safe into de
	pop	bc	;return address
	pop	hl	;
	push	de
	ld	de,SLIPPKT
	ld	(hl),e
	inc	hl
	ld	(hl),d
	pop	de	
	push	hl
	push	bc
	ex	de,hl
#endasm
}

/*
 *	We quit if we have finished sending a packet and need
 *	it to be freed. If the device is offline we keep things
 *	in the queue and wait till we're online again
 */

int SlipSend()
{
#asm
	ld	hl,0
	ld	a,(_online)
	and	a	;offline
	ret	z
        ld      b,NUMCALLS
._slipsend1
        ld   hl,_slipstat   ;check to see if we should send
        ld   a,(hl)
        rrca
	jr	nc,send_out	;return with hl=0 -> dont free
	push	bc
        call    outchar
        pop     bc      
	ret	nc
        djnz    _slipsend1
.send_out
	ld	hl,0
#endasm
}


#asm


; Variables used by the Slipcode (will eventually be placed in the
; system structure..

#ifdef BLAHBLAHPLUGIN
;Sending


.slipstat defb   0         ;bit 0=send
                           ;bit 1=sent initial END
                           ;bit 2=send ESCced character
                           ;bit 3=send final end
.sliplen  defw   0
.slippos  defw   0
.slippack defw   0
.sliplast defw   0
.slipesch defb   0

;Receiving

.inslippos     defw   0
.insliplen     defw   0
.inslipflag    defb   0    ;bit =escaped

; General status
.online		defb  0
#endif



; C entry for the send packet function, on stack
; return address, length, address
; This routine simply places the packet in the queue


._SlipSendPkt
        pop     de
        pop     bc
        pop     hl
        push    hl
        push    bc
        push    de

;Store a slip packet in the queue
;Entry:   hl=addy of packet
;         bc=packet length

;Exit:    hl=0 no room for it..

          dec  hl
          dec  hl
          dec  hl
          dec  hl
;Okay, have an address in hl for the packet now...
          ld   de,(_sliplast)     ;old last position
          ld   (_sliplast),hl     ;address of last packet
          ld   a,d
          or   e
          jp   z,slipsend1         ;no last packet
          ex   de,hl
          ld   (hl),e
          inc  hl
          ld   (hl),d
          ex   de,hl
.slipsend6
          ld   (hl),0              ;mark end of packet queue
          inc  hl                  ;
          ld   (hl),0
          inc  hl
          ld   (hl),c              ;length of packet
          inc  hl
          ld   (hl),b              ;hl is nonzero, exit OK
          ret
;This is is if the packet is first in queue
.slipsend1
          ld   (_sliplen),bc
          ld   (_slippack),hl
          call slipsend6 ;copy the packet
          inc  hl
          ld   (_slippos),hl
          ld   a,1
          ld   (_slipstat),a   ;indicate data, and need to send END
                              ;hl is non zero so exit OK
          ret
          

;---------------------------------------------------------

;
; Routine that sends the packet byte by byte, called in spurts of
; 20 by our code, but other drivers may vary..
;
; Enters with a=slipstat
; Exits with c+hl = free packet;
;	     nc   = keep calling me pls..

.outchar 
;          ld   hl,_slipstat   ;check to see if we should send
;         ld   a,(hl)
;          rrca
;          ret  nc
          rrca
          jp   c,outchar0
          ld   a,SP_END     ;flush out noise
          call sendchar
          ret  c
          set  1,(hl)
	  scf
          ret
.outchar0 
          rrca
          jp   nc,outcharb
;Okay..need to escape here...
          ld   a,(_slipesch)
          call sendchar
          ret  c
          res  2,(hl)
          ld   hl,(_slippos)
          jp   outchar3
.outcharb 
          rrca
          jp   nc,outcharc
          ld   a,SP_END
          call sendchar
          ret  c
          res  3,(hl)
          jp   outchard
.outcharc ld   hl,(_slippos)
          ld   a,(hl)
          cp   SP_END
          jp   nz,outchar1
          ld   a,SP_ESC
          call sendchar
          ret  c
.outchar9 
          ld   a,SP_ESC_END
          ld   (_slipesch),a
          ld   hl,_slipstat
          set  2,(hl)
	  scf
          ret
.outchar1 cp   SP_ESC
          jp   nz,outchar2
          ld   a,SP_ESC
          call sendchar
          ret  c
.outchara
          ld   a,SP_ESC_ESC
          ld   (_slipesch),a
          ld   hl,_slipstat
          set  2,(hl)
	  scf
          ret
.outchar2 call sendchar
          ret  c
.outchar3 inc  hl
          ld   (_slippos),hl
          ld   hl,(_sliplen)
          dec  hl
          ld   (_sliplen),hl
          ld   a,h
          or   l
	  scf
          ret  nz

          ld   hl,_slipstat
          set  3,(hl)
	  scf
          ret



;Finished sending the packet...
.outchard
;          ld   hl,pktout
;          call incdword


;Okay...the packet is finished, find the next one..
          ld   hl,(_slippack)
          ld   e,(hl)
          inc  hl
          ld   d,(hl)    ;addy of next packet
#ifndef ARCHAIC_SLIP_AGAIN
; Skip up to actual address of packet for FreePacket();
	inc	hl
	inc	hl
	inc	hl
;de = address of new packet, hl=addy of old packet (to be freed)
	ld	a,d
	or	e
	ld	(_slipstat),a
	jp	nz,outchar8
; This was the last packet, clean sliplast
	ld	(_sliplast),de
	and	a
	ret

;Now, set up to get a new packet..
.outchar8
	push	hl	;keep old packet
	ex	de,hl	;get new packet into hl
          ld   (_slippack),hl
          inc  hl
          inc  hl
          ld   e,(hl)
          inc  hl
          ld   d,(hl)
          ld   (_sliplen),de
          inc  hl
          ld   (_slippos),hl
	  ld	a,1
	  ld (_slipstat),a
	pop	hl	;get old packet
	and	a	;nc=fre packet
        ret
#else
          push de
          dec  hl
          push hl
          call free      ;free the just sent packet
          pop  bc
          pop  hl
          ld   a,h
          or   l
          ld   (_slipstat),a
          jp   nz,outchar8
;Okay, this was the last packet, so clear sli
          ld   (_sliplast),hl
          ret
;Now, set up to get a new packet..
.outchar8

          ld   (_slippack),hl
          inc  hl
          inc  hl
          ld   e,(hl)
          inc  hl
          ld   d,(hl)
          ld   (_sliplen),de
          inc  hl
          ld   (_slippos),hl
          ld   hl,_slipstat
          ld   (hl),1
	  ret
#endif

;------------------------------------------------------------------------

;Receive a packet..this is called continuously..we hope!!
;Always uses the same buffer for input..
;Entry:   none
;Exit:
;         hl=length of packet   !!CHANGED 25/4/99
;         nc=full packet
;          c=packet not yet finished



.inchar   call getchar
          ret  c                ; no good
          ld   hl,_inslipflag
          bit  0,(hl)
          jp   nz,inchar21
          cp   SP_END
          jp   nz,inchar1
          ld   de,(_insliplen)
          ld   a,d
          or   e
          scf
          ret  z    ;zero length packet..
          ld   hl,MINHDR
          and  a
          sbc  hl,de
          jp   c,inchar0

; Shame, the packet we just received was < MINHDR bytes, have to try
; all over again!
.restpack
          ld    hl,0
          ld    (_insliplen),hl
          ld   hl,SLIPPKT
          ld   (_inslippos),hl
          scf
          ret

; Success! We have received a complete packet, so reset the input length
; to zero, and exit with hl=length received
; We also reize the block here down from 600 bytes to whatever is
; actually needed
; As we come through we have:
;                               de=recvd packet length
.inchar0
          ld    hl,0
          ld    (_insliplen),hl
          ld    hl,SLIPPKT
          ld    (_inslippos),hl
#ifdef RESIZEBLK
          push  de
          call  _resizeblk
          pop   hl
#else
          ex    de,hl
#endif
          and  a
          ret       ;nc, with hl=packet length

;We didnt receive an END..so..
.inchar1  cp   SP_ESC
          jp   z,inchar2
.inchar3  ld   hl,(_inslippos)
          ld   (hl),a
          inc  hl
          ld   (_inslippos),hl
          ld   hl,(_insliplen)
          inc  hl
          ld   (_insliplen),hl
          ld   bc,MAXPKTSZ
          and  a
          sbc  hl,bc
          jp   nc,restpack     ;weve received too many bytes..outta here
          scf                  ;not full packet yet!!
          ret
;We received an escape character...
.inchar2  ld   hl,_inslipflag
          set  0,(hl)
          scf
          ret

.inchar21 res  0,(hl)
          cp   SP_ESC_ESC
          jp   nz,inchar4
          ld   a,SP_ESC
          jp   inchar3
.inchar4  cp   SP_ESC_END
          jp   nz,inchar3     ;undefined..let it slide
          ld   a,SP_END
          jp   inchar3

;------------------------------------------------------------------------

        INCLUDE         "#serintfc.def"


;       Low Level initiialisation - set up the serial port (flush out)
;       And set some counters.

._SlipInit 
#ifdef __Z88__
          ld   l,si_sft
          call_oz os_si
#endif
          xor  a
          ld   (_slipstat),a
          ld   (_inslipflag),a
          ld   hl,0
          ld   (_sliplen),hl
          ld   (_slippos),hl
          ld   (_slippack),hl
          ld   (_sliplast),hl
          ld    (_insliplen),hl
#ifdef PLUGIN
          ld   hl,SLIPPKT
#else
	  ld	hl,0
	  ld	(_pktin),hl
	call	_SlipReturnPkt
#endif
          ld   (_inslippos),hl
#ifdef __CPM__
ld    a,1
ld  (_online),a
#endif
._SlipReInit
          ld   hl,4          ;over head for slip packets
; SLIP always resets itself before handing packet to stack
          ret



;
;       Confused as to what is corrupted, and what isnt!
;

#ifdef __Z88__
.sendchar
        push    hl
        ld      l,si_pbt        ;reason code
        ld      bc,0            ;temeout
        call_oz(os_si)
        pop     hl
        ret

.getchar
        ld      l,si_gbt
        ld      bc,0
        call_oz(os_si)
        ret
#endif

#ifdef __CPM__
.sendchar
	push   hl
	ld     e,a
	ld     d,0
	ld     c,4
	call   5
	and    a
	pop	hl
	ret 

.getchar
	ld     c,3
	call   5
	ld     a,l
	and    a
	inc    l
        ret    nz
	scf
	ret
#endif
	
	

/*
 *	If we're a plugin then we have our buffer with us..
 */

#ifdef PLUGIN
._pktin
	defs	1500,0
#endif


#endasm


