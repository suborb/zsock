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
 * $Id: lowlevel2.c,v 1.6 2002-10-08 20:43:55 dom Exp $
 *
 * Z88 Packages
 */





#include "zsock.h"


#ifdef Z88
extern int StackInit();




/*
 *	Routines for paging the device in and out
 */

u8_t PageDevIn()
{
#asm
	ld	hl,($4D0)
	ld	h,0
	ld	a,(_sysdata+page0)
	ld	($4D0),a
	out	($D0),a
#endasm
}

void PageDevOut(u8_t bind)
{
#asm
	pop	de
	pop	hl
	push	hl
	push	de
	ld	a,l
	ld	($4D0),a
	out	($D0),a
#endasm
}


void _DeviceOffline(int flag)
{
	int val,temp;
#asm
	call pagein
	pop	hl	;store bindings
	push	bc
#endasm
	val = PageDevIn();
	device->offlinefn(flag);
	PageDevOut(val);
#asm
	pop	bc
	push	bc
	call pageout
#endasm
	return_nc;
}

void _DeviceOnline()
{
	int val,temp;
#asm
	call pagein
	pop	hl
	push	bc
#endasm
	val=PageDevIn();
	device->onlinefn();
	PageDevOut(val);
#asm
	pop	bc
	push	bc
	call pageout
#endasm
	return_nc;
}


/*
 *	The Are You There function, grabs mem
 *	(if needed) and adds the interrupt
 */

