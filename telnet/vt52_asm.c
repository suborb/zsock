/*
 *      vt52 Display Routine
 *
 *      djm 17/12/99
 *
 *	djm 22/12/99 Rewritten parts in asm for sppppeeeed!
 *
 *	djm 6/1/2000 Added dumb support for ESC F/ESC G
 *		     Sort of tab emulation add
 *
 *	djm 7/1/2000 Rewrote most of it -> asm
 *
 *	djm 10/1/2000 Sped up a lot(!) by using OZ cursor
 *		      Movement rather than my own.
 */

#include <stdio.h>
#include <sys/types.h>

#pragma asm
	INCLUDE		"#stdio.def"
#pragma endasm

#define ESCAPE 27

int	cx,cy;
u8_t esc_need;

/* fns written in asm need to be declared extern so picked up by
 * sccz80 as internal
 */

extern MovePosn();


void ResetTerm()
{
#pragma asm
	ld	hl,0
	ld	(_cx),hl
	ld	(_cy),hl
	ld	a,l
	ld	(_esc_need),a
	jp	_ClrToScr
#pragma endasm
}



int PrintVT52(u8_t c)
{
#pragma asm
	ld	a,(_esc_need)
	and	a
	jp	z,not_escaped
	dec	a	;1
	jp	z,do_escape
	dec	a	;2
	jp	nz,not_gotx
;got x posn
	ld	(_esc_need),a
	ld	a,l
	sub	32
	ld	(_cx),a
	jp	_MovePosn	;Move to new posn
.not_gotx
;Got y position ; a=2
	ld	a,2
	ld	(_esc_need),a
	ld	a,l
	sub	32
	ld	(_cy),a
	ld	l,0	;hl=0 return
	ret
;Handle an escape code..
.do_escape
	ld	(_esc_need),a	;a=0
#pragma endasm
                switch(c) {
/* VT52 compatability modes */
                        case 'A': /* Up */
#pragma asm
	ld	c,0
	ld	e,-1
	ld	a,$0B
._MoveIt
	call_oz(os_out)
	jp	_MoveCurs
#pragma endasm
                        case 'B': /* Down */
#pragma asm
	ld	c,0
	ld	e,1
	ld	a,$0A
	jr	_MoveIt
#pragma endasm
                        case 'C': /* Right */
#pragma asm
.MoveRight
	ld	c,1
	ld	e,0
	ld	a,9
	jr	_MoveIt
#pragma endasm
                        case 'D': /* Left */
#pragma asm
.moveleft
	ld	c,-1
	ld	e,0
	ld	a,8
	jr	_MoveIt
#pragma endasm
                        case 'H': /* Home */
#pragma asm
	ld	hl,0
	ld	(_cx),hl
	ld	(_cy),hl
	jp	_MovePosn
#pragma endasm
                                break;
			case 'I': /* Rev line feed */
#pragma asm
	call	_ScrollDown
	jp	_MovePosn
#pragma endasm
			case 'Y':
#pragma asm
	ld	a,3
	ld	(_esc_need),a
	ld	hl,0
	ret
#pragma endasm
                        case 'J': /* Erase to end of screen */
#pragma asm
	jp	_ClrToScr
#pragma endasm
                        case 'K': /* Erase to end of line */
#pragma asm
	jp	_ClrToLine
#pragma endasm
			case 'Z': /* Ident sequence */
#pragma asm
	ld	hl,1
	ret
#pragma endasm
			default:

#pragma asm
	ld	hl,0
	ret
#pragma endasm
                }
#pragma asm
.not_escaped
	ld	a,l
	cp	ESCAPE
	jr	nz,not_escaped_1
	ld	a,1
	ld	(_esc_need),a
	ld	l,0	;h=0
	ret
.not_escaped_1
	ld	hl,0
	cp	10
	ret	z
	cp	9
	jp	z,_dotab
	cp	13
	jr	nz,notcr
	call_oz(gn_nln)
	ld	hl,0
	ld	(_cx),hl
	ld	c,l
	ld	e,1
	jp	_MoveCurs
.notcr
	call_oz(os_out)	;print char
	cp	7	;BEL
	ret	z
	cp	8	;Backspace
	ld	c,-1
	jr	z,Backsp
	ld	c,1
.Backsp
	ld	e,0
	jp	_Movecurs
#pragma endasm
}
                
#pragma asm
;de=py bc=px
._MoveCurs
	ld	d,0
	ld	b,d
	ld	hl,(_cy)
	add	hl,de
	ex	de,hl	;keep cy safe
	ld	hl,(_cx)
	add	hl,bc
	bit	7,l
	jr	z,notsmx
	ld	hl,0
.notsmx
	ld	a,l
	cp	80
	jr	c,storex
	ld	hl,0
	inc	de	;increment row
.storex
	ld	(_cx),hl
;Now deal with y gunk..
	ex	de,hl
	bit	7,l
	jr	z,notsmy
	ld	hl,0
.notsmy
	ld	a,l
	cp	8
	jr	c,storey
	ld	hl,7
.storey
	ld	(_cy),hl
	ld	hl,0
	ret
#pragma endasm

#pragma asm
._ScrollDown
	ld	hl,scrolldowntxt
	call_oz(gn_sop)
	ret
.scrolldowntxt
	defb	1,254,0
#pragma endasm

#ifdef IMPOSSIBLE
/* Scrollup not needed - z88 does it automagically */
ScrollUp()
{
	putc(1);
	putc(255);
}
#endif

#pragma asm
._MovePosn
	ld	hl,moveposn1
	call_oz(gn_sop)
	ld	a,(_cx)
	add	a,32
	call_oz(os_out)
	ld	a,(_cy)
	add	a,32
	call_oz(os_out)
	ld	hl,0
	ret
.moveposn1
	defb	1,'3','@',0
#pragma endasm

#pragma asm
._ClrToLine
	ld	hl,clrtolinet
	call_oz(gn_sop)
	jp   _MovePosn
.clrtolinet
	defb	1,'2','C',253,0
#pragma endasm


#pragma asm
._ClrToScr
	ld	hl,clrtoscrtxt
	call_oz(gn_sop)
	jp   _MovePosn
.clrtoscrtxt
	defb	1,'3','@',32,32,1,'2','C',254,0
#pragma endasm

#pragma asm
._DoTab
	ld	a,(_cx)
	and	7
	ld	c,a
	ld	a,8
	sub	c	;number of chars to tab
	ld	c,a
	ld	b,a
.dotab1
	ld	a,32
	call_oz(os_out)
	djnz	dotab1
;c should remain untouched throughout this... b=0, so okay
	ld	e,b	;py
	jp	_MoveCurs
#pragma endasm
