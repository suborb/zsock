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
 * $Id: tcp.c,v 1.11 2002-10-08 19:39:58 dom Exp $
 *
 * [This code owes a debt to Waterloo TCP, just to let ya know!]
 */

#include "zsock.h"

#define DEBUGTCP


/* Number of bytes offset into read buffer before we copy back */

#define READ_FLUSH      256

/* default size of the read/write buffers */
#ifdef Z80
#define BUF_SIZE        1024
#else
#define BUF_SIZE        1024
#endif






/*
 *      Routines used by other routines
 */

static void tcp_unthread(TCPSOCKET *);
static TCPSOCKET *tcp_find_socket(ip_header_t * ip, tcp_header_t * tp);
static void tcp_set_sockvj(TCPSOCKET * s);
static void tcp_unwind(TCPSOCKET * ds);
static void tcp_sendsoon(TCPSOCKET * t);
static void tcp_invalidate(TCPSOCKET * t);
static void tcp_processdata(TCPSOCKET * s, tcp_header_t * tp, i16_t len);
static void tcp_rst(ip_header_t * hisip, tcp_header_t * oldtp);
static int min(int b, int a);

static TCPSOCKET *tcp_new_buff_socket();
static TCPSOCKET *tcp_new_socket();
static void tcp_signal_task(TCPSOCKET *, u8_t);

static void tcp_parse_options(TCPSOCKET *s, tcp_header_t *tp);
#ifdef DEBUGTCP
#define TCPSEND(s) tcp_send(s,__LINE__)
static void tcp_send(TCPSOCKET * s, int line);
#else
#define TCPSEND(s) tcp_send(s)
static void tcp_send(TCPSOCKET * s);
#endif

static int tcp_output(TCPSOCKET *s,u32_t seqnum, u32_t acknum, u8_t flags, u16_t window,
		      void *payload, int payload_len,
		      void *tcpopts, int tcpopts_len);

static TCPSOCKET *tcp_add_buffers(TCPSOCKET *s);


/*
 *
 *      Initialize TCP services etc
 */

void tcp_init()
{
#if !SCCZ80 && !CYBIKO
    sysdata.tcpport = time(NULL)%65535;
#else
    sysdata.tcpport = 1024;
#endif

    sysdata.tcpfirst = NULL;
    service_registertcp();
#ifndef SCCZ80
    starthttpd();
#endif
}

/*
 *	Signal to a task abt imminent CLOSE/abort
 *	And then do it (mark closed/unthread)
 *	I think it's safe to increment close count here
 */

static void tcp_signal_task(TCPSOCKET * s, u8_t signal)
{
    if (s->datahandler) {
#ifdef __Z88__
	HCALL(0, signal, s);
#else
	s->datahandler(0,signal,s);
#endif
	tcp_unthread(s);
#ifdef NETSTAT
	++netstats.tcp_closed;
#endif
    } else {
/*
 * Task is supposed to close the socket itself.. 
 * ie call sock_shutdown
 */
	s->ip_type = CONN_CLOSED;
#ifdef NETSTAT
	++netstats.tcp_closed;
#endif
//      tcp_unwind(s);
    }
}


/*
 *	Resize the incoming buffer
 *	Discard previous gunk..
 */

int tcp_recvresize(TCPSOCKET * s, u16_t size)
{
    u8_t *ptr;

    if ((ptr = malloc(size)) == NULL)
	return s->recvsize;
    free(s->recvbuff);
    s->recvbuff = ptr;
    s->recvsize = size;
    s->recvread = 0;
    s->recvoffs = 0;
    return (size);
}

int tcp_sendresize(TCPSOCKET * s, u16_t size)
{
    u8_t *ptr;

    if ((ptr = malloc(size)) == NULL)
	return s->sendsize;
    free(s->sendbuff);
    s->sendbuff = ptr;
    s->sendsize = size;
    s->sendoffs = 0;
    return (size);
}




/*
 *      Open a TCP connection to a particular destiation
 */


TCPSOCKET *tcp_open(ipaddr_t ipdest,
		    tcpport_t lport, tcpport_t dport,
		    int (*datahandler)(), u8_t type)
{
    TCPSOCKET *s;

#if 1
    if (lport == 0) {
	if (get_uniqport(sysdata.tcpfirst, &sysdata.tcpport))  {
	    printf("No unique\n");
	    return_c(EADDRINUSE,NULL);
	}

	lport = sysdata.tcpport;
    }
#else
    if (lport == 0)
	lport = sysdata.tcpport++;
#endif
    if ((s = tcp_new_buff_socket()) == NULL) {
	printf("buffer\n");
	return_c(ENOMEM,NULL);	/* Find a new socket */
    }
    s->state = tcp_stateSYNSENT;
    SetLONGtimeout(s);
    s->hisaddr = ipdest;
    s->myport = htons(lport);
    s->myaddr = sysdata.myip;
    s->hisport = htons(dport);
    s->flags = tcp_flagSYN;
    s->unhappy = TRUE;
    s->datahandler = datahandler;
    s->handlertype = type;
    s->window = s->sendsize;
    tcp_set_sockvj(s);
    TCPSEND(s);
    s->rtt_time = set_ttimeout(1);	/* 1 second */
    return_ncv(s);
}

