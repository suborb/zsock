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
 * $Id: slip_gen.c,v 1.2 2002-06-01 21:43:18 dom Exp $
 *
 */



#include <stdio.h>
#include <sys/types.h>
#include "config.h"
#include "zsock.h"

#include <net/inet.h>
#include <net/device.h>

/*
 * Plugin has a buffer preset..
 */

#define MINHDR		20

#define MAXPKTSZ	1500

/*
 * Slip escape sequences, these are sorted out by the preproc
 *
 * SP_END     = end of packet
 * SP_ESC     = byte stuff coming up
 * SP_ESC_END = preceded by ESC means END datatype
 * SP_ESC_ESC = preceded by ESC means ESC datatype
 */

#define SP_END          192
#define SP_ESC          219
#define SP_ESC_END      220
#define SP_ESC_ESC      221


#define NUMCALLS 25




extern int SlipInit();
extern void SlipSendPkt(void *pkt, u16_t len);
extern void *SlipSend();
extern int SlipRead(void **pktin);
extern void SlipOnline();
extern void SlipOffline();
extern int  SlipStatus();


static void         *outchar();
static int           inchar();
static void          sendchar(u8_t ch);
static int           getinp();


/*
 *	Variables used by this module
 */

struct _slip {
    void *next;
    int   len;
};

static struct _slip  *slipfirst;
static struct _slip  *sliplast;
static unsigned char           slip_outstate;
static u16_t          sliplen;
static unsigned char *slip_outpkt;
static int            slipoffset;
static unsigned char           slipesch;
 
static u16_t         input_posn;
static unsigned char          inslipflag;

static int           online;
static int           sock;
static char          pktin[2048];


/*
 * Our little structure which contains all the info needed for the
 * SLIP driver!
 */
 

struct pktdrive z88slip = {
        "ZS0PKTDRV",                            /* Magic */
        "SLIP",                                 /* Type of driver */
        "ZSock Generic SLIP Driver by Dom",         /* (C) etc */
        SlipInit,                       /* Initialisation Routine */
        SlipSendPkt,                    /* Add to out-queue */
        SlipSend,                       /* Send func */
        SlipRead,                        /* Read func */
	SlipOnline,			/* Online */
	SlipOffline,			/* Offline */
	SlipStatus,			/* Status */
};



int SlipInit()
{
    printf("initialising slip\n");
#ifndef SCCZ80
    sock = open_terminal();
#endif
    slip_outstate = inslipflag = online = 0;
    slipfirst = NULL;
    online = 1;
    return sizeof(struct _slip);
}


int SlipStatus()
{
    return online;
}



void SlipOnline()
{
    online = 1;
}

/*
 * Turn device offline
 * If flag != 0 then hangup line
 * If flag == 0 then don't hangup
 */

void SlipOffline(int flag)
{
    online = 0;
}



int SlipRead(void **packet)
{
    int  i,len;

    if ( online == 0 )
	return 0;

    len = 0;
    for ( i = 0; i < NUMCALLS; i++ ) {
	len = inchar();
	if ( len )
	    break;
    }

    if ( len ) { 
	*packet = pktin;
	return len;
    }
    return 0;
}

/*
 *	We quit if we have finished sending a packet and need
 *	it to be freed. If the device is offline we keep things
 *	in the queue and wait till we're online again
 */

void *SlipSend()
{
    void *pkt;
    int  i;
    if  ( online == 0 )
	return 0;

    for ( i = 0; i < NUMCALLS; i++ ) {
	if ( slip_outstate == 0 )
	    return NULL;
	pkt = outchar();
	if ( pkt != NULL )
	    return pkt;
    }
    return NULL;
}



void SlipSendPkt(void *pkt, u16_t len)
{
    struct _slip *loop;
    struct _slip *llast;


    loop = ( pkt - sysdata.overhead);
    llast = sliplast;
    sliplast = loop;
    if ( llast != NULL ) {
        llast->next = loop;
    } else {
        slipfirst = loop;
	slip_outstate = 1; /* Indicate data - need to send end */
    }
    loop->next = NULL;
    loop->len  = len;      
}


void *outchar()
{
    struct _slip *loop;
    void         *pkt;

    if ( (slip_outstate & 2 ) == 0 ) {  /* Send END */
	sliplen = slipfirst->len;
	slipoffset = 0;
	slip_outpkt = ( (void *)slipfirst + sysdata.overhead);
	sendchar(SP_END);
	slip_outstate |= 2;
	return NULL;
    } else if ( ( slip_outstate & 4) == 4 ) {  /* Send escaped character */
	sendchar(slipesch);
	slip_outstate &= ~4;
    } else if ( ( slip_outstate & 8) == 8 ) {  /* Send final END char */
	sendchar(SP_END);
	/* Deal with finding the next packet */
	loop = slipfirst;
	slipfirst = loop->next;
	if ( slipfirst ) {
	    slip_outstate = 1;
	} else {
	    sliplast = NULL;
	    slipfirst = NULL;
	    slip_outstate = 0;
	}
	return NULL;
	return ( (void *)loop + sysdata.overhead );
    } else {
	unsigned char  c;

	c = *(slip_outpkt+slipoffset);

	switch ( c ) {
	case SP_END:
	    slipesch = SP_ESC_END;
	    sendchar(SP_ESC);
	    slip_outstate |= 4;
	    return NULL;
	case SP_ESC:
	    slipesch = SP_ESC_ESC;
	    sendchar(SP_ESC);
	    slip_outstate |= 4;
	    return NULL;
	default:
	    sendchar(c);
	}
    }
    ++slipoffset;
    if ( --sliplen == 0) {
	slip_outstate |= 8;
    }
    return NULL;
}

int inchar()
{
    int  c,len;

    c = getinp();

    if ( c == -1 )
	return 0;
    if ( ( inslipflag & 1 ) == 1 ) {
	inslipflag &= ~1;
	if ( c == SP_ESC_ESC )
	    c = SP_ESC;
	else if ( c == SP_ESC_END )
	    c = SP_END;
    } else if ( c == SP_ESC ) {
	inslipflag |= 1;
	return 0;
    } else if ( c == SP_END ) {
	if ( input_posn == 0 )
	    return 0;
	if ( input_posn < MINHDR ) {
	    input_posn = 0;
	    return 0;
	}
	len = input_posn;
	input_posn = 0;
	return len;
    }
    pktin[input_posn++] = c;
    return 0;
}


#ifndef SCCZ80
static void sendchar(unsigned char ch)
{
    write(sock,&ch,sizeof(unsigned char));
}


static int getinp()
{
    unsigned char  ch;

    if ( read(sock,&ch,sizeof(char)) == -1 ) {
	return -1;
    }
    return ch;
}
#endif

#ifdef __CPM__
static void sendchar(unsigned char ch)
{
    bdos(CPM_WPUN,ch);
}


static int getinp()
{
    unsigned char c = bdos(CPM_RRDR,0);
    if ( c != 255 )
	printf("Read %u\n",c);
    return ( c );
}
#endif


