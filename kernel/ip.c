/*
 *      IP Routines for Small C+ Demo TCP stack
 *
 *      djm 18/2/99
 */

#include "zsock.h"
#include <net/hton.h>
#include <stdio.h>

#define MASKVER 0xF0
#define IP_DF	0x4000

/* 
 * z80asm appears to be having problems with the number $007f0100
 * so, defining loopback ip address the hard way!
 */

extern LWORD loopbackip;

#pragma asm
._loopbackip
        defb    127,0,0,1
#pragma endasm



extern TCP_Handler();
extern ICMP_Handler();
extern UDP_Handler();

extern struct pktdrive *device;

struct pkt {
       struct ip_header ip;
       char   data;            /* actually [] but not important!*/
};

extern void PktRcvIP(struct pkt *,WORD);
extern void SendICMP(void *,BYTE,BYTE,void *);

WORD __FASTCALL__ IPHdrSum(void *);



/*
 * PktRcvIP, handles a decoded packet
 * This Routine is called when a full packet has been read in
 * this, decodes, checks whether it is applicable 
 */

void PktRcvIP(buf,len)
	struct pkt *buf;
        WORD len;                       /* length of incoming packet */
{

/* 
 * Initial check for verson, if not version 4 drop like a stone!
 */

        if ( ((buf->ip.version)&240) != 0x40 ) return;

        if (buf->ip.dest != sysdata.myip && buf->ip.dest !=loopbackip ) {
		 return;
	}
#ifdef NETSTAT
	++netstats.ip_recvd;
	netstats.ip_recvlen+=len;
#endif
/*
 * We don't like fragmentation either..
 */

	if (buf->ip.frag && buf->ip.frag != htons(IP_DF) ) {
		return;
	}

/* 
 * Now do a checksum on the packet
 * If this is non-zero then we've got a duff packet
 */

        if ( IPHdrSum(buf) ) {
#ifdef NETSTAT
		++netstats.ip_badck;
#endif
		return;
	}

/*
 * Now check length of packet
 */
        if ( len != htons(buf->ip.length) ) {
#ifdef NETSTAT
		++netstats.ip_badlen;
#endif
		return;
	 }

/* 
 * Decrement and check the TTL of the incoming packet
 *
 * We don't act as a gw so packet is always for us...
 */

	if ( buf->ip.ttl == 1 ) {
		SendICMP(buf,DEST_UNREACH,PORT_UNREACH,0);
		return;
	}

	if ( sysdata.catchall )
		if (PackageCall(buf,len,sysdata.catchall) ) return;

        switch (buf->ip.protocol) {
        
                case prot_TCP:
#ifdef NETSTAT
			++netstats.tcp_recvd;
#endif
                        TCP_Handler(buf,len);
                        break;

                case prot_ICMP:
#ifdef NETSTAT
			++netstats.icmp_recvd;
#endif
                        ICMP_Handler(buf,len);
                        break;

                case prot_UDP:
#ifdef NETSTAT
			++netstats.udp_recvd;
#endif
                        UDP_Handler(buf,len);
			break;
                case prot_IGMP:
                default:
			SendICMP(buf,DEST_UNREACH,PROT_UNREACH,0);
                        break;
        }

}

/*
 *
 *      Send an IP Packet - no check for local addresses or anything
 *      like that yet, just shovel it off to the SLIP driver
 *
 *	Check for myip and localhost added 19/12/99
 */

SendPacket(buf,len)
        struct ip_header *buf;
        WORD len;
{
	u8_t	binding;
#ifdef NETSTAT
	++netstats.ip_sent;
#endif
/*
 * Send a packet if it's to us, stash it into the local queue
 * if not then chuck it out through the defined packet driver
 */
	if (buf->dest == sysdata.myip || buf->dest == loopbackip ) 
		StashLocal((struct pkt *)buf,len);
	else {
	/* Shovel to device, page in and out device */
		binding=PageDevIn();
        	device->queuefn(buf,len);
		PageDevOut(binding);
	}
}





/* 
 * Calculate the checksum of IP header
 */

static WORD IPHdrSum(buf)
        void *buf;      /* for convenience! */
{
#pragma asm
        ld      a,(hl)
        and     @00001111
        rlca
        rlca
        ld      c,a
        ld      b,0
        call    FastCSum        ;exits with checksum in hl
        ld      l,c
        ld      h,b
#pragma endasm
}

/*
 *
 *      Calculate and set IP Header Checksum
 */