static void tcp_set_sockvj(TCPSOCKET * t)
{
    TCPSOCKET *s = t;
    s->mss = sysdata.mss;
    s->cwindow = 1;
    s->wwindow = 0;		/* Slow VJ algorithm */
    s->vj_sa = 36;
}


/*
 *      Passsive open: listen for a connecton
 */




TCPSOCKET *tcp_listen(ipaddr_t ipaddr, /* ipaddr_t myaddr, */
		      tcpport_t lport,
		      int (*datahandler)(), u8_t type, u16_t timeout)
{
    TCPSOCKET *s;

    if (lport == 0) {
	if (get_uniqport(sysdata.tcpfirst, &sysdata.tcpport))
	    return_c(EADDRINUSE,NULL);

	lport = sysdata.tcpport;
    } else if (CheckPort(sysdata.tcpfirst, lport)) {
	return_c(EADDRINUSE,NULL);
    }

    if ((s = tcp_new_socket()) == 0)
	return_c(ENOMEM,NULL);
    s->state = tcp_stateLISTEN;
    if (timeout)
	s->timeout = set_ttimeout(timeout);
    else
	s->timeout = 0;
    s->myport = htons(lport);
    s->hisaddr = ipaddr;
    /* s->myaddr  = myaddr; */
    s->hisport = 0;
    s->flags = 0;
    s->unhappy = FALSE;
    s->datahandler = datahandler;
    s->handlertype = type;
    return_ncv(s);
}


/*
 *      Send a FIN to a particular port - only works if open
 */

void tcp_close(TCPSOCKET * t)
{
    TCPSOCKET *s = t;

    if (s->state == tcp_stateESTAB || s->state == tcp_stateSYNREC
	|| s->state == tcp_stateESTCL) {
	if (s->sendoffs) {	/* Got to flush data first of all */
	    s->flags |= (tcp_flagPUSH | tcp_flagACK);
	    if (s->state < tcp_stateESTCL) {
		s->state = tcp_stateESTCL;
		tcp_sendsoon(s);
	    }
	} else {		/* Really closing */
	    s->flags = tcp_flagACK | tcp_flagFIN;
	    s->state = tcp_stateFINWT1;
	    SetLONGtimeout(s);
	    TCPSEND(s);
	}
	s->unhappy = TRUE;
    } else if (s->state == tcp_stateCLOSEWT) {
	/* Need to ack the fin and ge on with it */
	s->state = tcp_stateLASTACK;
	s->flags |= tcp_flagFIN;
	TCPSEND(s);
	s->unhappy = TRUE;
    } else {  /* If in listen etc, then make it closed */
	s->state = tcp_stateCLOSED;
    }
}

/* 
 *      Abort a TCP connection
 */

void tcp_abort(TCPSOCKET * t)
{
    TCPSOCKET *s = t;


    if (s->state != tcp_stateLISTEN && s->state != tcp_stateCLOSED) {
	s->flags = tcp_flagRST | tcp_flagACK;
	TCPSEND(s);
    }
    s->unhappy = FALSE;
    s->sendoffs = 0;
    s->state = tcp_stateCLOSED;
    /* Signal to task thats it's aborted */
    tcp_signal_task(s, handler_ABORT);
#ifdef OLD
    s->datahandler(s, 0, handler_ABORT);	/* HUH? */
    tcp_unthread(s);
#endif
}


/* 
 *      Write data to a TCP connection
 */

u16_t tcp_write(TCPSOCKET * s, void *dp, u16_t len)
{
    int x;


    if (s->state != tcp_stateESTAB)
	return (0);

    if (s->sendbuff == NULL)
	return (0);

    if (len > (x = s->sendsize - s->sendoffs))
	len = x;

    if (len) {
	memcpy(s->sendbuff + s->sendoffs, dp, len);
	s->sendoffs += len;
	if (s->sendoffs >= FLUSHLIM)
	    tcp_flush(s);
    }
    return (len);
}

/*
 *      Read data from a TCP connection
 */

u16_t tcp_read(TCPSOCKET * s, void *dp, u16_t len, u8_t flags)
{
    i16_t x;

    if (len > (x = s->recvoffs - s->recvread))
	len = x;

    if (s->recvbuff == 0)
	return 0;

    if (len) {
/* Move data to where the user wants it */
	memcpy(dp, s->recvbuff + s->recvread, len);
	if ((flags & MSG_PEEK) == MSG_PEEK)
	    return len;
	s->recvread += len;
	/* Now, we have to handle clearing the buffer etc... */
	if (s->recvread >= s->recvoffs) {
	    /* Read all that we can read */
	    s->recvread = 0;
	    s->recvoffs = 0;
	    return (len);
	}
	if (s->recvread > READ_FLUSH) {
	    /* we're a multiple of 256 into buffer copy back to start */
	    memcpy(s->recvbuff, s->recvbuff + s->recvread,
		   s->recvoffs - s->recvread);
	    s->recvoffs -= s->recvread;
	    s->recvread = 0;
	}	
    } else if (s->state == tcp_stateCLOSEWT)
	tcp_close(s);
    return (len);
}



