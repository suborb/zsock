/*
 * Z88 TCP Rewritten in C to Make it Slightly More Legible!
 *
 * This code owes a debt to Waterloo TCP, just to let ya know!
 *
 * djm 26/1/99
 *
 * djm 22/2/99 - Can finally accept an incoming connection, but we 
 *               send something which fubarrs up the miggy, so we get
 *               "flooded" with rsts.
 *
 * djm 23/2/99 - Incorporated changes from ostcp to CLOSING & FINWT1
 *               and tcp_processdata
 *
 * djm 24/2/99 - New socket flag s->sent - set by tcp_write so that
 *               listen (eg quote) doesn't send the same data twice
 *               though, I'll be damned if I can get it to close
 *               properly!!!
 *
 * djm 26/2/99 - Got the thing working after incorporating a few
 *               ideas for Waterloo TCP, though it transpires that
 *               the original works as well (was fubarred due to
 *               mistakes in lib functions!)
 *
 * djm 24/4/99 - Fixing a few things, such as notifying handler
 *               of conn open and closed, stopped tcp_write flushing
 *               for every write - this is okay for daemons since they
 *               are called by kernel, which does do flush
 *
 * djm 28/4/99 - Implementing receive buffers and tidying up program
 *               to remove the dependency on datahandler calls
 *
 * djm 7/12/99 - Starting to implement all complex gunk..implemented
 *               working (at least httpd can now send files) however
 *               qotd has stopped working :( at least till some
 *               data has been sent from remote site...weird
 *               Solved this problem by forcing a send...
 *
 * djm 9/12/99 - Killed annoying us=FINWT2 them=LASTACK bug
 *               sorted timers out..so they time(!) Datasize
 *               sending working now..
 *
 * djm 21/12/99- Rearrange socket to coincide with UDP layout,
 *		 s->sent flag removed from socket (unused)
 *
 * djm 10/1/2000-Fixed major hang for the daemons in tcp_send
 *		 thing we were basically getting a -ve size
 *		 for the buffer and it was blowing up from
 *		 there..
 *
 * djm 12/1/2000-Made things work for user listen processes eg
 *		 for ftp-data we check for match and don't clone
 *		 the socket on connect
 *
 * djm 9/2/2000 -Added call to handle daemons which are packages
 *    11/2/2000  Instead of s->datahandler() we use HCALL
 *		 This means a new variable in SOCKET * to tell
 *		 us what type of routine - could dump this by
 *		 having internal daemons use packages but that
 *		 is just silly!
 *
 * PROBLEMS:
 *
 * 6/1/2000	Increasing the size of the TCP window causes
 *		an almighty barf in the telnet client when
 *		we get to the prompt stage...dunno what's
 *		happening really...
 */

#include <stdio.h>
#include <net/hton.h>
#include "zsock.h"

/* Number of bytes offset into read buffer before we copy back */

#define READ_FLUSH      256

/* default size of the read/write buffers */
#define BUF_SIZE        512

/*
 *	Some "useful" routines in ip.c
 */

extern SendPacket();
extern WORD __FASTCALL__ IPHeaderCheck(void *);
extern PsHeaderSum();
extern TCPCSum();


/* 
 *      Memory Functions
 *      These will have to be changed for far model
 *
 *      These functions are very dodgy to say the least and
 *      allocate memory from a string!!!
 */ 


extern BYTE *AllocatePkt();
extern FreePacket();

/*
 *	Routine that sets up internal daemons
 */

extern RegisterServicesTCP();      /* Set up some services */

/*
 *      User Servicable Routines
 *	(Don't use these..use sock_ routines instead)
 */

void tcp_close(TCPSOCKET *);
void tcp_abort(TCPSOCKET *);
void tcp_unthread(TCPSOCKET *);
int tcp_write(TCPSOCKET *, u8_t *, u16_t);
int tcp_read(TCPSOCKET *, u8_t *, i16_t);


/*
 *      Routines used by other routines
 */

extern LWORD GetSeqNum();
int __FASTCALL__ tcp_GetDataOffset(struct tcp_header *tp);
TCPSOCKET *NewBuffSocket();
TCPSOCKET *NewTCPSocket();
void *newsocket();
void SignalTCPTask(TCPSOCKET *,u8_t);


#ifdef DEBUGTCP
#define TCPSEND(s) tcp_send(s,__LINE)
#else
#define TCPSEND(s) tcp_send(s)
#endif


/*
 *
 *      Initialize TCP services etc
 */

TcpInit()
{
        sysdata.tcpport=1024;
        sysdata.tcpfirst = 0;
        RegisterServicesTCP();
//        starthttpd();
}

/*
 *	Signal to a task abt imminent CLOSE/abort
 *	And then do it (mark closed/unthread)
 *	I think it's safe to increment close count here
 */

void SignalTCPTask(s,signal)
        TCPSOCKET *s;
        u8_t    signal;
{
    if (s->datahandler) {
	HCALL(0,signal,s);
        tcp_unthread(s);
#ifdef NETSTAT
	++netstats.tcp_closed;
#endif
    } else  {
/*
 * Task is supposed to close the socket itself.. 
 * ie call sock_shutdown
 */
        s->ip_type=CONN_CLOSED;
#ifdef NETSTAT
	++netstats.tcp_closed;
#endif
//	tcp_unwind(s);
    }
}


/*
 *	Resize the incoming buffer
 *	Discard previous gunk..
 */

