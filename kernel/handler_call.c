/*
 *	Handler call...
 *
 *	djm 11/2/2000
 *	Very z88dk Specific...
 *
 *	This simply requires that we tag the socket onto
 *	the end of the call to the handler. (i.e. order
 *	rearranged from earlier versions of ZSock. For 
 *	internal we try to tranparently call, for package
 *	calls we just add a bit of stack offset
 *	(+2)
 */
 
#include "zsock.h"

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
	ld	hl,7	;offset to handler_type
	add	hl,de
	ld	a,(hl)
	inc	hl	;go to handler address
	and	a
	jr	nz,do_ext_call
; Internal Call - socket already in de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	jp	(hl)
; External package call additional offset of 2.
.do_ext_call
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
	ret
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
	jr	ext_call_in
#pragma endasm
}

	

