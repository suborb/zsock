/*
 *      Trivial FTP Daemon - Part of this is BSD derived, so here goes:
 *
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 *      But having said that, all the actual daemon stuff is mine, just
 *      the structure definitions come courtesy of them..
 *
 *      djm 7/5/99
 *
 *      Got to find a way to get timeouts into here...hmmmm...
 *
 *      djm 4/12/99
 *      Working! We keep a file open all the way through the
 *      transfer, this speeds things up remarkably, and allows
 *      us to writ (and get around the OZ bug...)
 *
 *      Code for timeout handling is in place, just need to
 *      add timeouts to UDP layer... 
 *
 *      djm 21/12/99
 *      Define FILECHEAT to use fopen()/fclose instead of open,
 *      creat/close - this is fine with current stdio model on z88
 *
 *      djm 11/1/2000
 *      Fixed problem in sendfile() which used fd instead of blk->fd
 *
 *      djm 23/4/2000
 *      Turning into a package <arrghh>
 *
 *      djm 5/2/2001
 *      Split up into package call and non package call files
 */

#include "tftpd.h"


/* Low level package initialisation */

#pragma -shareoffset=6
#pragma -shared-file

void pack_ayt()
{
#asm
        INCLUDE "#packages.def"
        push    af
        push    bc
        push    de
        push    hl
        push    ix
        push    iy
#endasm
        if (QueryPackage(LIB_TCP,0,0) == NULL) {
                /* Failure */
#asm
.pack_ayt_failure
                pop     iy
                pop     ix
                pop     hl
                pop     de
                pop     bc
                pop     af
                scf
                ret
#endasm
        }

        sock_listen(0L,69,DAEMON_CALL,prot_UDP);
#if 0
        iferror {
#asm
                jr  pack_ayt_failure
#endasm
        }
#endif
#asm
        pop     iy
        pop     ix
        pop     hl
        pop     de
        pop     bc
        pop     af
        and     a
#endasm
}

void pack_bye(void)
{
	killdaemon(69,prot_UDP);
        return_nc;
}

void pack_dat(void)
{
#asm
	ld	de,0
	ld	bc,0
	ld	a,0
#endasm
	return_nc;
}






tftp_daemon(tp,len,ip,up,s)
        struct tftphdr *tp;
        u16_t   len;
        struct ip_header *ip;
        struct udp_header *up;
        SOCKET  *s;
{

/* Pick up the request type */
        SOCKET       *n;
        struct tftpinfo *blk;
        u8_t   request;

	// printk("Got to daemon routine tp=%d len=%d ip=%d up=%d s=%d\n",tp,len,ip,up,s);
	if ( tp == EOF)	{
		sock_close(s);
		return_nc;
	}

        request = ntohs(tp->th_opcode);
	/* This handler is only called at the start, we spawn other "processes"
	 * which will take care of data later on..
	 */
        if ( request == WRQ  || request == RRQ  ) {
                u8_t    xfermode;
                u8_t    first;
                u8_t    *cp,*filename,*mode;
                filename = cp = tp->th_stuff;
again:
                while (cp < (u8_t *)tp + len) {
                        if ( *cp == '\0')
                                break;
                        cp++;
                }
                if (*cp != '\0') {
                        pnak(up,EBADOP);
                        return_nc;
                }
                if (first) {
                        mode = ++cp;
                        first = 0;
                        goto again;
                }
/* Now we have to convert the mode to lower case */
                strlwr(mode);
                if (strcmp(mode,"octet") == 0 ) xfermode=0;
                else if (strcmp(mode,"netascii") == 0 ) xfermode=1;
                else {pnak(up,EBADOP); return_nc; }
/* Okay, so we've got a valid filename and a valid mode, duplicate
 * ourselves!! If we return 0 we're in trouble and not enough mem
 * to even send an error message, so just return...
 */
                if ( (n=sock_pair_listen(ip->source,69,htons(up->srcport),0,prot_UDP) ) == 0 ) return_nc;
                if ( (blk=tcp_calloc(sizeof(struct tftpinfo),1)) == 0 ) { sock_close(n); return_nc ; } /* No memory for our little buffer */
                sock_setptr(n,blk);     /* Set user pointer */
                strcpy(blk->filename,filename);
                blk->type=request;
                blk->mode=xfermode;
                blk->block=0;
                blk->fd=0;
		// printk("tftpd request %d file: %s type: %s sock=%d\n",request,blk->filename,mode,n);
                if ( request == WRQ ) {
/* Starting a write, first off create the file... */
                        sock_sethandler(n,RECV_CALL);
                        if (CreateFile(blk) == 0 ) {
				// printk("Couldn't create file\n");
                                nak(n,ENOSPACE);
                                tftp_close(n);
                                return_nc;
                        }
/* That was successful, send an ack back! (and set timeout) */
                        SendACK(n,0);
                        sock_settimeout(n,TFTPTIMEOUT);
                        return_nc;
                } else {
/* They want us to send some data */
                        if ( (blk->buf=tcp_calloc(SEGSIZE+TFTPHDRLEN+10,1) ) == 0) { tftp_close(n); return_nc; }
                        sock_sethandler(n,XMIT_CALL);
                        blk->block=EOF;
                        SendBlock(n,0);
                        sock_settimeout(n,TFTPTIMEOUT);
 
                }
        } else pnak(ip,up,EBADOP);
}