/*
 *      Send pending data
 */

void tcp_flush(TCPSOCKET * t)
{
    TCPSOCKET *s = t;

    if (s->sendoffs) {
	s->flags |= tcp_flagPUSH;
	if (s->unacked == 0)
	    TCPSEND(s);
    }
}




/*
 *
 *      KERNEL ROUTINES
 *
 */


/*
 * Schedule a transmission pretty soon
 */

static void tcp_sendsoon(TCPSOCKET * t)
{
    u32_t temp;
    TCPSOCKET *s = t;

    temp = set_ttimeout(1);
    if (temp == s->rtt_time && s->rto < 2 && s->recent == 0) {
	s->karn_count = 0;
	TCPSEND(s);
	s->recent = 1;
	return;
    }
    if ((s->unhappy || s->sendoffs > 0 || s->karn_count == 1)
	&& (s->rtt_time < temp && s->rtt_time != 0))	// S. Lawson - handle 0
	return;

    s->rtt_time = set_ttimeout(1 + (s->rto >> 4));
    s->karn_count = 1;
}



#ifdef UNUSED
tcp_retransmit()
{
    u8_t x;
    TCPSOCKET *s;

    for (s = sysdata.tcpfirst; s; s = s->next) {
	if (s->state == tcp_stateLISTEN || s->ip_type != prot_TCP)
	    continue;
	x = FALSE;
	if (s->sendoffs != 0 || s->unhappy) {
	    s->recent = FALSE;
	    TCPSEND(s);
	    x = TRUE;
	}
	if (x || s->state != tcp_stateESTAB)
	    s->timeout -= tcp_RETRANSMITTIME;
	if (s->timeout <= 0) {
	    if (s->state == tcp_stateTIMEWT) {
		/* Closed */
		s->state = tcp_stateCLOSED;
		/* Closed conn, cleanup if dumb process */
		tcp_signal_task(s, handler_CLOSED);
	    } else {
		/* Timeout aborting */
		tcp_abort(s);
	    }
	}
    }
    return;
}
#else
void tcp_retransmit()
{
    TCPSOCKET *t, *s;
    
    for (s = sysdata.tcpfirst; s; s = t) {
	t = s->next;
	if (s->state == tcp_stateLISTEN)
	    continue;
	if (s->state == tcp_stateCLOSED) {
	    if (s->recvoffs == 0)
		tcp_abort(s);	/* unthread */
	    continue;
	}
	if (s->sendoffs || s->unhappy || s->karn_count == 1) {
	    /* retransmission strategy */
	    if (s->rtt_time && chk_timeout(s->rtt_time)) {
		s->rtt_time = 0L;
		/* strategy handles closed windows   J.D. + E.E. */
		if (s->window == 0 && s->karn_count == 2)
		    s->window = 1;
		if (s->karn_count == 0) {
		    /* if really did timeout */
		    s->karn_count = 2;
		    s->unacked = 0;
		    /* use the backed off rto - implied, no code necessary */
		    /* reduce the transmit window */
		    s->cwindow = ((s->cwindow + 1) * 3) >> 2;
		    if (s->cwindow == 0)
			s->cwindow = 1;
		    s->wwindow = 0;
		}
		if (s->sendoffs)
		    s->flags |= tcp_flagPUSH | tcp_flagACK;
		TCPSEND(s);
	    }

	}
	/* handle inactive tcp timeouts */
#ifdef INACTIVETO
	if (s->inactive_to) {
	    if (chk_timeout(s->inactive_to)) {
		/* this baby has timed out */
		/* "Connection timed out - no activity"; */
		tcp_close(s);
	    }
	}
#endif
	if (s->timeout && chk_timeout(s->timeout)) {
	    if (s->state == tcp_stateTIMEWT) {
		s->state = tcp_stateCLOSED;
		tcp_signal_task(s, handler_CLOSED);
		break;
	    } else if (s->state != tcp_stateESTAB
		       && s->state != tcp_stateESTCL) {
		/* "Timeout, aborting"; */
		tcp_abort(s);
		break;
	    }
	}
    }
}
#endif

static void tcp_invalidate(TCPSOCKET * t)
{
    TCPSOCKET *s = t;

    s->ip_type = BADIPTYPE;
    free(s->recvbuff);
    free(s->sendbuff);
    s->recvbuff = NULL;
    s->sendbuff = NULL;
    free(s);
}

/*
 * Shutdown a socket for good (basically free memory 
 * We assume some sanity that the socket has been closed
 * already
 */

void tcp_shutdown(TCPSOCKET * s)
{
    tcp_unwind(s);		/* Just to be certain */
    tcp_invalidate(s);
}


/*
 *  Unwind a user socket from the chain of command
 */

