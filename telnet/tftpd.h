#ifndef _TFTPD_H_
#define _TFTPD_H_


 /* Package so any stdio we do wants to be of the old type */
#define FDSTDIO 1

#include <stdio.h>
#include <z88.h>

#include <net/hton.h>
#include <net/tftp.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <net/misc.h>
#include <net/socket.h>

 /* This file is required for the kludge */

#include <net/tcpsock.h>


#define TFTPHDRLEN 4


extern int xmitfile();
extern int recvfile();

extern void nak();
extern void pnak();
extern void SendACK();


int tftp_daemon();

struct tftpinfo {
        u8_t    type;           /* RRQ/ WRQ - checking for bad requests */
        u8_t    mode;           /* octet/netascii */
        u16_t   block;
        u16_t   lastsize;
        int     fd;
        u8_t    *buf;
        u8_t    filename[FILENAME_MAX];
};

/* Package routines */

#define DAEMON_CALL	0x0a18
#define RECV_CALL	0x0c18
#define XMIT_CALL	0x0e18

#endif