int tcp_recvresize(s,size)
	TCPSOCKET *s;
	WORD	size;
{
	BYTE	*ptr;
	if	( (ptr=malloc(size) ) == NULL ) return s->recvsize;
	free (s->recvbuff);
	s->recvbuff=ptr;
	s->recvsize=size;
	s->recvread=0; s->recvoffs=0;
	return(size);
}

int tcp_sendresize(s,size)
	TCPSOCKET *s;
	WORD	size;
{
	BYTE	*ptr;
	if	( (ptr=malloc(size) ) == NULL ) return s->sendsize;
	free (s->sendbuff);
	s->sendbuff=ptr;
	s->sendsize=size;
	s->sendoffs=0;
	return(size);
}
		



/*
 *      Open a TCP connection to a particular destiation
 */



TCPSOCKET *tcp_open(ipdest,lport,dport,datahandler,type)
        LWORD   ipdest;
        WORD    lport,dport;
        int (*datahandler)();
	BYTE	type;	/* Handler type... */
{
        TCPSOCKET *s;
        if ( (s=NewBuffSocket(YES) ) == 0 ) return_c (ENOMEM);  /* Find a new socket */
        s->state = tcp_stateSYNSENT;
        SetLONGtimeout(s);
        if (lport == 0) lport = sysdata.tcpport++;
        s->hisaddr=ipdest;
        s->myport = htons(lport);
        s->myaddr = sysdata.myip;
        s->hisport = htons(dport);
        s->flags = tcp_flagSYN;
        s->unhappy = TRUE;
        s->datahandler = datahandler;
	s->handlertype=type;
/* Now the complex gunk */
	set_sockvj(s);
        TCPSEND(s);
        s->rtt_time=set_ttimeout(1);  /* 1 second */
        return_nc(s);
}

void set_sockvj(TCPSOCKET *s)
{
        s->mss = sysdata.mss;
        s->cwindow=1;
        s->wwindow=0;   /* Slow VJ algorithm */
        s->vj_sa=36;
}


/*
 *      Passsive open: listen for a connecton
 */


TCPSOCKET *tcp_listen(ipaddr,lport,datahandler,type, timeout)
	ipaddr_t ipaddr;
        WORD    lport;
        int     (*datahandler)();
	BYTE	type;		/* 0=internal 1=package */
        WORD    timeout;
{
        TCPSOCKET *s;
	if (CheckPort(sysdata.tcpfirst,lport)) return_c EADDRINUSE;
        if ( (s=NewTCPSocket() )==0 ) return_c(ENOMEM);
        s->state = tcp_stateLISTEN;
	if (timeout)
        	s->timeout = set_ttimeout(timeout);
	else
		s->timeout = 0;
        s->myport = htons(lport);
	s->hisaddr=ipaddr;
        s->hisport = 0;
        s->flags = 0;
        s->unhappy = FALSE;
        s->datahandler = datahandler;
	s->handlertype=type;
        return_nc(s);
}


/*
 *      Send a FIN to a particular port - only works if open
 */

void tcp_close(t)
        TCPSOCKET *t;
{
	TCPSOCKET *s=t;
        if ( s->state == tcp_stateESTAB || s->state == tcp_stateSYNREC || s->state== tcp_stateESTCL) {
                if (s->sendoffs) {       /* Got to flush data first of all*/
                        s->flags |= (tcp_flagPUSH | tcp_flagACK);
                        if (s->state < tcp_stateESTCL ) {
                                s->state = tcp_stateESTCL;
                                tcp_sendsoon(s);
                        } 
                } else { /* Really closing */
                        s->flags = tcp_flagACK | tcp_flagFIN;
                        s->state = tcp_stateFINWT1;
                        SetLONGtimeout(s);
                        TCPSEND(s);
                }
                s->unhappy=TRUE;
        } else if (s->state == tcp_stateCLOSEWT ) {
                /* Need to ack the fin and ge on with it */
                s->state = tcp_stateLASTACK;
                s->flags |= tcp_flagFIN;
                TCPSEND(s);
                s->unhappy=TRUE;
        }
}

/* 
 *      Abort a TCP connection
 */

void tcp_abort(t)
        TCPSOCKET *t;
{
	TCPSOCKET *s=t;
        if ( s->state !=tcp_stateLISTEN && s->state != tcp_stateCLOSED ) {
                s->flags = tcp_flagRST | tcp_flagACK;
                TCPSEND(s);
        }
        s->unhappy = FALSE;
        s->sendoffs=0;
        s->state = tcp_stateCLOSED;
/* Signal to task thats it's aborted */
        SignalTCPTask(s,handler_ABORT);
#ifdef OLD
        s->datahandler(s, 0, handler_ABORT);       /* HUH? */
        tcp_unthread(s);
#endif
}


/* 
 *      Write data to a TCP connection
 */

tcp_write(s,dp,len)
        TCPSOCKET *s;
        BYTE *dp;
        WORD len;
{
        int     x;

        if      (s->state != tcp_stateESTAB ) return(0);

	if	(s->sendbuff == 0 ) return (0);

        if      (len > ( x= s->sendsize - s->sendoffs ) ) len=x;

        if      (len ) {
                Move(dp,s->sendbuff+s->sendoffs,len);
                s->sendoffs+=len;
                if (s->sendoffs >= FLUSHLIM ) 
                        tcp_flush(s);
        }
        return(len);
}

/*
 *      Read data from a TCP connection
 */

