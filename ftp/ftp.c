/*
 * Copyright (c) 2001-2002 Dominic Morris
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
 * $Id: ftp.c,v 1.6 2002-06-08 17:19:03 dom Exp $
 *
 */

#include "ftp.h"

/* Put all the data together so that it's not paged out by ZSock */

char file_buffer[1024];
char buffer[160];
char input_buffer[160];
char spare[100];


int  hash;

int ftp_returncode()
{
    char  result[5];
    int   res;

    while ( 1 ) {
	if ( net_getline(input_buffer,sizeof(input_buffer)) == -1 )
	    return -1;
	fputs(input_buffer,stdout);
	fputc('\n',stdout);
	strncpy(result,input_buffer,4);
	if ( isdigit(result[0]) && isdigit(result[1]) &&
	     isdigit(result[2]) && result[3] == ' ' ) {
	    res = atoi(result);
	    break;
	}
    }
    return res;
}
	
	
/* Send a command to the session */
int ftp_send(char *buf)
{
    net_sendline(buf);
    
    return (ftp_returncode());
}

int ftp_data(char *buf, int direction, FILE *fp)
{
    char      activebuf[50];
    ipaddr_t  dest;
    tcpport_t port;
    int       ret;
    u8_t     *ptr;
    int	      i;
  
    if ( passive ) {
	ret = ftp_send("PASV"CRLF);
	if ( ret != 227 )
	    return ret;
	ptr = input_buffer;
	while (*ptr && *ptr != '(')
	    ptr++;
	++ptr;
	dest = 0;
	for ( i = 0; i < 4; i++ ) {
	    dest = ( dest << 8 ) + atoi(ptr);
	    if ( ( ptr = strchr(ptr,',') ) == NULL ) {
		printf("Couldn't parse PASV response\n");
		return -1;
	    }
	    ++ptr;
	}
	port = atoi(ptr);
	if ( ( ptr = strchr(ptr,',') ) == NULL ) {
	    printf("Couldn't parse PASV response\n");
	    return -1;
	}
	++ptr;
	port = ( port << 8 ) + atoi(ptr);
	dest = htonl(dest);
#if 0
	port = htons(port);
#endif
	ftpdata_fd = net_connect_ip(dest,port);
    } else {
#define hiword(x)       ((u16_t)((x) >> 16))
#define loword(x)       ((u16_t)(x & 0xffff)) 
#define hibyte(x)       (((x) >> 8) & 0xff)
#define lobyte(x)       ((x) & 0xff)
	ftpdata_fd = net_listen();
	net_getsockinfo(&dest,&port);
	sprintf(activebuf,"PORT %u,%u,%u,%u,%u,%u"CRLF,
		hibyte(hiword(ntohl(dest))), lobyte(hiword(ntohl(dest))),
		hibyte(loword(ntohl(dest))), lobyte(loword(ntohl(dest))),
		hibyte(ntohs(port)), lobyte(ntohs(port)));
	ret = ftp_send(activebuf);
	if ( ret != 200 ) {
	    net_close_fd(ftpdata_fd);
	    return (ret);
	}
    }
    ret = ftp_send(buf);
    if ( passive || ret == 125 || ret == 150 ) {         /* 150 opening ascii connection 125 = data conn already open */
	if ( !passive ) {
	    if ( net_wait_connect(ftpdata_fd) == -1 ) {
		printf("Connect failed\n");
		net_close_fd(ftpdata_fd);
	    }
	}
	switch ( direction ) {
	case RETR:
	    ret = recvfile(fp);
	    break;
	case STOR:
	    ret = sendfile(fp);
	}
	net_close_fd(ftpdata_fd);
	ret = ftp_returncode();
    } else {
	net_close_fd(ftpdata_fd);
    }
    return (ret);
}

unsigned long sendfile(FILE *fp)
{
    int           s,i;
    unsigned long c = 0,total = 0UL;

    while ( ( s = fread(file_buffer,1,sizeof(file_buffer),fp) ) != 0  ) {
	net_write(ftpdata_fd,file_buffer,s);
#ifdef SCCZ80
	if ( getk() == 27 )
	    break;
#endif
	total += s;
	if ( hash && total/ 256 > c ) {
	    i = total / 256 - c;
	    c = total/256;
	    while ( i-- )
		printf("#"); 
	    fflush(stdout);
	}
    }
    printf("\n%lu bytes sent\n",total);
    return total;
}
		

unsigned long recvfile(FILE *fp)
{
    int           s,i;
    unsigned long c = 0,total = 0UL;
    while ( ( s = net_read(ftpdata_fd,file_buffer,sizeof(file_buffer)) ) != -1 ) {
	if ( s == 0 )
	    break;

	if ( fp == stdout ) {
	    file_buffer[s-1] = 0;
	    print_native();	   
	} else {
	    fwrite(file_buffer,1,s,fp);
	}
	total += s;
	if ( hash && fp != stdout &&  total/ 256 > c ) {
	    i = total / 256 - c;
	    c = total / 256;
	    while ( i-- )
		printf("#"); 
	    fflush(stdout);
	}
    }
    if ( fp != stdout )
	printf("\n%lu bytes read\n",total);
    return total;
	
}


void print_native()
{
    char  *ptr = file_buffer;

    while ( *ptr ) {
	if ( *ptr == CR )
	    fputc('\n',stdout);
	else if (*ptr != LF )
	    fputc(*ptr,stdout);
	++ptr;
    }
}
