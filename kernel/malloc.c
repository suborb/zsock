/*
 *
 *      Memory allocation routines...
 *
 *      100% Assembler, ripped from some MSX routine I spotted
 *      on the web!
 *
 *      djm 18/2/99
 */



#include <string.h>
#include "zsock.h"

void *malloc(int);
void *calloc(int, int);
void free(void *);
void heapinit(int);
int getlarge(void);
int getfree(void);
void *allocatepkt(int size);

int HeapBlocks;
int HeapLast;
extern void *heap;

void *calloc(int num, int size)
{
        void *ptr;
        int  tsize;

        tsize=size*num;

        if ( (ptr=malloc(tsize) ) ) {
		memset(ptr,0,tsize);
        }
        return (ptr);
}

#pragma asm


._clrmem
        pop     hl
        pop     bc      ;length
        pop     de      ;buffer
        push    de
        push    bc
        push    hl

        ld      a,b
        or      c
        ret     z       ;so no duff stuff!
        ld      l,e
        ld      h,d
        inc     de
        ld      (hl),0
        dec     bc
        ld      a,b
        or      c
        ret     z
        ldir            ;quick'n'easy
        ret
._free
        pop     bc
        pop     hl
        push    hl
        push    bc
	ld	a,h
	or	l
	call	nz,basic_free
	ret



.free_new
          ; bc=bp
     ld   (_HeapLast),bc
     ld   a,c
     ld   (bc),a
     ld   a,b
     inc  bc
     ld   (bc),a         ; bp->next=bp
     inc  l         ; hl=1
     ld   (_HeapBlocks),hl
     pop  de
     pop  bc
     ret


; free tries to add the block to an existing block, and keeps the
;   list in ascending order.

.basic_free          ; IN: HL=pointer to memory previously allocated
     push bc
     push de
     ld   bc,4
     and  a
     sbc  hl,bc
     ld   c,l
     ld   b,h       ;bc=address of block to free (bp)  (true address)
     ld   hl,(_HeapBlocks)
     ld   a,h
     or   l
     jp   z,free_new     ; No free blocks -> create list

; Find where to link bp to the list
     ld   hl,(_HeapLast)
;Start of loop...
.free_1                  ; check if between p and next
     ld   d,h
     ld   e,l            ;de=p (addy of heaplast)
     ld   a,(hl)
     inc  hl
     ld   h,(hl)         ; hl=p->next
     ld   l,a            ;hl points to next free block
;Now find out if bp is within this range (ie below next and above p)
     push hl
     and  a
     sbc  hl,bc          ;if c, block to free is  next free block
     pop  hl
     jp   c,free_2  ; bp > next

                ;Were below next, but are we above p
     ex   de,hl
     push hl        ;low pointer/hl=addy of first block
     and  a
     sbc  hl,bc     ;if c, block to free>first free block
     pop  hl
     ex   de,hl
     jp   c,free_3  ; p < bp    <= next
                                ; p,next >= bp

;If here, we are not in range/bang on..

.free_2                  ; check if at top or bottom
                                ; bp > next  ||  bp <= p,next
;hl=next block, de=first block
     push hl
     and  a
     sbc  hl,de
     pop  hl
     jp   z,free_21 ;if the same, we only have one block!!!
     jp   nc,free_1 ; next > p - get next bloc

;Get here if we have one block next<p (linked list remember!!)
.free_21
     push hl        ;hl=next
     and  a
     sbc  hl,bc
     pop  hl
     jp   nc,free_3 ; bp <= next   && p >= next
     ex   de,hl
     push hl        ;hl=p
     and  a
     sbc  hl,bc
     pop  hl
     ex   de,hl
     jp   nc,free_1 ; bp <= p     && bp,p >(=?) next

; insert bp into list: p->next=bp, bp->next=next
; bc=bp, de=p, hl=next
;=======================
.free_3                  ; bp <= next < p           bp in front
                    ; p < bp <= next           bp in middle
                    ; next < p < bp            bp at end
     ld   (_HeapLast),de  ; last=p
     ld   a,l
     ld   (bc),a         ; bp->next=next
     inc  bc
     ld   a,h
     ld   (bc),a
     dec  bc             ; (bc=bp)
     ex   de,hl
     ld   (hl),c         ; p->next=bp
     inc  hl
     ld   (hl),b
     inc  hl             ; (hl=p+2)
     inc  bc
     inc  bc             ;bc=bp+2

