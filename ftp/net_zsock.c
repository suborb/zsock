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
 * $Id: net_zsock.c,v 1.4 2002-06-08 17:19:03 dom Exp $
 *
 */

#include "ftp.h"


tcpsock_t    ftpdata_fd;
tcpsock_t    ftpctrl_fd;


ipaddr_t     client_addr;
ipaddr_t     server_addr;

tcpport_t    local_port;

int          connected;           /* Whether ctrl is connected */



int net_open_ctrl(char *hostname, tcpport_t port)
{
    struct sockinfo_t info;
    ipaddr_t          remote;

    if ( ( remote = resolve(hostname) ) == 0L ) {
	printf("Unknown host\n");
	return -1;
    }

    server_addr = remote;

    if ( (ftpctrl_fd = net_connect_ip(remote,port) ) != -1 ) {
	connected = TRUE;
	printf("Connected to %s:%d\n",hostname,port);
    } else {
	printf("Connection refused by %s:%d\n",hostname,port);
	net_close_fd(ftpctrl_fd);
	return -1;
    }

    if ( sock_getinfo(ftpctrl_fd,&info) != 0 ) {
	net_close_fd(ftpctrl_fd);
	return -1;
    }
    client_addr = info.local_addr;

    
    return 0;
}

tcpsock_t net_connect_ip(ipaddr_t addr, tcpport_t port)
{
    tcpsock_t  ret;

    if ( ( ret = sock_open(addr,port,0,prot_TCP) ) == 0 ) {
	return -1;
    }
    switch ( sock_waitopen(ret ) ) {
    case 1:
	return ret;
    default:
	net_close_fd(ret);
	return -1;
    }
}


int net_close_ctrl()
{
    net_close_fd(ftpctrl_fd);

}


int net_close_fd(tcpsock_t sock)
{
    if ( sock == -1 )
	return;
    sock_close(sock);
    if ( sock_waitclose(sock) == -1 )
	return -1;
    sock_shutdown(sock);
    return 0;
}

/* Wait for a listening socket to connect */
int net_wait_connect(tcpsock_t sock)
{
    if ( sock_waitopen(sock) == -1 ) {
	net_close_fd(sock);
	return -1;
    }
    return 1;
}



/* Send a line to the ctrl fd */
int net_sendline(char *buf)
{
    net_write(ftpctrl_fd,buf,strlen(buf));
    sock_flush(ftpctrl_fd);

}


/* Listen to connection from server_addr */
int net_listen()
{   
    ftpdata_fd = sock_listen(server_addr,0,0,prot_TCP);

    if ( ftpdata_fd == 0 ) 
	return -1;

    return ftpdata_fd;
}



int net_read(tcpsock_t fd, char *buf, int buflen)
{
    int  closed = 0;


    while ( sock_dataready(fd) == 0 ) {
	if ( closed )
	    return 0;
	if ( getk() == 27 )
	    return -1;
#ifdef SCCZ80
	GoTCP();
#else
	BUSYINT();
#endif
	closed = sock_closed(fd);
    }
    return(sock_read(fd,buf,buflen));
  
}


int net_write(tcpsock_t fd, char *buf, int buflen)
{
    int offs = 0;

    while ( ( offs += sock_write(fd,buf+offs,buflen-offs) ) != buflen ) {
	getk();
#ifdef SCCZ80
	GoTCP();
#else
	BUSYINT();
#endif
	if ( sock_closed(fd) )
	    return 0;
    }

    return offs;
}

int net_getsockinfo(ipaddr_t *addr, tcpport_t *port)
{
    struct sockinfo_t  info;

    if ( sock_getinfo(ftpdata_fd,&info) != 0 ) {
	net_close_fd(ftpdata_fd);
	return -1;
    }

    *addr = client_addr;
    *port = info.local_port;
    return 0;
}





   







char *net_readline(int sock)
{
    static int  pos = 0;
    static int  len = 0;
    int         start,offset,readflag;
    char       *ptr,ret;

    if ( len == 0 )
        readflag = 1;
    else
        readflag = 0;



    while ( 1 ) {
        if ( readflag ) {
            offset = net_read(sock,spare+len,100 - len - 1 );
            if ( offset <= 0 ) {
		return NULL;	
	    }
            len += offset;
        }

	if ( spare[pos] == LF )
	    ++pos;
        start = pos;
        while ( spare[pos] != CR && pos < len )
            ++pos;

        if ( pos == len ) {
            memcpy(spare,spare+start,pos-start);
	    len -= start;
            pos = 0;
            readflag = 1;
            continue;
        }
        /* We now have a line */
        spare[pos] = 0;
        ++pos;
        if ( pos >= len ) {
            len = 0;
            pos = 0;
        }
        return spare + start;
    }
    return NULL;
}

int net_getline(char *buf, int len)
{
    char  *ptr;

    ptr = net_readline(ftpctrl_fd);

    if ( ptr == NULL ) {
        buf[0] = 0;
	return -1;
    } else {      /* Hope it's shorter than what we ask for.. */
        strncpy(buf,ptr,len);
    }
    return 0;
}

#ifdef SCCZ80
int fgets_net(unsigned char *str, int max, int echo)
{
   unsigned char c;
   int ptr;
   ptr=0;

   while (1) {
      c = getk();
      GoTCP();
      if ( c == 0 )
	  continue;
      if (c == '\n' || c == '\r' || ptr == max-1) {
	  fputc_cons('\n');
	  return ptr;
      }
      if (c == 127  ) {
	  if ( ptr > 0 ) {
	      str[--ptr] = 0;
	      if ( echo ) {
		  fputc_cons(8);
		  fputc_cons(32);
		  fputc_cons(8);
	      }
	  }
      }
      else {
         str[ptr++] = c;
         str[ptr] = 0;
	 if ( echo ) {
	     fputc_cons(c);
	 }
      }
   }
}
#endif