static void tcp_unwind(TCPSOCKET * ds)
{
    TCPSOCKET *s, **sp;

    sp = &sysdata.tcpfirst;
    for (;;) {
	s = *sp;
	if (s == ds) {
	    *sp = s->next;
	    break;
	}
	if (s == NULL)
	    break;
	sp = &s->next;
    }
    return;
}


/*
 * Unthread a socket from the socket list, if it's there 
 */

static void tcp_unthread(TCPSOCKET * ds)
{
    TCPSOCKET *s, **sp;

    sp = &sysdata.tcpfirst;
    for (;;) {
	s = *sp;
	if (s == ds) {
	    *sp = s->next;
	    tcp_invalidate(ds);
	    break;
	}
	if (s == NULL)
	    break;
	sp = &s->next;
    }
    return;
}

#if 0
TCPSOCKET *tcp_find_connect(tcpport_t *port)
{
    TCPSOCKET *s;

    for (s = sysdata.tcpfirst; s; s = s->next) {
	if ( s->ip_type == prot_TCP && s->state == tcp_stateESTAB && s->myport == port )
	    return s;
    }
    return NULL;
}
#endif


static TCPSOCKET *tcp_find_socket(ip_header_t * ip, tcp_header_t * tp)
{
    TCPSOCKET *s;

    for (s = sysdata.tcpfirst; s; s = s->next) {
	if (s->ip_type == prot_TCP && s->hisport != 0 &&
	    tp->dstport == s->myport &&
	    tp->srcport == s->hisport && ip->source == s->hisaddr)
	    return s;
    }
    return NULL;
}

/* Cancel handler */

void tcp_cancel(void *buf, int action, ipaddr_t * new)
{
    ip_header_t *ip;
    tcp_header_t *tp;
    TCPSOCKET *s;

    ip = buf;
    tp = buf + (ip->version & 15) * 4;
    s = tcp_find_socket(ip, tp);
    if (s != NULL) {
	switch (action) {
	case 1:		/* Halt it */
	    tcp_abort(s);
	    return;
#ifdef NEED_QUENCH
	case 2:		/* Slow it down */
	    s->cwindow = 1;
	    s->wwindow = 1;
	    s->rto <<= 2;
	    s->vj_sa <<= 2;
	    s->vj_sd <<= 2;
	    return;
#endif
	case 5:		/* Redirect */
	    s->hisaddr = *new;
	    return;
	}
    }
}



/* 
 * TCP handler, enters with pointer to IP header
 *
 * This routine called by the IP layer
 */