; Try to merge p, bp: if p+p->size=bp then p->next=next,p->size+=bp->size,bp=p
; bc=bp+2, de=next, hl=p+2
     push de        ; save next
     ld   e,(hl)
     inc  hl
     ld   d,(hl)         ; de=p->size
     dec  hl             ; (hl=p+2)
     push hl        ; save p+2
     add  hl,de               ; (hl=p+p->size+2)
     and  a
     sbc  hl,bc               ; (hl=p+p->size+2-bp-2=p+p->size-bp)
     jp   nz,nomerge_1
     ld   a,(bc)
     ld   l,a
     inc  bc
     ld   a,(bc)
     ld   h,a            ; (hl=bp->size)
     add  hl,de               ; (hl=bp->size+p->size)
     ex   de,hl
     pop  hl        ; get p+2
     ld   (hl),e         
     inc  hl
     ld   (hl),d         ; p->size+=bp->size
     dec  hl
     push hl        ; save p+2
     pop  bc        ; bp=p      (bc=p+2)
     dec  hl             ; (hl=p+1)
     pop  de        ; get next
     ld   (hl),d
     dec  hl
     ld   (hl),e         ; p->next=next
     ex   de,hl
                         ; bc=p+2, hl=next
     jp   merge_2        ; Skip raising: it was merged
.nomerge_1
     ld   hl,(_HeapBlocks)
     inc  hl        ; Raise free block counter
     ld   (_HeapBlocks),hl
     pop  hl
     pop  hl
                         ; bc=bp+2, hl=next

; Try to merge bp, next: if bp+bp->size=next then bp->next=next->next
;                                                 bp->size+=next->size
; bc=bp+2, hl=next
.merge_2
     ld   e,(hl)
     inc  hl
     ld   d,(hl)              ; (de=next->next)
     inc  hl             ; (hl=next+2)
     push de        ; save next->next
     ld   a,(bc)
     ld   e,a
     inc  bc
     ld   a,(bc)
     ld   d,a            ; de=bp->size
     push de        ; save bp->size
     ex   de,hl
     add  hl,bc               ; (hl=bp->size+bp+3)
     scf
     sbc  hl,de               ; (hl=bp->size+bp+3-(next+2)-1)
     jp   nz,nomerge_2
                         ; bc=bp+3, de=next+2
     ex   de,hl
     ld   e,(hl)
     inc  hl
     ld   d,(hl)              ; (de=next->size)
     pop  hl        ; get bp->size
     add  hl,de               ; (hl=bp->size+next->size)
     ld   a,h
     ld   (bc),a
     dec  bc
     ld   a,l
     ld   (bc),a         ; bp->size+=next->size
     dec  bc
     pop  hl
     ld   a,h
     ld   (bc),a
     dec  bc
     ld   a,l
     ld   (bc),a         ; bp->next=next->next
     ld   (_HeapLast),bc
     ld   hl,(_HeapBlocks)
     dec  hl        ; Lower free block counter
     ld   (_HeapBlocks),hl
     pop  de        ;original values entered...
     pop  bc
     ret
.nomerge_2
     pop  hl
     pop  hl
     pop  de
     pop  bc
     ret


;   getfree     OUT: HL=Total free heap,
;                    DE=Biggest block of heap
;                    BC=Number of blocks free
; Goes through the whole list, counting and adding everything
;           uses AF,BC,DE,HL
._getfree
     push ix
     ld   hl,0
     ld   de,0
     ld   bc,(_HeapBlocks)     ; Number of free blocks list in BC
     ld   ix,(_HeapLast)  ; IX walks through linked list
     ld   a,b
     or   c
     jp   z,getfree_3         ; No free blocks
.getfree_1
     push bc
     ld   c,(ix+0)
     ld   b,(ix+1)
     push bc
     pop  ix        ; ix=ix->next
     ld   c,(ix+2)
     ld   b,(ix+3)  ; BC=size of block
     add  hl,bc          ; add block to total
     ex   de,hl
     push hl
     or   a
     sbc  hl,bc
     pop  hl
     jp   nc,getfree_2   ; Not bigger
     ld   h,b
     ld   l,c       ; save size of bigger block
.getfree_2     
     ex   de,hl
     pop  bc
     dec  bc        ; try next block
     ld   a,b
     or   c
     jp   nz,getfree_1   ; not yet through list
     ld   bc,(_HeapBlocks)
.getfree_3
     pop  ix
     ret;

;
;       Find the largest free block
;

._getlarge
        call    _getfree
        ex      de,hl
        ret

; Exit: hl=free blocks

._heapinit
        pop     bc
        pop     de      ;heapsize
        push    de
        push    bc
        push    de      ;put size back on stack

        ld      hl,_heap
        push    hl      ;start
        push    de      ;size
        call    _clrmem
        pop     bc
        pop     bc

        ld   hl,1
        ld   (_HeapBlocks),hl    ; One free block fot starters
        ld   hl,_heap
        ld   (_HeapLast),hl
        ld   (_heap),hl      ; First block points to itself
        pop     hl              ; heap size
        ld   (_heap+2),hl    ; Has size of whole heap
        ret

