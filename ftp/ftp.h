#ifndef __FTP_H__
#define __FTP_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef SCCZ80
#include <net/socket.h>
#include <net/inet.h>
#include <net/hton.h>
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
#endif

#define MAXARGS 10

typedef int tcpsock_t;


extern  unsigned char passive;
extern	int connected;

extern tcpsock_t ftpdata_fd;
extern tcpsock_t ftpctrl_fd;

extern int  hash;


extern int    cmdargc;
extern char  *cmdargv[MAXARGS];

enum { RETR = 0, STOR };

enum { FALSE = 0, TRUE };


extern tcpsock_t net_connect_ip(ipaddr_t addr, tcpport_t port);




#endif