void pack_ayt(void)
{
#asm
	INCLUDE	"#packages.def"
	INCLUDE "#error.def"
	INCLUDE "#stdio.def"
	push	af	;preserve primary registers
	push	bc
	push	de
	push	hl
	push	ix
	push	iy
	ld	a,(ZSOCK0STORE)
	and	a
	jp	nz,gotmem
;
; Allocate memory here
;
	call	mem_allocate
	jr	c,exit_fail	;no mem :(
	push	bc		;preserve old bindings..
.reinit
	ld	hl,1		;Initialise all
	push	hl
	call	_StackInit	;(old main)
	pop	bc
	ld	a,l
	and	a
	jr	nz,inst_failure	; Initialisation failed usu pktdriver
;
; If we are called then package handling does exist so
; no need to check for it..
;
; Register global interrupt handler
.reg_int
	ld	hl,PACK_VERSION
	ld	(_sysdata+version),hl
#ifndef BUSY_VERSION
	ld	a,int_pkg
	ld	c,1		;TICK (define pls garry!)
	ld	b,10		;Every 1/20 second
				;Gives ping throughput of
				;~500ms with 10 calls/sec
	ld	hl,ZSOCK_INT	;interrupt handler
	call_pkg(pkg_intr)
	jr	c,inst_failure
#endif
	pop	bc		;old bindings
	call	pageout		;page em out boy!
.exit_success
	pop	iy
	pop	ix
	pop	hl
	pop	de
	pop	bc
	pop	af
	and	a
	ret
;
; Hmm, failed to register an interrupt, we must have no
; room left...so deallocated our nemory and return failure
;
.inst_failure
	pop	bc
	call	pageout
	call	mem_dealloc	;stores 0 in magic place
.exit_fail
	pop	iy
	pop	ix
	pop	hl
	pop	de
	pop	bc
	pop	af
	scf
	ret
; Here, the memory is in place, so check version
.gotmem
	call	pagein		;page in exits with bc=bindings
	push	bc
	ld	hl,(_sysdata+version)	;mem base
	ld	de,PACK_VERSION
	and	a
	sbc	hl,de
	jr	z,reg_int	;good version
	jr	nc,inst_failure	;data is higher version
	jr	reinit		;lower version so set up again
#endasm
}

/*
 *	Get Rid of The Package
 */

void pack_bye(void)
{
#asm
	call	checkarea
	ret	c
; Check to see if were being used
	push	bc
	push	de
	push	hl
	call	pagein
	push	bc	;old bindings
	call	_GetResources
	pop	bc	;get em back
	call	pageout
	ld	a,l
	or	h
	jr	z,pack_bye_ok
;We are being used by external stuff
	ld	a,rc_use
	pop	hl
	pop	de
	pop	bc
	scf
	ret
.pack_bye_ok
#ifndef BUSY_VERSION
	ld	hl,ZSOCK_INT	;interrupt
	ld	a,int_pkg
	call_pkg(pkg_intd)
#endif
	call	mem_dealloc
	pop	hl
	pop	de
	pop	bc
	and	a
#endasm
}

#asm
.checkarea
	ld	a,(ZSOCK0STORE)
	and	a	;resets carry
	ret	nz
	ld	a,rc_pnf
	scf
	ret
#endasm
	
	

/*
 *	Return resources
 */

void pack_dat(void)
{
#asm
	call	checkarea
	ret	c
	push	hl
	call	pagein
	push	bc
	call	_GetResources
	pop	bc
	call	pageout
	ld	a,l	;apps using us
	pop	hl
	ld	de,32768
	ld	c,0	;mem used
	ld	b,0	;handles open
	and	a	;reset carry
#endasm
}

/*
 *	Return the number of open sockets
 *	(as done by apps, indicated by s->handlertype being 1)
 *
 *	25/02/2001 Now we count listening sockets
 *
 */

int GetResources(void)
{
	int	num=0;
	TCPSOCKET	*s;

	for (s=sysdata.tcpfirst; s ; s=s->next ) {
		if (s->handlertype || s->state != tcp_stateLISTEN ) num++;
	}
	for (s=sysdata.udpfirst; s ; s=s->next ) {
		if (s->handlertype ) num++;
	}
	return(num);
}

/* Busy wait loop */

void _GoTCP(void)
{
#asm
#ifdef BUSY_VERSION
	call	checkarea
	ret	c
	call	pagein	;page in segs 1 & 2
	push	bc	;keep old bindings
#if 0
	ld	hl,$4D0
	ld	a,(hl)
	push	af	;seg 0 bindings
	ld	a,(_sysdata+page0)
	ld	(hl),a
	out	($D0),a
#endif
	call	_Interrupt	;Main C interrupt routine
#if 0
	pop	af
	ld	($4D0),a
	out	($D0),a
#endif
	pop	bc
	call	pageout
#else
; On interrupt no busy code
#endif
#endasm
}



#asm

	XDEF	Interrupt
	XDEF	Syscall
	XREF	_Interrupt

; Interrupt Routine


.Interrupt
	push	bc	;preserve main registers...
	push	de
	push	hl
	push	ix
	push	iy
	call	pagein	;page in segs 1 & 2
	push	bc	;keep old bindings
	ld	hl,$4D0
	ld	a,(hl)
	push	af	;seg 0 bindings
	ld	a,(_sysdata+page0)
	ld	(hl),a
	out	($D0),a
	call	_Interrupt	;Main C interrupt routine
	pop	af
	ld	($4D0),a
	out	($D0),a
	pop	bc
	call	pageout
	pop	iy
	pop	ix
	pop	hl
	pop	de
	pop	bc
	ret

;
;	Allocate the memory
;	Set up the bindings in sysdata
;
; Exit:	bc = old bindins
;	c/nc = failure/success

	XREF	in_dor_seg2

.mem_allocate
; Allocate mem for 16384-32768
	ld	a,bnk_any	;any type will do
	call_pkg(pkg_bal)
	jr	c,mem_failure	;failure
	ld	bc,($4D1)	;old bindings
	push	bc		;keep on
	ld	($4D1),a	;page it in..
	out	($D1),a
; Clear seg 1..
	ld	hl,16384
	ld	de,16385
	ld	bc,16383
	ld	(hl),l
	ldir
	ld	(_sysdata+page1),a	;keep it
	ld	hl,PACK_VERSION
	ld	(_sysdata+version),hl
	ld	a,($4D3)	;binding of 3
	and	@11000000	;isolate slot
	ld	l,a
	ld	a,(in_dor_seg2)		;Get binding for seg2
	or	l		;or slot with offset ->bank
	ld	(_sysdata+page2),a
	ld	($4D2),a		;page it in (for stack_init)
	out	($D2),a		
; Now try for low one...
	ld	a,bnk_even
	call_pkg(pkg_bal)
	jr	nc,gotallmem
; failure...free up 16384-32767 bank
	ld	a,(_sysdata+page1)
	call_pkg(pkg_bfr)
	pop	bc
	call	pageout			;get old ones in again
.mem_failure
#ifndef SNEAKY
	ret
#else
	ld	hl,memfailtxt
	call_oz(gn_sop)
	scf				;indicate failure
	ret
.memfailtxt
	defm	"ZSock: Could not allocate requested memory"&13&10&0
#endif

.gotallmem
	ld	(_sysdata+page0),a
;	ld	($4D0),a
;	out	($D0),a
	ld	a,(_sysdata+page1)
	ld	(ZSOCK0STORE),a		;set up our byte
	pop	bc			;return old bindings..
	and	a
	ret

.mem_dealloc
	call	pagein		;page ZSock in
	push	bc
	ld	a,(_sysdata+page0)
	call_pkg(pkg_bfr)
	ld	a,(_sysdata+page1)
	call_pkg(pkg_bfr)
	xor	a
	ld	(ZSOCK0STORE),a
	pop	bc
	call	pageout
	ret



;
; The guts of the API
; This receives a system call in a (parameters on stack(!))
; Must page in memory
; Must page out mem
; Must keep af safe
		
.syscall
	cp	PKG_MAX+1
	jr	c,sys_call_good
; Bad number
	ld	hl,-1
	ld	a,rc_fail
	scf
	ret
.sys_call_good
	push	af
	call	pagein	;page in 2 banks
	pop	af
	push	bc
	add	a,a
	ld	l,a
	ld	h,0
	ld	de,sys_table
	add	hl,de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	call	l_dcal	;vars are at stack + 10
	pop	bc	;get pages back
	ld	a,l	;get any error code into (so lib fns work)
	push	af	;preserve af
	call	pageout	;go back to where we were
	pop	af
	ret

; Package call.
; Pages in/out Zsock data page
; Entry:  l = bank to page in
; Exit:   l = Bank which was just switched out

; Same:  ..BC..../IXIY
; Diff:  AF..DEHL/....

._user_Pagein
	ld	a,(ZSOCK0STORE)
	ld	l,a
._user_Pageout
	ld	de,$4D1
	ld	a,(de)
	ld	h,a
	ld	a,l
	ld	(de),a
	out	($D1),a
	ld	l,h
	and	a	;reset carry i.e. OK
	ret


; Paging routines for the syscall routines 


.pagein
	ld	bc,($4D1)
	ld	a,(ZSOCK0STORE)
	ld	($4D1),a
	out	($D1),a
	ld	a,(_sysdata+page2)
	ld	($4D2),a
	out	($D2),a
	ret

;Enters with:
;b=binding for seg 2
;c=binding for seg 1
.pageout
	ld	a,c
	ld	($4D1),a
	out	($D1),a
	ld	a,b
	ld	($4D2),a
	out	($D2),a
	ret

;
; Table of entry points for system calls
;

.sys_table
	defw	_sock_write	;0
	defw	_sock_putc
	defw	_sock_puts
	defw	_sock_flush
	defw	_sock_read
	defw	_sock_close	;5
	defw	_sock_abort
	defw	_sock_shutdown
	defw	_sock_dataready
	defw	_sock_opened
	defw	_sock_closed	;10
	defw	_sock_listen
	defw	_sock_open
	defw	_sock_settimeout
	defw	_sock_chktimeout
	defw	_user_settimeout	;15
	defw	_user_setctimeout	
	defw	_user_chktimeout
	defw	_resolve		;18
	defw	_reverse_addr_lookup
	defw	_getservbyname		;20
	defw	_getservbyport
	defw	_getservprotobyport
	defw	_getservprotobyname
	defw	_getprotobyname
	defw	_getprotobynumber	;25
	defw	_getnetbyname
	defw	_getnetbynumber
	defw	_getdomain
	defw	_gethostaddr
	defw	_sethostaddr		;30
	defw	_setnameservers
	defw	_getnetstats
	defw	_inet_addr
	defw	_inet_ntoa
	defw	_tcp_malloc		;35
	defw	_tcp_calloc
	defw	_tcp_free
	defw	_tcp_regcatchall	;38
	defw	_sock_setptr
	defw	_sock_getptr		;40
	defw	_sock_sethandler
	defw	_sock_setrsize
	defw	_sock_setmode		;43
	defw	_sock_waitopen	
	defw	_kill_daemon		;45
	defw	_sock_waitclose
	defw	_sock_settos
	defw	_sock_setttl		;48
	defw	_sock_pair_listen	;49
	defw	_sock_setssize
	defw	_sock_recv		;51
	defw	_sock_getinfo		;52

; Some External defs

	XDEF	_user_pagein
	XDEF	_user_pageout

	XREF	_sock_write
	XREF	_sock_putc
	XREF	_sock_puts
	XREF	_sock_flush
	XREF	_sock_read
	XREF	_sock_close
	XREF	_sock_abort
	XREF	_sock_shutdown
	XREF	_sock_dataready
	XREF	_sock_opened
	XREF	_sock_closed
	XREF	_sock_listen
	XREF	_sock_open
	XREF	_sock_settimeout
	XREF	_sock_chktimeout
	XREF	_user_settimeout
	XREF	_user_setctimeout
	XREF	_user_chktimeout
	XREF	_resolve
	XREF	_reverse_addr_lookup
	XREF	_getservbyname
	XREF	_getservbyport
	XREF	_getservprotobyport
	XREF	_getservprotobyname
	XREF	_getprotobyname
	XREF	_getprotobynumber
	XREF	_getnetbyname
	XREF	_getnetbynumber
;	XREF	_getdomain
;	XREF	_gethostaddr
	XREF	_sethostaddr
	XREF	_setnameservers
	XREF	_getnetstats
	XREF	_inet_addr
	XREF	_inet_ntoa
	XREF	_tcp_free
	XREF	_tcp_malloc
	XREF	_tcp_calloc
	XREF	_tcp_regcatchall
	XREF	_sock_setptr
	XREF	_sock_getptr		;40
	XREF	_sock_sethandler
	XREF	_sock_setrsize
	XREF	_sock_setmode		;43
	XREF	_sock_waitopen		;44
	XREF	_kill_daemon
	XREF	_sock_waitclose	
	XREF	_sock_settos
	XREF	_sock_setttl
	XREF	_sock_pair_listen
	XREF	_sock_setssize
	XREF    _sock_recv
	XREF	_sock_getinfo

#endasm

#endif /* Z88 */
