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
 * $Id: httpd.c,v 1.3 2002-05-13 21:47:43 dom Exp $
 *
 * Dumb http server (won't work without alteration)
 */




#include "zsock.h"

#define HTTPBUFSIZ      256

char *FindMime(char *);
int httpd(TCPSOCKET *, u8_t *, u16_t);
int httpd2(TCPSOCKET *, u8_t *, u16_t);


char head[]=  "HTTP/0.9 200 OK\n\l" \
               "Server: Zhttpd v0.1\n\l" \
               "Content-type: ";


char txt400[]="<html><head><title>400 Bad Request</title></head>\n\l" \
               "<body><h1>400 Bad Request.</h1>\n\l" \
               "Your client sent an invalid request\n\l";

char txt404[]="<html><head><title>404 Not Found.</title></head>\n\l" \
               "<body><h1>404 Not Found.</h1>\n\l" \
               "<hr><em><b>Zhttpd v0.1 &copy;1999 D Morris</b></em>\n\l" \
               "</body></html>\n\l";


struct httpinfo {
        u16_t   fd;                  /* File descriptor */
        u16_t   read;
        u16_t   sent;
        u8_t    buffer[HTTPBUFSIZ];
};

starthttpd()
{
        tcp_listen(0L,80,httpd,0,0);
}


int httpd(s,addr,len)
        TCPSOCKET *s;
        u8_t *addr;
        u16_t len;
{
    char    *mimetype;
    char    *filename;
    struct  httpinfo *blk;
    char    *ptr;

    if (addr && len ) {
	s->appptr=0;
	if ( (blk = calloc(sizeof(struct httpinfo),1)) == NULL) {
	    /* No room for buffer, just 404 */
	    Send404(s);
	    goto getout;
	}
	s->appptr = blk;  /* Link us in */
	/* We've been sent something */
	if (strnicmp(addr,"GET ",4) !=0) {
	    /* Bad request */
	    Send400(s);
	    goto getout;
	}
	/* Do some truncation crap */
	ptr = addr + 4;
	while (*ptr > 32 && *ptr < 128) 
	    ptr++;
	*ptr = 0;
	filename = addr + 4;
	if ( *filename=='/' ) 
	    ++filename;
	if ( *filename == '\0' || isspace(*filename) ) 
	    filename = "index.htm";
#ifndef FILECHEAT
	if ( (blk->fd=open(filename,O_RDONLY,0)) == EOF ) {
#else
	if ( (blk->fd=fopen(filename,"r") ) == NULL ) {
#endif
	    blk->fd=0;
	    Send404(s);
	    goto getout;
	}
	/* Now find the type.. */
	if ( (ptr=strrchr(filename,'.')) !=NULL ) {
	    /* Got an extension */
	    mimetype = FindMime(ptr+1);
	} else {
	    mimetype = "text/plain";
	}
	SendHead(s,mimetype);
	SendBlock(s);
	s->datahandler = httpd2;
	return(len);
	/* 
	 *      Get out of here, close the conn and return bytes read 
	 *      This is used if we 404 or 400 or other error
	 */
getout:
	CloseHttpd(s);
	sock_close(s);
    }
    return(len);
}

/*
 *      Secondary http daemon, called to continually shove 
 *      data out, addr is non-zero if called from process_data
 *      otherwise we should close up cos we're either abort
 *      or closed..
 */

int httpd2(s,addr,len)
        TCPSOCKET *s;
        u8_t *addr;
        u16_t len;
{
    if      (addr) {
	/* Sommat was acked, try sending again */
	SendBlock(s);
    } else 
	CloseHttpd(s);
    return(len);
}


/* Send a block */

SendBlock(TCPSOCKET *s)
{
    struct httpinfo *blk;
        
    blk = s->appptr;

    if ( blk == NULL) return;

    if (blk->read == 0 ) {
	blk->read = read(blk->fd,blk->buffer,HTTPBUFSIZ);
	blk->sent = 0;
    }

    blk->sent += sock_write(s,blk->buffer+blk->sent,blk->read-blk->sent);
    /* Not gonna send any more data... */
    if (blk->read < HTTPBUFSIZ && blk->sent==blk->read ) {
	CloseHttpd(s);
	sock_close(s);
	return(1);
    } 
    if (blk->read == blk->sent) 
	blk->read = 0;
    return(0);
}

CloseHttpd(TCPSOCKET *s)
{
    struct httpinfo *blk;
    blk=s->appptr;
    if (blk) {
	if (blk->fd) { 
#ifndef FILECHEAT
	    close(blk->fd);
#else
	    fclose(blk->fd);
#endif
	    blk->fd=0; 
	}
	free(blk);
	s->appptr = 0;
    }
}



                


SendHead(TCPSOCKET *s,char *type)
{
    sock_puts(s,head);
    sock_puts(s,type);
    sock_puts(s,"\n\l\n\l");
}


Send400(TCPSOCKET *s)
{
    SendHead(s,"text/html");
    sock_puts(s,txt400);  
}



Send404(TCPSOCKET *s)
{
    SendHead(s,"text/html");
    sock_puts(s,txt404);

}


char *FindMime(char *ext)
{
    if (strnicmp(ext,"htm",3) == 0 ) 
	return "text/html";
    else if (strnicmp(ext,"txt",3) == 0 ) 
	return "text/plain";
    else if (strnicmp(ext,"gif",3) == 0 ) 
	return "image/gif";
    else 
	return "text/plain";
}