void tcp_handler(ip_header_t * ip, u16_t length)
{
    tcp_header_t *tp;
    signed int len;
    signed int diff;		/* If this is a long we have not enough mem! */
    TCPSOCKET *n;
    u8_t align;			/* Align to word boundary - optimization! */
    u8_t flags;
    TCPSOCKET *s;


    /* Get the start of the TCP header */
    tp = (tcp_header_t *)( (u8_t *)ip + (ip->version & 15) * 4);

    /* Calculate length of TCP data */
    len = ntohs(ip->length) - (ip->version & 15) * 4;

    /* Check active sockets */
    s = tcp_find_socket(ip, tp);

    /* Check for passive listening to certain hosts */
    if (s == NULL) {
	for (s = sysdata.tcpfirst; s; s = s->next) {
	    if (s->ip_type == prot_TCP && s->hisport == 0
		&& tp->dstport == s->myport 
		&& (s->hisaddr == 0L || s->hisaddr == ip->source)
		/* && (s->myaddr  == 0L || s->myaddr == ip->dest) */ )
		break;
	}
    }


    /* Do a checksum on the packet now */
    if (inet_cksum_pseudo(ip,tp, prot_TCP, len)) {
#ifdef NETSTAT
	++netstats.tcp_badck;
#endif
	return;
    }


    flags = tp->flags;

    /* Convert the sequence & ack numbers over to host format */
    tp->seqnum = ntohl(tp->seqnum);
    tp->acknum = ntohl(tp->acknum);


    /* No matching socket found, send a rst (only if no rst to us) */
    if (s == NULL) {
	if ((flags & tcp_flagRST) == 0) {
	    tcp_rst(ip, tp);
	}
	return;
    }

    if (flags & tcp_flagRST) {
#ifdef NETSTAT
	++netstats.tcp_rstrecvd;
#endif
	/* Connection reset */
	if (s->hisport) {
	    s->state = tcp_stateCLOSED;
	    tcp_signal_task(s, tcp_stateCLOSED);
	}
	return;
    }
#ifdef INACTIVETO
    s->inactive_to = set_ttimeout(INACTIVETO);
#endif
    /* Update retransmission info */
    if (s->state != tcp_stateLISTEN) {
	if (s->karn_count == 2) {
	    s->karn_count = 0;
	} else {
	    if (s->vj_last) {
		long diffticks;

		if ((diffticks = set_ttimeout(0) - s->vj_last) == 0) {
		    diffticks -= (long) (s->vj_sa >> 3);
		    s->vj_sa += (int) diffticks;
		    if (diffticks < 0)
			diffticks = -diffticks;
		    diffticks -= (s->vj_sd >> 2);
		    s->vj_sd += (int) diffticks;
		    if (s->vj_sa > MAXVJSA)
			s->vj_sa = MAXVJSA;
		    if (s->vj_sd > MAXVJSD)
			s->vj_sd = MAXVJSD;
		}
		/* Only compute rtt hence rrto after success */
		s->rto = 1 + ((s->vj_sa >> 2) + (s->vj_sd)) >> 1;
	    }
	    s->karn_count = 0;
	    if (s->wwindow++ >= s->cwindow) {
		s->cwindow++;
		s->wwindow = 0;
	    }
	}
/* Kludgey bit to optimize...hence defn of var here */
	{
	    long scheduleto;

	    scheduleto = set_ttimeout(s->rto + 2);
	    if (s->rtt_time < scheduleto)
		s->rtt_time = scheduleto;
	}
    }
#ifdef DEBUGTCP
    printf("Received flags %d len = %d state = %d\n",tp->flags,len,s->state);
#endif

    switch (s->state) {

    case tcp_stateLISTEN:
	if (flags & tcp_flagSYN) {
	    /* ZSock: If s->hisaddr == ip->source then the user has set
	     *  up a listening socket, so use that and don't alloc a new one
	     */
#ifdef NETSTAT
	    ++netstats.tcp_connreqs;
#endif
	    if (s->hisaddr == ip->source) {
		n = s;
		if ( tcp_add_buffers(n) == NULL ) /* No room */
		    return;
	    } else {
		if ((n = tcp_new_buff_socket()) == NULL) {
		    return;
		}
	    }
#ifdef NETSTAT
	    ++netstats.tcp_connaccs;
#endif
	    n->acknum = tp->seqnum + 1;
	    n->hisport = tp->srcport;
	    n->hisaddr = ip->source;
	    n->myaddr = ip->dest;
	    n->tos = ip->tos;	/* Pick up TOS */
	    n->myport = s->myport;

	    n->flags = tcp_flagSYN | tcp_flagACK;

	    n->datahandler = s->datahandler;
	    n->handlertype = s->handlertype;
	    tcp_set_sockvj(n);
	    tcp_parse_options(n,tp);
	    TCPSEND(n);
	    n->state = tcp_stateSYNREC;
	    n->unhappy = TRUE;
	    SetTIMEOUTtime(n);
	} else {
	    tcp_rst(ip, tp);   /* Out of sync packet, rst it */
	}
	return;			/* break; */

    case tcp_stateSYNSENT:
	if (flags & tcp_flagSYN) {

	    s->flags = tcp_flagACK;
	    SetTIMEOUTtime(s);
	    if ((flags & tcp_flagACK) && (tp->acknum == (s->seqnum + 1))) {
		/* Open! */
#ifdef NETSTAT
		++netstats.tcp_estab;
#endif
		s->state = tcp_stateESTAB;
		++s->seqnum;
		s->acknum = tp->seqnum + 1;
		/* Signal that conn is open, then feed data thru, not that
		 * should be any...
		 */
		if (s->datahandler) {
#ifdef __Z88__
		    HCALL(0, handler_OPEN, s);
#else
		    s->datahandler(0,handler_OPEN, s);
#endif
		}
		tcp_parse_options(s,tp);
		tcp_processdata(s, tp, len);
		s->unhappy = FALSE;
		TCPSEND(s);
	    } else {
		s->acknum++;
		s->state = tcp_stateSYNREC;
	    }
	}
	break;

    case tcp_stateSYNREC:
	if (flags & tcp_flagSYN) {
	    s->flags = tcp_flagSYN | tcp_flagACK;
	    s->unhappy = TRUE;
	    TCPSEND(s);
	    SetTIMEOUTtime(s);
	    return;
	    /* Retransmit of original syn */
	}
	if ((flags & tcp_flagACK) && tp->acknum == (s->seqnum + 1)) {
	    /* This is quite kludgey but it works... */
	    s->window = BUF_SIZE;
	    s->flags = tcp_flagACK;
	    ++s->seqnum;
#ifdef NETSTAT
	    ++netstats.tcp_estab;
#endif
	    s->state = tcp_stateESTAB;
	    SetTIMEOUTtime(s);
	    s->unhappy = FALSE;
	    if (s->datahandler) {
#ifdef __Z88__
		HCALL(0, handler_OPEN, s);
#else
		s->datahandler(0, handler_OPEN, s);
#endif
	    }
	    TCPSEND(s);		/* naughty */
	    return;
	    /* Synackc received - established */
	}
	break;


    case tcp_stateESTAB:
    case tcp_stateESTCL:
    case tcp_stateCLOSEWT:

	if ((flags & tcp_flagACK) == 0)
	    return;		/* must ack sommat */

	tcp_processdata(s, tp, len);


	if ( flags & tcp_flagFIN ) {
	    if ( s->state != tcp_stateCLOSEWT )
		s->acknum++;
	    s->state = tcp_stateCLOSEWT;
	    TCPSEND(s);
	    s->unhappy = FALSE;
	}



	if (s->state == tcp_stateESTCL)
	    tcp_close(s);

	return;

    case tcp_stateFINWT1:

	tcp_processdata(s, tp, len);

	/* Check if other tcp has acked all sent data and can change states */

	if ((flags & (tcp_flagFIN | tcp_flagACK)) == (tcp_flagFIN | tcp_flagACK)) {
	    if (tp->seqnum == s->acknum) {
		s->acknum++;	/* ack their FIN */
		if (tp->acknum >= (s->seqnum + 1)) {
		    /* Not simul close (they ACK'd our FIN) */
		    s->seqnum++;
		    s->state = tcp_stateTIMEWT;
		} else {
		    /* simul close */
		    s->state = tcp_stateCLOSING;
		}
		s->flags = tcp_flagACK;
		TCPSEND(s);
		s->unhappy = FALSE;
		if (s->state == tcp_stateTIMEWT)
		    SetLONGtimeout(s);
		else
		    SetTIMEOUTtime(s);
	    }
	} else if (flags & tcp_flagACK) {
	    /* Other side is legitamately acking us */
	    if ((tp->acknum == (s->seqnum + 1)) && (tp->seqnum == s->acknum) && (s->sendoffs == 0)) {
		s->seqnum++;
		s->state = tcp_stateFINWT2;
		SetTIMEOUTtime(s);
		s->unhappy = FALSE;
	    }
	}
	break;

    case tcp_stateFINWT2:

	/* They may be still transmitting data, must read it */
	tcp_processdata(s, tp, len);

	if ((flags & (tcp_flagACK | tcp_flagFIN)) == (tcp_flagACK | tcp_flagFIN)) {
	    if ((tp->acknum == s->seqnum) && (tp->seqnum == s->acknum)) {
		s->flags = tcp_flagACK;
		s->acknum++;
		TCPSEND(s);
		s->unhappy = FALSE;

		s->state = tcp_stateTIMEWT;
		SetLONGtimeout(s);
		if (s->datahandler) {
#ifdef __Z88__
		    HCALL(0, handler_CLOSED, s);
#else
		    s->datahandler(0, handler_CLOSED, s);
#endif
		}
		return;
	    }
	}
	break;

    case tcp_stateCLOSING:
	if ((flags & (tcp_flagACK | tcp_flagFIN)) == tcp_flagACK) {
	    /* 8/12/99 seqnum+1 seqnum++ change */
	    if ((tp->acknum == (s->seqnum /* + 1*/)) &&
		(tp->seqnum == s->acknum)) {
		//s->seqnum++;
		s->state = tcp_stateTIMEWT;
		SetTIMEOUTtime(s);
		s->unhappy = FALSE;
	    }
	}
	break;


    case tcp_stateLASTACK:
	if (flags & tcp_flagFIN) {
	    /* They lost our two packets, back up */
	    s->flags = tcp_flagACK | tcp_flagFIN;
	    TCPSEND(s);
	    SetTIMEOUTtime(s);
	    s->unhappy = FALSE;
	    return;
	} else {
	    if (tp->acknum == (s->seqnum + 1) && (tp->seqnum == s->acknum)) {
		s->state = tcp_stateCLOSED;	/*no 2msl need */
		tcp_signal_task(s,handler_CLOSED);  /* Close it up immediately */
		return;
	    }
	}
	break;

    case tcp_stateTIMEWT:
	if (flags & (tcp_flagACK | tcp_flagFIN) == (tcp_flagACK | tcp_flagFIN)) {
	    /* he needs an ack */
	    s->flags = tcp_flagACK;
	    TCPSEND(s);
	    s->unhappy = FALSE;
	    s->state = tcp_stateCLOSED;         /* Might this be bad so soon after an ACK? */
	    tcp_signal_task(s,handler_CLOSED);  /* Close it up immediately */
	}
	break;
    case tcp_stateCLOSED:
	tcp_rst(ip, tp);       /* Should't get a packet here, but may do if we close before shutdown */
	return;
    }
    if (s->unhappy)
	tcp_sendsoon(s);
}