/* Send file daemon */

xmit_daemon(tp,len,ip,up,s)
        struct tftphdr *tp;
        u16_t   len;
        struct ip_header *ip;
        struct udp_header *up;
        SOCKET  *s;
{
        struct tftpinfo *blk;

	// printk("Got to xmit daemon routine tp=%d len=%d ip=%d up=%d s=%d\n",tp,len,ip,up,s);

/* Check for timeouts... */
        if ( tp == EOF ) { 
                // printk("tftpd Xmitfile timed out\n");
                tftp_close(s); 
                return_nc; 
        }

/* Get our block into somewhere convenient..i.e. top of stack! */
        blk=sock_getptr(s);     /* Pick up the user pointer */
        sock_settimeout(s,TFTPTIMEOUT);
 
        if (blk->type != RRQ || htons(tp->th_opcode) != ACK ) {
/* Oh, something up here, nevermind, just send an error back */
                nak(s,EBADOP);
                tftp_close(s);
                return_nc;
        }
/* Check to see if we've been given an ack for the correct block */
        if ( blk->block==htons(tp->th_block) ) {
        /* Correct block, check size and close if necessary */
                if (blk->lastsize != SEGSIZE) {
                    // printk("xmit Ack for last packet\n");
                        tftp_close(s);
                        return_nc;
                }
                SendBlock(s,blk->block);
        } else {
                // printk("Rexmit\n");
                /* Send block after the ACK'd one */
                SendBlock(s,htons(tp->th_block) );
        }
}


/* Receive file daemon */
recv_daemon(tp,len,ip,up,s)
        struct tftphdr *tp;
        u16_t   len;
        struct ip_header *ip;
        struct udp_header *up;
        SOCKET  *s;
{
/* Get our block into somewhere convenient..i.e. top of stack! */
        struct tftpinfo *blk;
	// printk("Got to recv daemon routine tp=%d len=%d ip=%d up=%d s=%d\n",tp,len,ip,up,s);


/* Check for timeout */
        if ( tp == EOF ) { 
                // printk("tftpd Recvfile timeout\n");
                tftp_close(s); 
                return_nc; 
        }

        blk=sock_getptr(s);
        sock_settimeout(s,TFTPTIMEOUT);
 
        if (blk->type != WRQ || htons(tp->th_opcode) != DATA ) {
/* uh oh, boo boo, coming here and we're not receiving file...or we're
 * not being sent DATA */
                nak(s,EBADOP);
                tftp_close(s);
                return_nc;
        }
        len-=TFTPHDRLEN;         /* Length of datasent.. */
        if ( htons(tp->th_block) != blk->block+1 ) {
/* uh, oh, out of sequence block, try to resequence, send ack for 
 * previous block..
 */
                SendACK(s,blk->block);
                return_nc;
        }
        if ( (WriteBlock(blk,tp->th_data,len,blk->block) ) != len ) {
/* We haven't written all we should have done... */
                nak(s,ENOSPACE);
                tftp_close(s);
                return_nc;
        }
        SendACK(s,++blk->block);
        if (len < SEGSIZE ) {
/* End of file! Remove ourselves! */
        // printk("tftpd recv: end of file reached\n");
                tftp_close(s);
        }
        return_nc;
}

