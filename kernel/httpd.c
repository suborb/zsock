/*
 *      Stupid dumb http server for ZSock
 *
 *      djm 6/12/99
 *
 *	djm 21/12/99
 *	#define FILECHEAT to use fopen etc instead of open..
 */


#include <fcntl.h>
#include <malloc.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include "zsock.h"

#define HTTPBUFSIZ      256

char *FindMime(char *);
int httpd(TCPSOCKET *, BYTE *, WORD);
int httpd2(TCPSOCKET *, BYTE *, WORD);


char heada[]="HTTP/0.9 200 OK\n\l";
char headb[]="Server: Zhttpd v0.1\n\l";
char headc[]="Content-type: ";


char txt400a[]="<html><head><title>400 Bad Request</title></head>\n\l";
char txt400b[]="<body><h1>400 Bad Request.</h1>\n\l";
char txt400c[]="Your client sent an invalid request\n\l";

char txt404a[]="<html><head><title>404 Not Found.</title></head>\n\l";
char txt404b[]="<body><h1>404 Not Found.</h1>\n\l";
char txt404c[]="<hr><em><b>Zhttpd v0.1 &copy;1999 D Morris</b></em>\n\l";
char txt404d[]="</body></html>\n\l";


struct httpinfo {
        u16_t   fd;                  /* File descriptor */
        u16_t   read;
        u16_t   sent;
        u8_t    buffer[HTTPBUFSIZ];
};

starthttpd()
{
        tcp_listen(0L,80,httpd,0);
}


int httpd(s,addr,len)
        TCPSOCKET *s;
        BYTE *addr;
        WORD len;
{
        char    *mimetype;
        char    *filename;
        struct  httpinfo *blk;
        char    *ptr;

        if (addr && len ) {
                s->appptr=0;
                if ( (blk=calloc(sizeof(struct httpinfo),1)) == NULL) {
                /* No room for buffer, just 404 */
                        Send404(s);
                        goto getout;
                }
                s->appptr=blk;  /* Link us in */
                /* We've been sent something */
                if (strnicmp(addr,"GET ",4) !=0) {
                        /* Bad request */
                        Send400(s);
                        goto getout;
                }
                /* Do some truncation crap */
                ptr=addr+4;
                while (*ptr > 32 && *ptr < 128) ptr++;
                *ptr=0;
                filename=addr+4;
                if ( *filename=='/' ) ++filename;
                if ( *filename == '\0' || isspace(*filename) ) filename="index.htm";
                printf("Going to open file %s\n",filename);
#ifndef FILECHEAT
                if ( (blk->fd=open(filename,O_RDONLY,0)) == EOF ) {
#else
		if ( (blk->fd=fopen(filename,"r") ) == NULL ) {
#endif
                        blk->fd=0;
                        printf("Failed to open\n");
                        Send404(s);
                        goto getout;
                }
                printf("Opened..fd=%d\n",blk->fd);
                /* Now find the type.. */
                if ( (ptr=strrchr(filename,'.')) !=NULL ) {
                        /* Got an extension */
                        mimetype=FindMime(ptr+1);
                } else
                        mimetype="text/plain";
                printf("Mimetype is %s\n",mimetype);
                SendHead(s,mimetype);
                SendBlock(s);
                s->datahandler=httpd2;
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
 *      Secondary http daemon, called to continual shove 
 *      data out, addr is non-zero if called from process_data
 *      otherwise we should close up cos we're either abort
 *      or closed..
 */

int httpd2(s,addr,len)
        TCPSOCKET *s;
        BYTE *addr;
        WORD len;
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
        
        blk=s->appptr;

        if (blk==NULL) return;

        if (blk->read == NULL ) {
                blk->read=read(blk->fd,blk->buffer,HTTPBUFSIZ);
                blk->sent=0;
        }

        blk->sent+=sock_write(s,blk->buffer+blk->sent,blk->read-blk->sent);
/* Not gonna send any more data... */
        if (blk->read < HTTPBUFSIZ && blk->sent==blk->read ) {
                CloseHttpd(s);
                sock_close(s);
                return(1);
        } 
        if (blk->read == blk->sent) blk->read=NULL;
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
                s->appptr=0;
        }
}



                


SendHead(TCPSOCKET *s,char *type)
{
        sock_puts(s,heada);
        sock_puts(s,headb);
        sock_puts(s,headc);
        sock_puts(s,type);
        sock_puts(s,"\n\l\n\l");
}


Send400(TCPSOCKET *s)
{
        SendHead(s,"text/html");
        sock_puts(s,txt400a);
        sock_puts(s,txt400b);
        sock_puts(s,txt400c);
        sock_puts(s,txt404c);
        sock_puts(s,txt404d);
}



Send404(TCPSOCKET *s)
{
        SendHead(s,"text/html");
        sock_puts(s,txt404a);
        sock_puts(s,txt404b);
        sock_puts(s,txt404c);
        sock_puts(s,txt404d);
}


char *FindMime(char *ext)
{
        if (strnicmp(ext,"htm",3) == 0 ) return "text/html";
        else if (strnicmp(ext,"txt",3) == 0 ) return "text/plain";
        else if (strnicmp(ext,"gif",3) == 0 ) return "image/gif";
        else return "text/plain";
}