int tcp_read(s,dp,len)
        TCPSOCKET *s;
        BYTE *dp;
        i16_t len;
{
        i16_t   x;

        if      (len > (x= s->recvoffs - s->recvread)) len=x;

	if	(s->recvbuff == 0 ) return 0;

        if      (len) {
/* Move data to where the user wants it */
                Move(s->recvbuff+s->recvread,dp,len);
                s->recvread+=len;
/* Now, we have to handle clearing the buffer etc... */
                if ( s->recvread >= s->recvoffs) {
/* Read all that we can read */
                        s->recvread=0; s->recvoffs=0;
                        return (len);
                }
                if ( s->recvread > READ_FLUSH )  {
/* we're a multiple of 256 into buffer copy back to start */
                        Move(s->recvbuff+s->recvread,s->recvbuff,s->recvoffs-s->recvread);
                        s->recvoffs-=s->recvread;
                        s->recvread=0;
                }
        } else if (s->state==tcp_stateCLOSEWT)
		tcp_close(s);
        return(len);
}



/*
 *      Send pending data
 */

tcp_flush(t)
        TCPSOCKET *t;
{
        TCPSOCKET *s=t;
        if      (s->sendoffs) {
                s->flags |=tcp_flagPUSH;
                if (s->unacked==0)
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

tcp_sendsoon(TCPSOCKET *t)
{
        LWORD    temp;
	TCPSOCKET *s=t;

        temp=set_ttimeout(1);
        if ( temp == s->rtt_time && s->rto < 2 && s->recent == 0 ) {
            s->karn_count = 0;
            TCPSEND(s);
            s->recent = 1;
            return;
        }
        if ((s->unhappy || s->sendoffs > 0 || s->karn_count == 1)
          && (s->rtt_time < temp && s->rtt_time != 0))  // S. Lawson - handle 0
            return;

        s->rtt_time = set_ttimeout( 1 + (s->rto >> 4) );
        s->karn_count = 1;
}



#ifdef UNUSED
tcp_retransmit()
{
        BYTE    x;
        TCPSOCKET *s;

        for ( s = sysdata.tcpfirst; s ;s = s->next ) {
                if (s->state == tcp_stateLISTEN || s->ip_type != prot_TCP ) continue;
                x=FALSE;
                if ( s->sendoffs != 0 || s->unhappy ) {
                        s->recent=FALSE;
                        TCPSEND(s);
                        x=TRUE;
                }
                if ( x || s->state != tcp_stateESTAB )
                        s->timeout -= tcp_RETRANSMITTIME;
                if ( s->timeout <= 0 ) {
                        if ( s->state == tcp_stateTIMEWT ) {
                                /* Closed */
                                s->state = tcp_stateCLOSED;
/* Closed conn, cleanup if dumb process */
                                SignalTCPTask(s,handler_CLOSED);
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
    TCPSOCKET *s;

    for ( s = sysdata.tcpfirst; s; s = s->next ) {
        if ( s->state==tcp_stateLISTEN ) continue;
        if ( s->state==tcp_stateCLOSED ) {
           if ( s->recvoffs == 0) tcp_abort(s); /* unthread */
           continue;
        }
        if ( s->sendoffs  || s->unhappy || s->karn_count==1) {
            /* retransmission strategy */
            if ( s->rtt_time && chk_timeout( s->rtt_time )) {
               s->rtt_time = 0L;
                /* strategy handles closed windows   J.D. + E.E. */
               if (s->window == 0 && s->karn_count == 2)
                  s->window = 1;
                if ( s->karn_count == 0 ) {
                    /* if really did timeout */
                    s->karn_count = 2;
                    s->unacked = 0;
                    /* use the backed off rto - implied, no code necessary */
                    /* reduce the transmit window */
                    s->cwindow =  ((s->cwindow + 1) * 3) >> 2;
                    if ( s->cwindow == 0 ) s->cwindow = 1;
                    s->wwindow = 0;
                }
                if (s->sendoffs)
                    s->flags |= tcp_flagPUSH | tcp_flagACK;
                TCPSEND(s);
            }

        }
        /* handle inactive tcp timeouts */
#ifdef INACTIVETO
        if ( s->inactive_to ) {
            if ( chk_timeout( s->inactive_to)) {
                /* this baby has timed out */
                /* "Connection timed out - no activity"; */
                tcp_close ( s );
            }
        }
#endif
        if ( s->timeout && chk_timeout( s->timeout)) {
            if ( s->state == tcp_stateTIMEWT ) {
                s->state = tcp_stateCLOSED;
                SignalTCPTask(s,handler_CLOSED);
                break;
            } else if (s->state != tcp_stateESTAB && s->state != tcp_stateESTCL ) {
                /* "Timeout, aborting"; */
                tcp_abort(s);
                break;
            } 
        }
    }
}
#endif

void tcp_invalidate(t)
	TCPSOCKET *t;
{
	TCPSOCKET *s=t;
	s->ip_type=BADIPTYPE;
        free(s->recvbuff);
        free(s->sendbuff);
	s->recvbuff=0;
	s->sendbuff=0;
        free(s);
}

/*
 * Shutdown a socket for good (basically free memory 
 * We assume some sanity that the socket has been closed
 * already
 */

void tcp_shutdown(s)
	TCPSOCKET *s;
{
	tcp_unwind(s);	/* Just to be certain */
	tcp_invalidate(s);
}


/*
 *  Unwind a user socket from the chain of command
 */

void tcp_unwind(ds)
        TCPSOCKET *ds;
{
        TCPSOCKET *s, **sp;


        sp = &sysdata.tcpfirst;
        for (;;) {
                s = *sp;
                if ( s == ds ) {
                        *sp = s->next;
                        break;
                }
                if ( s == NULL ) break;
                sp = &s->next;
        }
        return;
}


/*
 * Unthread a socket from the socket list, if it's there 
 */

void tcp_unthread(ds)
        TCPSOCKET *ds;
{
        TCPSOCKET *s, **sp;


        sp = &sysdata.tcpfirst;
        for (;;) {
                s = *sp;
                if ( s == ds ) {
                        *sp = s->next;
			tcp_invalidate(ds);
                        break;
                }
                if ( s == NULL ) break;
                sp = &s->next;
        }
        return;
}

TCPSOCKET *tcp_find_socket(struct ip_header *ip, struct tcp_header *tp)
{
	TCPSOCKET *s;
        for ( s = sysdata.tcpfirst ; s ; s = s->next ) 
                if ( s->ip_type == prot_TCP && s->hisport != 0 &&
                     tp->dstport == s->myport &&
                     tp->srcport == s->hisport &&
                     ip->source == s->hisaddr) 
			return s;
	return NULL;
}

/* Cancel handler */

void tcp_cancel(char *buf, int action, ipaddr_t *new)
{
	struct ip_header *ip;
	struct tcp_header *tp;
	TCPSOCKET *s;

	ip = buf;
        tp = buf + (ip->version&15)*4;
	s = tcp_find_socket(ip,tp);
	if ( s != NULL ) {
		switch (action) {
		case 1:		/* Halt it */
			tcp_abort(s);
			return;
#ifdef NEED_QUENCH
		case 2:		/* Slow it down */
			s->cwindow = 1;
			s->wwindow = 1;
			s->rto <<= 2;
			s->vj_sa <<=2;
			s->vj_sd <<=2;
			return;
#endif
		case 5:		/* Redirect */
			s->hisaddr = new;
			return;
		}
	}
}



/* 
 * TCP handler, enters with pointer to IP header
 *
 * This routine called by the IP layer
 */


tcp_handler(ip,length)
        struct ip_header *ip;
        WORD    length;
{
        struct tcp_header *tp;
        BYTE    *t;
        signed int    len;
        signed int    diff; /* If this is a long we have not enough mem! */
        TCPSOCKET *n;
        BYTE    align;  /* Align to word boundary - optimization! */
        BYTE    flags;
        TCPSOCKET *s;

/* 
 * Get the length of the Internet Header and point to tp to TCP header
 *
 * This is a bit kludgey, but the best we can do without casts!
 */

        t = ip;
        tp = t + (ip->version&15)*4;

/*
 * Calculate length of TCP data
 */
        len = ntohs(ip->length) - (ip->version&15)*4;


/* Check active sockets */

	s = tcp_find_socket(ip,tp);
/* Now for passive handles listening for certain host as well
 * needed for ftp to work
 */
        if ( s == NULL ) {
                for ( s = sysdata.tcpfirst ; s ; s = s->next ) {
                   if ( s->ip_type == prot_TCP && s->hisport == 0 && tp->dstport == s->myport && ( s->hisaddr==0L || s->hisaddr==ip->source)  ) break;
                }
        }


/* Do a checksum on the packet now */
        if (TCPCheckSum(ip,prot_TCP, len ) ){
#ifdef NETSTAT
		++netstats.tcp_badck;
#endif
                return;
        }


        flags = tp->flags;

/* Convert the sequence & ack numbers over to our format */
        tp->seqnum = ntohl(tp->seqnum);
        tp->acknum = ntohl(tp->acknum);


/* No matching socket found, send a rst (only if no rst to us) */
        if ( s == NULL ) {
                if ( (flags&tcp_flagRST) == 0 ) {
                        tcp_rst(ip,tp);
                } 
                return;
        }

        if ( flags & tcp_flagRST ) {
#ifdef NETSTAT
		++netstats.tcp_rstrecvd;
#endif
                /* Connection reset */
                if (s->hisport) {
                        s->state = tcp_stateCLOSED;
                        SignalTCPTask(s,tcp_stateCLOSED);
                }
                return;
        }
#ifdef INACTIVETO
	s->inactive_to = set_ttimeout(INACTIVETO);
#endif
/* 
 * Update retransmission info
 */
 if (s != tcp_stateLISTEN)  {
        if (s->karn_count == 2 ) {
                s->karn_count=0;
        } else {
                if (s->vj_last) {
                        long diffticks;
                        if ((diffticks=set_ttimeout(0) - s->vj_last) == 0 ) {
                                diffticks -=(long)   (s->vj_sa>>3);
                                s->vj_sa+=(int)diffticks;
                                if (diffticks<0)
                                        diffticks=-diffticks;
                                diffticks-= (s->vj_sd >> 2);
                                s->vj_sd+=(int)diffticks;
                                if (s->vj_sa > MAXVJSA) s->vj_sa=MAXVJSA;
                                if (s->vj_sd > MAXVJSD) s->vj_sd=MAXVJSD;
                        }
                        /* Only compute rtt hence rrto after success */
                        s->rto =1 + ( (s->vj_sa>>2)+(s->vj_sd) ) >>1;
                }
                s->karn_count=0;
                if (s->wwindow++ >= s->cwindow) {
                        s->cwindow++;
                        s->wwindow=0;
                }
        }
/* Kludgey bit to optimize...hence defn of var here */
        {
                long scheduleto;
                scheduleto=set_ttimeout(s->rto+2);
                if (s->rtt_time < scheduleto ) s->rtt_time = scheduleto;
        }
   }
        
                        


        switch (s->state) {

        case tcp_stateLISTEN:
                if (flags & tcp_flagSYN) {
/* 
 * Should really make a copy of the socket here, so that we can
 * have multiple connections on the same port...if the socket
 * has a data handler then remove the recvbuff, not needed...
 *
 * Send RST if not enough mem for Socket?
 *
 * Change again! If s->hisaddr == ip->source then the user
 * has set up a listening socket so we have to stick with the
 * (and don't allocate a new one)
 */
#ifdef NETSTAT
			++netstats.tcp_connreqs;
#endif
			if (s->hisaddr == ip->source ) {
				n=s;
			} else {
				if ( (n=NewBuffSocket(flags) ) == 0 ) return;
			}
#ifdef NETSTAT
			++netstats.tcp_connaccs;
#endif
                        n->acknum=tp->seqnum+1;
                        n->hisport = tp->srcport;
                        n->hisaddr = ip->source;
                        n->myaddr  = ip->dest;
			n->tos = ip->tos; /* Pick up TOS */
                        n->myport = s->myport;
                        n->flags = tcp_flagSYN | tcp_flagACK;
                        n->datahandler = s->datahandler;
			set_sockvj(n);
                        TCPSEND(n);
                        n->state = tcp_stateSYNREC;
                        n->unhappy = TRUE;
                        SetTIMEOUTtime(n);
                } else {
                        tcp_rst(ip,tp);
                        /* Send rst */
                }
                return;         /* break; */

        case tcp_stateSYNSENT:
                if ( flags & tcp_flagSYN ) {

                        s->flags = tcp_flagACK;
                        SetTIMEOUTtime(s);
                        if ( ( flags & tcp_flagACK ) && (tp->acknum == (s->seqnum + 1 ) ) ) {
                                /* Open! */
#ifdef NETSTAT
				++netstats.tcp_estab;
#endif
                                s->state = tcp_stateESTAB;
                                ++s->seqnum;
                                s->acknum= tp->seqnum+1;
/* Signal that conn is open, then feed data thru, not that should be
 * any of course!
 */
                                if (s->datahandler)
					HCALL(0,handler_OPEN,s);
                                tcp_processdata(s,tp,len);
                                s->unhappy = FALSE;
                                TCPSEND(s);
                        } else {
                                s->acknum++;
                                s->state = tcp_stateSYNREC;
                        }
                }
                break;

        case tcp_stateSYNREC:
                if ( flags & tcp_flagSYN ) {
                        s->flags = tcp_flagSYN | tcp_flagACK;
                        s->unhappy = TRUE;
                        TCPSEND(s);
                        SetTIMEOUTtime(s);
                        return;
                        /* Retransmit of original syn */
                }
                if ( ( flags & tcp_flagACK ) && tp->acknum == (s->seqnum + 1)  ) {
			/* This is quite kludgey but it works... */
    			s->window=min(htons(tp->window),tcp_MAXDATA*2);
                        s->flags=tcp_flagACK;
                        ++s->seqnum;
#ifdef NETSTAT
			++netstats.tcp_estab;
#endif
                        s->state = tcp_stateESTAB;
                        SetTIMEOUTtime(s);
                        s->unhappy = FALSE;
                        if (s->datahandler) 
                                HCALL(0,handler_OPEN,s);
                        TCPSEND(s);   /* naughty */
                        return;
                        /* Synackc received - established */
                }
                break;


        case tcp_stateESTAB:
        case tcp_stateESTCL:
        case tcp_stateCLOSEWT:

                /* Handle lost syn */
                if ((flags & tcp_flagSYN) && (flags & tcp_flagACK) ) {
                        TCPSEND(s);
                        return;
                }

                if ( (flags & tcp_flagACK) == 0) return; /* must ack sommat */

                if ( flags & tcp_flagSYN ) {
                        tcp_rst(ip,tp);
                        /* reset */
                        return;
                }

                diff = tp->acknum - s->seqnum;
                if (diff >  0 && diff <= s->sendoffs ) {
                        s->sendoffs -=diff;
                        s->unacked  -= diff;
                        if (s->sendoffs < 0 ) s->sendoffs=0;
                        Move(diff+s->sendbuff,s->sendbuff,s->sendoffs);
                        s->seqnum+=diff;
                } else {
/* handler condfused so set unacked to 0 */
                        s->unacked = 0;
                }
                if (s->unacked < 0 ) s->unacked =0;

                s->flags = tcp_flagACK;

                tcp_processdata(s,tp,len);


                if (( flags & tcp_flagFIN) && (s->state != tcp_stateCLOSEWT ) /* && ( s->acknum == tp->seqnum) && s->sendoffs==0 */) {
                        s->acknum++;
                        s->state=tcp_stateCLOSEWT;
                        TCPSEND(s);
                        s->state=tcp_stateLASTACK;
                        s->flags |= tcp_flagFIN;
                        s->unhappy=TRUE;
                }
                if ( (diff > 0 && s->sendoffs) || len > 0 ) {
                        /* Update window...how urgent? */
                        if  ( (diff >0 && s->sendoffs) || (len > 0) ) 
                                TCPSEND(s);
                        else
                                tcp_sendsoon(s);
                }
                if ( s->state == tcp_stateESTCL ) tcp_close(s); 

                return;

                case tcp_stateFINWT1:
/* They have not necessarily read all the data yet - supply as
 * requested 
 */

                diff = tp->acknum - s->seqnum;

                if (diff >  0 && diff <= s->sendoffs ) {
                        s->sendoffs -=diff;
                        s->unacked  -= diff;
                        if (s->sendoffs < 0 ) s->sendoffs=0;
                        Move(diff+s->sendbuff,s->sendbuff,s->sendoffs);
                        s->seqnum+=diff;
                        if (diff == 0 && s->unacked < 0 ) s->unacked =0;
                }

/*
 * They might still be transmitting, we must read it!
 */
                tcp_processdata(s,tp,len);

                /* Check if other tcp has acked all sent data and is
                 * ready to change states 
                 */

                if ( (flags & (tcp_flagFIN|tcp_flagACK) ) == (tcp_flagFIN|tcp_flagACK) ) {
#ifdef ARCHAIC
                        /* Trying to do simultaenous close */
                        if ( (tp->acknum >= (s->seqnum + 1) ) && (tp->seqnum == s->acknum )) {
                                s->seqnum++;
                                s->acknum++;
                                s->flags = tcp_flagACK;
                                TCPSEND(s);
                                s->unhappy = FALSE;
                                SetTIMEOUTtime(s);
                                s->state = tcp_stateCLOSED;
                                if (s->datahandler)
                                       HCALL(0,handler_CLOSED,s);
                        }
#else
                        if (  tp->seqnum == s->acknum ) {
                                s->acknum++;    /* ack their FIN */
                                if ( tp->acknum >= (s->seqnum+1) ) {
                                /* Not simul close (they ACK'd our FIN*/
                                        s->seqnum++;
                                        s->state=tcp_stateTIMEWT;
                                } else {
                                /* simul close */
                                        s->state=tcp_stateCLOSING;
                                }
                                s->flags=tcp_flagACK;
                                TCPSEND(s);
                                s->unhappy=FALSE;
                                if ( s->state == tcp_stateTIMEWT) SetLONGtimeout(s);
                                else SetTIMEOUTtime(s);
                        }
#endif
                } else if (flags & tcp_flagACK ) {
                        /* Other side is legitamately acking us */
                        if ( (tp->acknum == (s->seqnum+1)) && (tp->seqnum == s->acknum) && (s->sendoffs == 0)) {
                                s->seqnum++;
                      //          s->acknum++;
                                s->state = tcp_stateFINWT2;
                                SetTIMEOUTtime(s);
                                s->unhappy = FALSE;
                        }
                }
                break;

        case tcp_stateFINWT2:

                /* They may be still transmitting data, must read it */
                tcp_processdata(s,tp,len);

                if ( (flags & (tcp_flagACK | tcp_flagFIN) ) == (tcp_flagACK | tcp_flagFIN)) {
                        if ( (tp->acknum == s->seqnum ) && (tp->seqnum == s->acknum)) {
                                s->flags=tcp_flagACK;
                                s->acknum++;
                                TCPSEND(s);
                                s->unhappy=FALSE;
#ifdef ARCHAIC
                                SetTIMEOUTtime(s);
                                s->state = tcp_stateCLOSED;
#else
                                s->state=tcp_stateTIMEWT;
                                SetLONGtimeout(s);
#endif
                                if (s->datahandler)
                                        HCALL(0,handler_CLOSED,s);
                                return;
                        }
                }
                break;

        case tcp_stateCLOSING:
                if ((flags & (tcp_flagACK| tcp_flagFIN )) == tcp_flagACK ) {
			/* 8/12/99 seqnum+1 seqnum++ change */
                        if ( (tp->acknum == (s->seqnum+1)) &&
                             (tp->seqnum == s->acknum) ) {
                                s->seqnum++;
                                s->state = tcp_stateTIMEWT;
                                SetTIMEOUTtime(s);
                                s->unhappy = FALSE;
                        }
                }
                break;


        case tcp_stateLASTACK:
                if ( flags & tcp_flagFIN ) {
                        /* They lost our two packets, back up */
                        s->flags = tcp_flagACK | tcp_flagFIN;
                        TCPSEND(s);
                        SetTIMEOUTtime(s);
                        s->unhappy = FALSE;
                        return;
                } else {
                        if ( tp->acknum == (s->seqnum+1)  && (tp->seqnum == s->acknum )   ) {
                                if (s->datahandler)
                                        HCALL(0,handler_CLOSED,s);
                                s->state=tcp_stateCLOSED; /*no 2msl need*/
                                s->unhappy = FALSE;
                                return;
                        }
                }
                break;

        case tcp_stateTIMEWT:
                if ( flags & (tcp_flagACK| tcp_flagFIN ) == (tcp_flagACK| tcp_flagFIN) ) {
                        /* he needs an ack */
                        s->flags=tcp_flagACK;
                        TCPSEND(s);
                        s->unhappy=FALSE;
                        if (s->datahandler)
                                HCALL(0,handler_CLOSED,s);
                        s->state = tcp_stateCLOSED;
                }
                break;

        }
        if (s->unhappy) tcp_sendsoon(s);
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

tcp_processdata(s, tp, len)
    TCPSOCKET *s;
    struct tcp_header *tp;
    WORD len;
{
    WORD *options,numoptions;
    signed int diff;
    WORD x;
    BYTE *dp;

/* This is quite kludgey but it works... */
    s->window=min(htons(tp->window),tcp_MAXDATA*2);

    diff = s->acknum - tp->seqnum;
    if ( tp->flags & tcp_flagSYN ) --diff;
    dp = tp;
    x = tcp_GetDataOffset(tp); 
    dp += x;
    len -= x;

#ifdef TCPOPTIONS
/* Process TCP options */

	if ( (numoptions =x - sizeof (struct tcp_header)) ) {
		options=(BYTE *)tp+sizeof(struct tcp_header);
		while (numoptions--) {
			switch (*options++) {
				case 0: /* End of options */
					numoptions=0;
				case 1: /* NOP */
					break;
				case 2: /* mss */
					if (*options == 4 ) {
						opt_temp=htons(options[1]);
						if (opt_temp < s->mss)
   						  s->mss=opt_temp)
					}
				default:
					numoptions-=(*options-1);
					options+=(*options-1);
			}
		}
	}
#endif


    if ( diff >= 0 ) {
        dp += diff;
        len -= diff;
#ifdef NETSTAT
	netstats.tcp_recvdlen+=len;
#endif
        if (s->datahandler) {
                s->acknum +=HCALL(dp, len,s);
        } else {
		/* No datahandler installed - i.e. user app, dump to a buffer
		 * taking care not to overwrite what is already there. We only
		 * ACK what we've copied
		 */
                if ( len > (x=s->recvsize - s->recvoffs) )
                        len=x;
                if ( len ) {
                         s->acknum+=len;
                         Move(dp,s->recvbuff+s->recvoffs,len);
                         s->recvoffs+=len;
                 }
        }
    }
    s->unhappy = ((s->sendoffs) ? TRUE : FALSE);
    if (diff == 0 & s->unacked && chk_timeout(s->rtt_lasttran) )
        s->unacked=0;
    else
        tcp_sendsoon(s);
    SetTIMEOUTtime(s);
}

/*
 * Format and send an outgoing segment
 */
#ifdef DEBUGTCP
tcp_send(s,line)
    TCPSOCKET *s;
    int line;
#else
tcp_send(s)
    TCPSOCKET *s;
#endif
{
    int senddata,startdata,sendtotdata,sendtotlen;
    int i;
    BYTE *dp;
    struct pktdef {
        struct ip_header ip;
        struct tcp_header tcp;
        WORD maxsegopt;
        WORD maxsegopt2;
    } *pkt;


#ifdef SIMPLE
        if (s->karn_count != 2 || s->cwindow==1 ) {
                /* FIXME please!! TCP options! */
                if ( (senddata=(s->sendoffs-s->unacked )) < 0 ) senddata=0;
                startdata=s->unacked;
        } else {
                senddata=s->sendoffs;
                startdata=0;
        }
#endif


	if (s->karn_count != 2 ) {
		sendtotdata=s->sendoffs-s->unacked;
		sendtotdata=min(s->sendoffs,s->window) - s->unacked;
		startdata=s->unacked;
	} else {
		sendtotdata = (s->sendoffs >=s->window) ? s->window : s->sendoffs;
		startdata=0;
	}
	if	(sendtotdata<0) sendtotdata=0;


/*
 *  For non SYM packets this allocates an extra 4 bytes..not really
 *  worth fixing IMO
 */
	sendtotlen = 0;
	for (i= 0; i< s->cwindow; i++ ) {
		senddata = min( sendtotdata,s->mss);


		if ((pkt = AllocatePkt(sizeof (struct pktdef) + senddata)) == NULL ) return;

#ifdef DEBUG  
        	printf("Sending data of size %d flags %d st %d line %d \n",senddata,s->flags,s->state,line);

#endif
    		dp = &pkt->maxsegopt;

    		pkt->ip.length = sizeof(struct ip_header) + sizeof(struct tcp_header);

    		/* tcp header */
    		pkt->tcp.srcport = s->myport;
    		pkt->tcp.dstport = s->hisport;
    		pkt->tcp.seqnum = htonl(s->seqnum);
    		pkt->tcp.acknum = htonl(s->acknum);
    		pkt->tcp.window = htons(512);
    		pkt->tcp.flags  = s->flags;
    		pkt->tcp.cksum = 0;
    		pkt->tcp.urgptr = 0;
/*
 * Syn can't carry data
 */
    		if ( s->flags&tcp_flagSYN ) {
        		pkt->tcp.offset=6*16;
        		pkt->ip.length += 4;
        		pkt->maxsegopt = htons(512+4);
        		pkt->maxsegopt2 = htons(s->mss);
    		} else {
/* We can have data! */
        		pkt->tcp.offset=5*16;
        		if (senddata ) {
                		pkt->ip.length += senddata;
                		Move(s->sendbuff+startdata, dp, senddata);
        		}
    		}

    		FillHeader((struct ip_header *)pkt,prot_TCP,s->ttl);
    		pkt->ip.tos = s->tos;
    		pkt->ip.source = s->myaddr;
    		pkt->ip.dest = s->hisaddr;
/* Reverse the length to big endian */
    		pkt->ip.length=htons(pkt->ip.length);
#ifdef NETSTAT
    		++netstats.tcp_sent;
    		netstats.tcp_sentlen+=senddata;
#endif

    		IPHeaderCheck(&pkt->ip);

    		pkt->tcp.cksum =TCPCheckSum(&pkt->ip, prot_TCP, (ntohs(pkt->ip.length) - sizeof(struct ip_header) )  );

    		SendPacket(pkt,htons(pkt->ip.length));
		sendtotlen+=senddata;
		sendtotdata-=senddata;
		startdata+=senddata;
		if (sendtotdata <= 0 ) break;
	}

    s->unacked=startdata;
        s->vj_last=0;
        if (s->karn_count == 2) {
                if (s->rto) s->rto = (s->rto *3)/2;
                else s->rto=4;
        } else {
                /* vj last non zero,immediate response */
                if (s->unhappy || s->sendoffs)
                        s->vj_last=set_ttimeout(0);
                s->karn_count=0;
        }
        s->rtt_time=set_ttimeout(s->rto +2);
        if (sendtotlen > 0) s->rtt_lasttran=s->rtt_time+s->rto;
}

/*
 *      Send a rst packet
 */

tcp_rst(hisip,oldtp)
        struct ip_header *hisip;
        struct tcp_header *oldtp;
{
        BYTE oldflags;
        struct pktdef2 {
                struct ip_header ip;
                struct tcp_header tcp;
        } *pkt;

        oldflags=oldtp->flags;
        if (oldflags & tcp_flagRST) return;

        if (oldflags & tcp_flagACK ) {
                oldtp->seqnum = oldtp->acknum;
                oldtp->acknum = 0;
        } else {
            oldtp->acknum = oldtp->seqnum + ( htons(hisip->length) - ((hisip->version&15)*4) );
            oldtp->seqnum = 0;
        }
            


        if ( (pkt = AllocatePkt(sizeof(struct pktdef2)) ) == NULL ) return;


    /* tcp header */
        pkt->tcp.srcport = oldtp->dstport;
        pkt->tcp.dstport = oldtp->srcport;
        pkt->tcp.seqnum = htonl(oldtp->seqnum);
        pkt->tcp.acknum = htonl(oldtp->acknum);
        pkt->tcp.window = 0;
        pkt->tcp.offset = 5*16;
        pkt->tcp.flags = tcp_flagRST;
        pkt->tcp.cksum = 0;
        pkt->tcp.urgptr = 0;

    /* internet header - just go with default TTL/tos*/               
	FillHeader((struct ip_header *)pkt,prot_TCP,255);
        pkt->ip.source = hisip->dest;
        pkt->ip.dest = hisip->source;
        pkt->ip.length=htons(sizeof(struct pktdef2));

        IPHeaderCheck(&pkt->ip);

/*
 * Do the TCP Checksum, length is always going to be 20 (TCP header)
 */
        pkt->tcp.cksum = TCPCheckSum(&pkt->ip, prot_TCP, sizeof(struct tcp_header)  );
#ifdef NETSTAT
	++netstats.tcp_rstsent;
#endif

        SendPacket(pkt,sizeof(struct pktdef2));

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

TCPSOCKET *NewBuffSocket(flag)
	BYTE	flag;
{
        void *ptr;
        TCPSOCKET *s;
        
	/* Find a new socket, return NULL if we have no mem */
        if ( flag &&  (s=NewTCPSocket() ) == 0 ) return (0);  
/*
 * Now, set up some buffers for us..all variables are initialised by
 * NewTCPSocket();
 *
 * Rearranged order of mallocs 10/1/2000 so we can get a larger
 * free chunk for daemons (less fragging)
 */
        if ( (ptr=malloc(BUF_SIZE)) == 0 ) { free(s); return(0); }
        s->sendbuff=ptr;
        if ( (ptr=malloc(BUF_SIZE)) == 0 ) { free(s->sendbuff); free(s); return(0); }
        s->recvbuff=ptr;
        s->recvsize=s->sendsize=BUF_SIZE;
        return(s);
}

TCPSOCKET *NewTCPSocket()
{
        TCPSOCKET *s;
        if (s=newsocket() == 0 ) return 0;
        s->seqnum=GetSeqNum();
        s->ip_type=prot_TCP;
	s->ttl=255;	/* DEFAULT TCP ttl */
	s->tos=0;	/* DEFAULT TCP tos */
        return(s);
}

/*
 *      Get memory for a new socket, use calloc to clear the memory
 *      Fed in the size so works for any
 */

void *newsocket()
{
        TCPSOCKET *s;
        if ( ( s=calloc(sizeof(TCPSOCKET),1) ) == 0 ) return 0;
/* Put this socket into the queue */
        s->next=sysdata.tcpfirst;
        sysdata.tcpfirst=s;
        return(s);
}



/*
 *      Maintainance Routines - assembler for speed
 */

/* 
 * Calculate Checksum for TCP packet - should also work for UDP
 * Returns big endian
 */


TCPCheckSum(buf,protocol,length)
        BYTE *buf;
        BYTE    protocol;
        WORD    length;
{
#pragma asm
        ld      ix,2    ;use ix as frame pointer
        add     ix,sp
        ld      c,(ix+0)        ;length
        ld      b,(ix+1)
        ld      e,(ix+2)        ;protocol
        ld      l,(ix+4)        ;buffer
        ld      h,(ix+5)        
        push    bc
        push    hl
        call    _PsHeaderSum
        ld      d,b
        ld      e,c
        pop     hl
;Get header length from IP packet - this can be used for sending
;and receiving!
        ld      a,(hl)
        and     @00001111
        rlca
        rlca
        ld      c,a
        ld      b,0
        add     hl,bc
        pop     bc
        call    _TCPCsum
        ld      l,b
        ld      h,c
#endasm
}



/* Find the offset of TCP data within TCP header */

int __FASTCALL__ tcp_GetDataOffset(struct tcp_header *tp)
{
#pragma asm
        ld      bc,12   ;enters with hl=tp
        add     hl,bc   ;to offset variable
        ld      a,(hl)
        rrca
        rrca
        and     @00111100
        ld      l,a
        ld      h,0
#pragma endasm
}

/*
 *
 *      Move data from hither to tither!
 */

Move(src,dest,diff)
        BYTE *src,*dest;
        WORD    diff;
{
#pragma asm
; Lets use ix as a frame pointer!
        ld      ix,2
        add     ix,sp
        ld      c,(ix+0)
        ld      b,(ix+1)
        ld      a,c
        or      b
        ret     z
        ld      e,(ix+2)
        ld      d,(ix+3)
        ld      l,(ix+4)
        ld      h,(ix+5)
        ldir
#pragma endasm
}

min(b,a)
	int b,a;
{
	if (a < b ) return a;
	return b;
}
#if 0
max(b,a)
	int b,a;
{
	if (a > b ) return a;
	return b;
}
#endif



     