/* Extract the mss out of the TCP header */
void tcp_parse_options(TCPSOCKET *s, tcp_header_t *tp)
{
    u16_t  opt_temp;
    u8_t  *options;
    u16_t  numoptions;

 
    if ((numoptions = (( tp->offset & 0xf0 ) >> 2 ) - sizeof(struct tcp_header))) {
        options = ((u8_t *) tp ) + sizeof(struct tcp_header);
        while (numoptions--) {
            switch (*options++) {
            case 0:             /* End of options */
                numoptions = 0;
            case 1:             /* NOP */
                break;
            case 2:             /* mss */
                if (*options == 4) {
                    opt_temp = htons(*(u16_t *)(options+1) );
                    if (opt_temp < s->mss)
                        s->mss = opt_temp;
                }
            default:
                numoptions -= (*options - 1);
                options += (*options - 1);
            }
        }
    }
}




/*
 * Process the data in an incoming packet.
 * Called from all states where incoming data can be received: established,
 * fin-wait-1, fin-wait-2
 *
 * We want to reject data that is out of window, i.e. 
 * if tp->seqnum > s->acknum+len, otherwise if we've missed a packet
 * we get out of order data - the other TCP will resend
 */

static void tcp_processdata(TCPSOCKET * s, tcp_header_t * tp, i16_t len)
{
    u16_t *options, numoptions;
    i16_t diff;
    u16_t x;
    u8_t *dp;

    diff = tp->acknum - s->seqnum;
    if (diff >= 0 && diff <= s->sendoffs) {
	s->sendoffs -= diff;
	s->unacked -= diff;
	if (s->sendoffs < 0)
	    s->sendoffs = 0;
	memcpy(s->sendbuff, diff + s->sendbuff, s->sendoffs);
	s->seqnum += diff;
    } else {
	/* handler condfused so set unacked to 0 */
	s->unacked = 0;
    }
    if (s->unacked < 0)
	s->unacked = 0;




    diff = s->acknum - tp->seqnum;
    if (tp->flags & tcp_flagSYN)
	--diff;
    

    dp = (void *)tp;

    x = ( tp->offset & 0xf0 ) >> 2;  /* Obtain the length of the TCP header */
    dp += x;
    len -= x;
    


    len -= diff;
    if (len >= 0) {
	dp += diff;
	//len -= diff;
#ifdef NETSTAT
	netstats.tcp_recvdlen += len;
#endif
	if (s->datahandler) {	 
#ifdef __Z88__
	    s->acknum += HCALL(dp, len, s);
#else
	    x = s->datahandler(dp, len, s);
	    s->acknum += x;

#endif
	} else {
	    /* No datahandler installed - i.e. user app, dump to a buffer
	     * taking care not to overwrite what is already there. We only
	     * ACK what we've copied
	     */
	    if (len > (x = s->recvsize - s->recvoffs))
		len = x;
	    if (len) {
		s->acknum += len;
		memcpy(s->recvbuff + s->recvoffs, dp, len);
		s->recvoffs += len;
	    }
	}
    }


    if ( s->sendoffs || len ||  ( s->unacked == 0 && s->state == tcp_stateCLOSEWT ) ) {
	TCPSEND(s);
    }


    s->unhappy = ((s->sendoffs) ? TRUE : FALSE);
    if (diff == 0 & s->unacked && chk_timeout(s->rtt_lasttran))
	s->unacked = 0;
    else
	tcp_sendsoon(s);
    SetTIMEOUTtime(s);
}



