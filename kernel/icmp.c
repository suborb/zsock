/*
 *
 *      Basic ICMP Module to the Small C+ TCP stack
 *
 *      djm 18/2/99
 * 
 *      All this does is reply to a ping message
 *
 *	djm 4/12/99
 *	Can send an ICMP message
 *
 *	djm 8/12/99
 *	DOESN'T KEEP PACKET ANY LONGER!!!
 *
 *	djm 11/1/2000
 *	To reflect recent changes (hah!)
 *	We now have an option to pass an ICMP packet to
 *	a user routine to examine for ping etc
 *	We now have a routine to send a ping packet
 *
 *	Things to implement
 *	- Source Quench Request - almost farcial - are we 
 *	talking to a c64 (possible though)
 *	- Host unreachable (i.e. link down)
 *	- Port unreachable (i,e. UDP/traceroute)
 *	- Prot unreachable (unlikely since we only talk ICMP/UDP/TCP)
 *	All these require passing message up to the user level..not
 *	in this initial package release of ZSock
 */

#include <net/hton.h>
#include "zsock.h"




extern FreePacket();
extern void *AllocatePkt(int);

void ICMP_Handler(BYTE *buf, WORD len);
int __CALLEE__ FixSum(int cksum, int add);
void FillHeader(struct ip_header *,u8_t, u8_t);

/*
 * Register User ICMP Handler..eg for ping
 *
 * This is only used by app, so can get away with
 * removing the check..
 */

RegisterICMPHandler(int (*fn)())
{
//	if (sysdata.usericmp) return 0;
	sysdata.usericmp=fn;
	return(fn);
}

DeRegisterICMPHandler()
{
	sysdata.usericmp=0;
}

		    


/*
 *	Send an ICMP message to host ipaddr
 */

void SendICMP(buf,type,code,unused)
	struct ip_header *buf;
	u8_t	type;
	u8_t	code;
	u32_t	*unused;
{
    u16_t 	length;
    struct pktdef {
        struct ip_header ip;
	struct icmp_header icmp;
    } *pkt;

/* Don't send ICMP in response to ICMP */
    if (buf->protocol == prot_ICMP){
#ifdef NETSTAT
		++netstats.icmp_icmp;
#endif
		 return;
    }

    if ( (pkt = AllocatePkt(sizeof (struct pktdef)) ) == NULL) return;

#ifdef NETSTAT
     ++netstats.icmp_sent;
     ++netstats.icmp_out[type];
#endif

    pkt->ip.length = htons(sizeof( struct pktdef) );

    Move(buf,pkt->icmp.data,28);

    FillHeader((struct ip_header *)pkt,prot_ICMP,255);

    pkt->ip.source = buf->dest;
    pkt->ip.dest = buf->source;
/* Icmp header */
    pkt->icmp.type=type;
    pkt->icmp.code=code;
    pkt->icmp.cksum=0;
/* Do the arguments, 4 bytes long so fake as long! */
    if (unused) pkt->icmp.unused=*unused;
    else pkt->icmp.unused=0L;
/* ICMP checksum */
    pkt->icmp.cksum = htons(CheckSum(&pkt->icmp,sizeof(struct icmp_header) ));
    IPHeaderCheck(&pkt->ip);
    SendPacket(pkt,sizeof(struct pktdef));
}

/*
 *	Build a ping packet for host, with ident and sequence
 *	pointed to by unused
 *	Returns the time sent off to the interface
 *
 *	If len left blank then take default (48 data bytes)
 */