int IPHeaderCheck(buf)
        BYTE *buf;
{
#pragma asm
               ld   bc,10
               add  hl,bc
               xor  a
               ld   (hl),a
               inc  hl
               ld   (hl),a         ; Set checksum to zero
               push hl
               scf
               sbc  hl,bc
               ld   a,(hl)
               and  $0f
               rlca
               rlca
               ld   c,a
               ld   b,0
               call FastCsum
               pop  hl
               ld   (hl),c
               dec  hl
               ld   (hl),b
#pragma endasm
}


/*
 *      Generic Checksum routine - used by most handlers
 */

int CheckSum(buf,len)
        BYTE    *buf;
        WORD    len;
{
#pragma asm
        pop     de      ;return address
        pop     bc      ;length
        pop     hl      ;address of buffer
        push    hl
        push    bc
        push    de
        call    FastCSum
        ld      l,c
        ld      h,b
#pragma endasm
}

/*
 *      Some Checksum routines - sorry, more assembler!!
 */

#pragma asm

                XDEF    _PsHeaderSum
                XDEF    _TCPCsum

;Calculate 1s complement sum of pseudo IP header
;Entry:   hl=buffer, e=protocol, bc=length
;Exit:    bc=sum

._PsHeaderSum   
                push hl
               ld   d,0       ; add (0,protocol)
               ex   de,hl
               add  hl,bc          ; add data length
               push af        ; save carry
               ld   bc,12
               ex   de,hl
               add  hl,bc
               pop  af
               ld   b,4       ; add 4 words (source, destination)
.PsHead_1
               ld   a,(hl)
               inc  hl
               adc  a,d
               ld   d,a
               ld   a,(hl)
               inc  hl
               adc  a,e
               ld   e,a
               djnz PsHead_1
               ld   a,d       ; add last carry
               adc  a,0
               ld   d,a
               ld   a,e
               adc  a,0
               ld   c,a
               ld   a,d
               adc  a,0
               ld   b,a
               pop  hl
               ret

;Calculate IP checksum
;Entry:   hl=buffer,bc=length
;Exit:    bc=checksum, hl=buffer

;Tcp fastcsum..enters with de holding total of pseudo header...

._tcpcsum 
               push hl
               push de
               ld   a,c
               ld   c,b       ;  swap b,c
               srl  c
               rra            ; adjust counter for words
               ld   b,a       ; (cb=#words)
               push af        ; save cary for a single byte
               or   c         ; check for zero also clear carry
               jr   z,FastCsum_2   ; Only one or less bytes
               inc  c
               ld   a,b
               or   a
               jr   z,FastCsum_1b
               jr   fastcsum_1
#pragma endasm



/* 
 * Assembler Checksum routine
 *
 * Enter: hl = start, bc = length
 * Exit:  bc = cksum, hl = start, de=de
 */

#pragma asm
.FastCSum      push hl
               push de
               ld   a,c
               ld   c,b       ;  swap b,c
               srl  c
               rra            ; adjust counter for words
               ld   b,a       ; (cb=#words)
               push af        ; save cary for a single byte
               ld   de,0      ; de=sum
               or   c         ; check for zero also clear carry
               jr   z,FastCsum_2   ; Only one or less bytes
               inc  c
               ld   a,b
               or   a
               jr   z,FastCsum_1b
.FastCsum_1              ; use counter c for outer loop
               ld   a,(hl)
               inc  hl
               adc  a,d
               ld   d,a
               ld   a,(hl)
               inc  hl
               adc  a,e
               ld   e,a
               djnz FastCsum_1     ; inner loop
.FastCsum_1b
               dec  c
               jr   nz,FastCsum_1  ; outer loop
               ld   a,d
               adc  a,0
               ld   d,a
               ld   a,e
               adc  a,0
               ld   e,a
               ld   a,d
               adc  a,0
               ld   d,a
.FastCsum_2
               pop  af        ; check for single byte
               jr   nc,FastCsum_3
               ld   a,(hl)
               add  a,d
               ld   d,a
               ld   a,e
               adc  a,0
               ld   e,a
               ld   a,d
               adc  a,0
               ld   d,a
               ld   a,e
               adc  a,0
               ld   e,a
.FastCsum_3
               ld   a,d
               cpl
               ld   b,a
               ld   a,e
               cpl
               ld   c,a
               pop  de
               pop  hl
               ret
; size=256*c+b
; This costs about 27660 states/kB: 7.7 ms/kB @3.58MHz
#pragma endasm