/*
 * Format and send an outgoing segment
 */
#ifdef DEBUGTCP
static void tcp_send(TCPSOCKET * s, int line)
#else
static void tcp_send(TCPSOCKET * s)
#endif
{
    u32_t optdata;
    int senddata, startdata, sendtotdata, sendtotlen;
    int i;
    u8_t *dp;
  




    if (s->karn_count != 2) {
	sendtotdata = s->sendoffs - s->unacked;
	sendtotdata = min(s->sendoffs, s->window) - s->unacked;
	startdata = s->unacked;
    } else {
	sendtotdata = (s->sendoffs >= s->window) ? s->window : s->sendoffs;
	startdata = 0;
    }
    if (sendtotdata < 0)
	sendtotdata = 0;



    sendtotlen = 0;
    for (i = 0; i < s->cwindow; i++) {
	senddata = min(sendtotdata, s->mss);

	if ( s->flags & tcp_flagSYN ) {
	    optdata = htonl( 2UL << 24  | 4UL << 16 )| htonl((u32_t)(s->mss));

	    tcp_output(s,s->seqnum,s->acknum,s->flags,s->window,NULL,0,&optdata,4);
	    return;
	}
	if ( s->state == tcp_stateCLOSEWT && s->unacked == 0 ) {
#ifdef DEBUGTCP
	    printf("i = %d line = %d\n",i,line);
#endif
	    tcp_output(s,s->seqnum,s->acknum,s->flags|tcp_flagFIN,s->window,NULL,0,NULL,0);
	    s->state = tcp_stateLASTACK;
	    return;
	} else {
#ifdef DEBUGTCP
	    printf("i = %d line = %d\n",i,line);
#endif
	    tcp_output(s,s->seqnum+startdata,s->acknum,s->flags,s->window,s->sendbuff + startdata,senddata,NULL,0);
	}

	sendtotlen += senddata;
	sendtotdata -= senddata;
	startdata += senddata;
	if (sendtotdata <= 0)
	    break;
    }

    s->unacked = startdata;
    s->vj_last = 0;
    if (s->karn_count == 2) {
	if (s->rto)
	    s->rto = (s->rto * 3) / 2;
	else
	    s->rto = 4;
    } else {
	/* vj last non zero,immediate response */
	if (s->unhappy || s->sendoffs)
	    s->vj_last = set_ttimeout(0);
	s->karn_count = 0;
    }
   
	
    s->rtt_time = set_ttimeout(s->rto + 2);
    if (sendtotlen > 0)
	s->rtt_lasttran = s->rtt_time + s->rto;
}

