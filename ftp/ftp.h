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
 * $Id: ftp.h,v 1.5 2002-06-08 17:23:27 dom Exp $
 *
 */

#ifndef __FTP_H__
#define __FTP_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef ZSOCK
#ifndef SCCZ80
#include "../kernel/config.h"
#define CRLF "\r\n"
#define CR '\r'
#define LF '\n'
#else
#include <z88.h>
#define CRLF "\n\r"
#define CR '\n'
#define LF '\r'
#endif
#include <net/socket.h>
#include <net/inet.h>
#include <net/hton.h>
#include <net/resolv.h>
#include <net/misc.h>

#else
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef char i8_t;
typedef short i16_t;
typedef int i32_t;


typedef u32_t ipaddr_t;
typedef u16_t tcpport_t;

#define CRLF  "\r\n"
#define CR '\r'
#define LF '\n'

#endif

#define MAXARGS 10


typedef int tcpsock_t;             /* Should be okay for ZSock as well */

extern  unsigned char passive;
extern	int connected;

extern char      spare[];
extern char      buffer[];
extern tcpsock_t ftpdata_fd;
extern tcpsock_t ftpctrl_fd;

extern int  hash;


extern int    cmdargc;
extern char  *cmdargv[MAXARGS];

enum { RETR = 0, STOR };

enum { FALSE = 0, TRUE };


extern tcpsock_t net_connect_ip(ipaddr_t addr, tcpport_t port);




#endif