LWORD PkPing(ipaddr,unused,len)
	ipaddr_t ipaddr;
	u32_t	 *unused;
	u16_t	len;
{
    u16_t	length;
#ifdef PINGFILLINC
    u8_t	*ptr;
    u8_t	pad,data;
#endif
    struct pktdef *pkt;

/* Sort the length out, we already have 28 bytes in the header */
    if (len ) length= len-28+sizeof(struct pktdef);
    else { length=sizeof(struct pktdef); len=28; }


    if ( (pkt = AllocatePkt(length) ) == NULL) return 0L;

#ifdef NETSTAT
    ++netstats.icmp_sent;
    ++netstats.icmp_out[ECHO_REQUEST];
#endif

    pkt->ip.length=htons(length);
    FillHeader((struct ip_header *)pkt,prot_ICMP,255);
    pkt->ip.source = sysdata.myip;
    pkt->ip.dest = ipaddr;
/* Copy some data across to the packet */
    FillPing(pkt->icmp.data,len);
#ifdef PINGFILLINC
    ptr=pkt->icmp.data;
    data=0x20;
    while (len--) {
	*ptr++=data&63;
	++data;
    }
#endif

/* Icmp header */
    pkt->icmp.type=ECHO_REQUEST;
    pkt->icmp.code=0;
    pkt->icmp.cksum=0;
    pkt->icmp.unused=*unused;
/* ICMP checksum */
    pkt->icmp.cksum = htons(CheckSum(&pkt->icmp,length-20 ));
    IPHeaderCheck(&pkt->ip);
    SendPacket(pkt,length);
    return(current());

}



void ICMP_Handler(buf,len)
        BYTE *buf;
        WORD len;
{
        LWORD  addr;
	BYTE	*buf2;
        struct ip_header *ip;
        struct icmp_header *icmp;

        ip=buf;
        len=htons(ip->length)-((ip->version&15)*4);
        icmp=buf+(ip->version&15)*4;

        if ( CheckSum(icmp,len) ) { 
#ifdef NETSTAT
		++netstats.icmp_badck;
#endif
		return;
	}
#ifdef NETSTAT
		++netstats.icmp_inp[icmp->type];
#endif

	if (sysdata.usericmp) {
		if ( sysdata.usericmp(ip,icmp) == 0 ) return;
	}
        switch (icmp->type) {
/* Reply to a ping */
                case ECHO_REQUEST:
/* We now have to construct another packet for replying to a ping
 * so allocate space and sort fings out..
 */
			if ( (buf2=AllocatePkt(htons(ip->length)) ) ==0) return;
#ifdef NETSTAT
			++netstats.icmp_sent;
			++netstats.icmp_out[ECHO_REPLY];
#endif
                        icmp->type=ECHO_REPLY;
                        addr=ip->dest;
                        ip->dest = ip->source;
                        ip->source = addr;
                        icmp->cksum = htons(FixSum(icmp->cksum,0x800));
			Move(buf,buf2,htons(ip->length));
                        SendPacket(buf2,htons(ip->length));
                        break;
        }
        return;
}

/*
 * Fill in the basics of an ip header...
 */

void FillHeader(ip,prot,ttl)
	struct ip_header *ip;
	u8_t	prot;
	u8_t	ttl;
{
	ip->version=0x45;
	ip->tos=0;
	ip->ident=htons(sysdata.ipseq++);
	ip->frag=0;
	ip->protocol=prot;
	ip->ttl=ttl;
}

/*
 *	Fill up a ping packet with data aka gibberish
 */

FillPing(char *ptr, WORD len)
{
#pragma asm
	pop	de
	pop	bc	;len
	pop	hl	;ptr
	push	hl
	push	bc
	push	de
	ld	a,32
	ex	af,af
.fillping1
	ld	a,b
	or	c
	ret	z
	ex	af,af
	ld	(hl),a
	inc	a
	and	63
	ex	af,af
	inc	hl
	dec	bc
	jr	fillping1
#pragma endasm
}

/* Short routine to jimmy the ICMP checksum, we add one if we overflow */
/* Routine to jimmy ICMP checksum for ping reply */

/* int __CALLEE__ FixSum(int sum, int add) */
#pragma asm
._FixSum
        pop     bc      ;return address
        pop     de      ;to add
        pop     hl      ;checksum
; We are __CALLEE__ so dont restore stack
;        push    hl
;        push    de
        push    bc
;swap bytes over
        ld      a,l
        ld      l,h
        ld      h,a
        add     hl,de
        ret     nc
        inc     hl
	ret
#pragma endasm