._malloc
        pop     bc
        pop     hl
        push    hl
        push    bc
        call    basic_malloc
        ret

.OutMem
     pop  iy
     pop  ix
     pop  de
     pop  bc
     ld   hl,0
     ret



; IN: HL=size OUT: HL=pointer to mem
;                                    =0 if out of memory
;           uses AF,(BC,DE),HL,(IX,IY)

.basic_malloc
     push bc
     push de
     push ix
     push iy
     ld   de,4      ; Add headersize
     add  hl,de
     jp   c,OutMem
     ex   de,hl          ; Requested size in DE
     ld   bc,(_HeapBlocks)     ; Number of free blocks list in BC
     ld   ix,(_HeapLast)  ; IX walks through linked list
     ld   a,b
     or   c
     jp   z,OutMem  ; No free blocks
.mal_1
     push ix        ; 
     pop  iy        ; IY=prev
     ld   l,(ix+0)  ;address of next block
     ld   h,(ix+1)
     push hl
     pop  ix        ; ix=ix->next (ix=addy of next block)
     ld   l,(ix+2)
     ld   h,(ix+3)  ; HL=size of block
     or   a
     sbc  hl,de
     jp   nc,mal_2  ; Request fits
     dec  bc        ; try next block
     ld   a,b
     or   c
     jp   nz,mal_1  ; not yet through list
                ;run out of memory!!!
                    ; Here more memory should be allocated
                    ; But it isnt for now
     jp   OutMem

;Here we try to fit memory..
.mal_2
                    ; BC=p, DE=ReqSize, HL=BlockSize-ReqSize
     jp   z,mal_3        ; Exactfit
                    ; HL=size remaining of block
     ld   (ix+2),l
     ld   (ix+3),h  ; Store it. The link is still correct
     ld   (_HeapLast),ix
     ex   de,hl
     add  ix,de          ; ix=IX+NewBlkSize
     ld   (ix+2),l
     ld   (ix+3),h  ; save size of new block
     jp   mal_4     ;get out of here..

;Exact fit...  ;iy holds previous block..
.mal_3    
     ld   a,(ix+0)  ; IY->next=IX->next
     ld   (iy+0),a
     ld   a,(ix+1)
     ld   (iy+1),a
     ld   hl,(_HeapBlocks)
     dec  hl        ; One free block less on the list
     ld   (_HeapBlocks),hl
     ld   (_HeapLast),iy
.mal_4
     push ix
     pop  hl
     ld   bc,4
     add  hl,bc          ; HL=pointer to datablock


     pop  iy
     pop  ix
     pop  de
     pop  bc
     ret

#pragma endasm





/* 
 *      Allocate/free memory for a packet
 *      This covers the slip packet overhead (actuall _pmalloc
 *      does this)
 */

FreePacket(buf)
        char *buf;
{
        free(buf-sysdata.overhead);
}

/*
 *	Fixed 6/12/99 to return null if no mem
 *	see if stack is robust enuff to handle...
 */

void *allocatepkt(size)
        int    size;
{
        void *ptr;

        if ( (ptr=malloc(size+sysdata.overhead)) == NULL) return 0;
        return(ptr+sysdata.overhead);
}


#ifdef RESIZEBLK
#pragma asm
        XDEF    _resizeblk
        

;
;       Routine to resize a block
;
;       Entry: de=new length (of data)
;              hl=start address (really +4)
;
;       The length of new block is really de+8

._resizeblk
        inc     de
        push    hl
        pop     ix
        ld      c,e
        ld      b,d
        ld      hl,8
        add     hl,de
        ex      de,hl   ;de=true length of blk 
; Pick up allocated size
        ld      l,(ix-6)
        ld      h,(ix-5)
; Decrease by 4 to cover overhead of malloc block
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        and     a
        sbc     hl,de
        ret     c       ;not big enough
        ld      a,h
        or      l
        ret     z       ;just the right size
; Okay, so, it fits, so, fake this current block
; Store length of data packet (old pkt)
        ld      (ix-6),e
        ld      (ix-5),d
        add     ix,bc   ;ix now points to start of new block
        ex      de,hl
        inc     de
        inc     de
        inc     de
        inc     de
        ld      (ix+2),e        ;store length of "free" packet
        ld      (ix+3),d
        push    ix
        pop     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        push    hl
        call    free
        pop     bc
        ret

#pragma endasm

#endif /* RESIZEBLK */