/*
 *      Send a rst packet
 */

static void tcp_rst(ip_header_t *hisip, tcp_header_t *oldtp)
{
    TCPSOCKET  s;
    u8_t oldflags;
    struct pktdef2 {
        ip_header_t   ip;
        tcp_header_t  tcp;
    } *pkt;

    oldflags = oldtp->flags;
    if (oldflags & tcp_flagRST)
        return;

    if (oldflags & tcp_flagACK) {
        oldtp->seqnum = oldtp->acknum;
        oldtp->acknum = 0;
    } else {
#if 0
        oldtp->acknum =
            oldtp->seqnum + (htons(hisip->length) -
                             ((hisip->version & 15) * 4));
#else
	if ( oldtp->flags & tcp_flagSYN )
	    oldtp->seqnum++;
	oldtp->acknum = oldtp->seqnum;
#endif
        oldtp->seqnum = 0;
    }

    s.myport  = oldtp->dstport;
    s.hisport = oldtp->srcport;
    s.myaddr  = hisip->dest;
    s.hisaddr = hisip->source;

    if ( tcp_output(&s,oldtp->seqnum,oldtp->acknum,tcp_flagACK|tcp_flagRST,0,NULL,0,NULL,0) == 0 ) {
#ifdef NETSTAT
        ++netstats.tcp_rstsent;
#endif
    }
    return;
}




/* Send a packet out - used by all output routines */
static int tcp_output(TCPSOCKET *s,u32_t seqnum, u32_t acknum, u8_t flags, u16_t window,
		      void *payload, int payload_len,
		      void *tcpopts, int tcpopts_len)
{
    u16_t   len;
    struct ptkdef {
        ip_header_t   ip;
        tcp_header_t  tcp;
	char          payload;
    } *pkt;

    len =  sizeof(ip_header_t) + sizeof(tcp_header_t) + payload_len + tcpopts_len;

    if ( ( pkt = pkt_alloc(len ) ) == NULL )
        return -1;

    pkt->ip.length   = htons(len);
    pkt->ip.source   = s->myaddr;
    pkt->ip.dest     = s->hisaddr;

    pkt->tcp.srcport = s->myport;
    pkt->tcp.dstport = s->hisport;
    pkt->tcp.seqnum  = htonl(seqnum);
    pkt->tcp.acknum  = htonl(acknum);
    pkt->tcp.flags   = flags;
    pkt->tcp.urgptr  = 0;
    pkt->tcp.cksum   = 0;
    pkt->tcp.window  = htons(window);
    
    pkt->tcp.offset  = (5 << 4 ) + ( tcpopts_len << 2 );

    /* Copy tcp options if present */
    memcpy(&pkt->payload,tcpopts,tcpopts_len);

    /* Copy the data */
    memcpy(&pkt->payload + tcpopts_len,payload,payload_len);



#ifdef DEBUGTCP
    printf("Sending packet of length %d flags %d\n",len,flags);
#endif

    /* Checksum */
    pkt->tcp.cksum   = inet_cksum_pseudo(&pkt->ip, &pkt->tcp,  prot_TCP,
                                         (len - sizeof(ip_header_t)));
    ip_send((ip_header_t *) pkt, len,prot_TCP,s->ttl);
    return 0;
}




/*
 *
 *      High Level Memory Routines
 *
 */

/* 
 * Allocate memory for the socket structure and the socket
 * queues (used by tcp_open and listen)
 *
 * Flag indicates whether to make the socket or not (for user
 * listen we don't want to recreate the socket since the user
 * already has a handle to it)
 */

static TCPSOCKET *tcp_new_buff_socket()
{
    TCPSOCKET *t,*s;

    /* Find a new socket, return NULL if we have no mem */
    if ( (s = tcp_new_socket()) == NULL)
	return (NULL);
    t = tcp_add_buffers(s);
    if ( t == NULL ) 
	free(s);
    return t;
}


static TCPSOCKET *tcp_add_buffers(TCPSOCKET *s)
{
    void *ptr;
    if ((ptr = malloc(BUF_SIZE)) == 0) {
	return (NULL);
    }
    s->sendbuff = ptr;
    if ((ptr = malloc(BUF_SIZE)) == 0) {
	free(s->sendbuff);
	return (NULL);
    }
    s->recvbuff = ptr;
    s->recvsize = s->sendsize = BUF_SIZE;
    return (s);
}

static TCPSOCKET *tcp_new_socket()
{
    TCPSOCKET *s;

    if ((s = calloc(sizeof(TCPSOCKET), 1)) == NULL)
	return NULL;
    s->next = sysdata.tcpfirst;
    sysdata.tcpfirst = s;
    s->seqnum = GetSeqNum();
    s->ip_type = prot_TCP;
    s->ttl = 255;		/* DEFAULT TCP ttl */
    s->tos = 0;			/* DEFAULT TCP tos */
    return (s);
}



static int min(int b, int a)
{
    if (a < b)
	return a;
    return b;
}
