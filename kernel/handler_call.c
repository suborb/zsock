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
 * $Id: handler_call.c,v 1.4 2002-06-01 21:43:18 dom Exp $
 *
 */



#include "zsock.h"

#ifdef Z80

/*
 *	This function obviously has more arguments but 
 *	we only care abt the last one...This is abt
 *	the only time that it's cool we push left->right
 *	instead of right->left
 */

int Handler_Call(s)
	TCPSOCKET *s;
{
#pragma asm
	INCLUDE "#stdio.def"
	pop	bc	;ret address
	pop	de	;socket
	push	de	;put them back
	push	bc
#ifdef __Z88__
	ld	hl,7	;offset to handler_type
	add	hl,de
	ld	a,(hl)
	inc	hl	;go to handler address
	and	a
	jr	nz,do_ext_call
#else
	ld	hl,8	;offset to handler_call
	add	hl,de
#endif
; Internal Call - socket already in de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	jp	(hl)
; External package call additional offset of 2.
.do_ext_call
#ifdef __Z88__
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	iy
.ext_call_in
	rst	16	;call through register
	jr	nc,ext_callok
; We failed..we are only called with return value important
; for acking data, so set to be 0 so we dont screw up
; We hijack part of this code for PktRcvIP, it also requires
; a zero return value to carry on so..
	ld	hl,0	;safe value
.ext_callok
#endif
#pragma endasm
}

/*
 *	Call a package directly (used for catchall)
 *	So similar to above that we use their code!
 */

PackageCall(rout)
	int	rout;
{
#pragma asm
	pop	hl	;return address
	pop	iy	;Packet ID to call
	push	iy
	push	hl
#ifdef __Z88__
	jr	ext_call_in
#else
	jp      (iy)
#endif
#pragma endasm
}

#endif
	

