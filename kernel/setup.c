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
 * $Id: setup.c,v 1.10 2002-10-08 19:39:57 dom Exp $
 *
 */




#define NETSTAT_TXT
#include "zsock.h"
#include <stdlib.h>   /* We need the HEAP macros */



/* We use an array as a MALLOC heap */
#define HPSIZE 15000
extern int process;

#ifdef __Z88__
#pragma -zorg=32768
#pragma -reqpag=1
#pragma -defvars=16384
#pragma output userheapvar=1
#endif



struct sys sysdata;

#ifdef NETSTAT
struct sysstat_s netstats;
#endif

#ifdef SCCZ80
HEAPSIZE(HPSIZE)
#endif

char version[]="$VER:ZSock v2.31 (c) 2002 D.Morris\n\r";


static ipaddr_t       defaultip = IP_ADDR(192,168,155,88);


void Interrupt()
{
    u16_t	bind;
    void       *pkt;
    int         value;
#ifdef __CPM__
    static int  counter = 0;
#endif
    

    bind = PageDevIn();
    /* Read in some bytes and handle the packet */
    if (value = device->readfn(&pkt) ) {
	PktRcvIP(pkt,value);    
    }
    /* Send out some bytes... */
    if ( pkt = device->sendfn() ) 
	pkt_free(pkt);
    PageDevOut(bind);
    /* Kludgey TCP timeout */
#ifdef __CPM__
    if ( ++counter == 250 ) {
	counter = 0;
	tcp_retransmit();
	udp_check_timeouts();
    }
#else
    if ( chk_timeout(sysdata.timeout ) ) {
	tcp_retransmit();
	udp_check_timeouts();
	sysdata.timeout = set_timeout(300);  /* Every 3 seconds */
    }
#endif
    loopback_recv();
}



int StackInit(int readconfig)
{
    char    name[18];
    ipaddr_t addr;
    int	i;

#ifdef SCCZ80
    heapinit(HPSIZE);
#endif
    sysdata.usericmp = 0;
    sysdata.mss = 512;
#ifdef __Z88__
    if ( readconfig ) {
	config_dns();            /* Read in DNS information */
	sysdata.myip=defaultip;	
        if ( config_hostname(name,sizeof(name)) == 0  ) {
                if ( config_hostaddr(name,sizeof(name)) == 0 ) {
                        addr = inet_addr_i(name);
                        if (addr) 
                                sysdata.myip = addr;
                }
        } 
	config_device();        /* Find device file info */

    }
#else
    sysdata.myip=defaultip;	
    sysdata.overhead = 16;
    device = &z88ppp;
#endif
    sysdata.timeout = 0;
    sysdata.debug   = 0;
    if ( device_attach(device) == FALSE )
	return (1);  
    loopback_init();   /* Setup loopback interface */
    tcp_init();        /* Initialise TCP layer */
    udp_init();        /* Initialise UDP layer */
    return(0);
}




/*
 *	User config..to enter in hostname and nameserver
 */

UserConfig()
{
	char	buffer[40];
	ipaddr_t addr;
	int	j,i;
	printf("IP addr of the z88: ");
	fgets_cons(buffer,19);
	if ( (addr=inet_addr_i(buffer) ) ) {
		sysdata.myip=addr;
	} else {
		printf("\nConfig aborted..");
		return;
	}

	printf("\nDefault domainname: ");
	fgets_cons(sysdata.domainname,MAXDOMSIZ-1);
	j=0;
	for (i=0; i<MAXNAMESERV;i++) {
		printf("\nIP addr of nameserver #%d: ",i+1);
		fgets_cons(buffer,19);
		if ( (addr=inet_addr_i(buffer)) ) {
			sysdata.nameservers[i]=addr;
			++j;
		} else break;
	}
	putchar('\n');
#ifdef __Z88__
	printf("\nFilename of network device driver: ");
	fgets_cons(buffer,39);
	putchar('\n');
	if (strlen(buffer) ) {
	    device = device_insert(buffer);
	}
#endif
	StackInit(FALSE);
	if (j == 0) 
	    return;

	sysdata.numnameserv = j;
}

do_netstat()
{
    static char *st[]=
    { "NONE", "LISTEN","SYNSENT","SYNREC","ESTAB","ESTABCL",
      "FINWT1","FINWT2","CLOSEWT","CLOSING","LASTACK",
      "TIMEWT","CLOSEMSL","CLOSED" };  
    int	time;
    TCPSOCKET *s;

#ifdef SCCZ80
    printf("Free: %d Largest: %d debug %d Conns are:\n",getfree(),getlarge(),sysdata.debug);
#endif

    printf("Lport\tDport\tFlags\tTimeout\tDataq\tState\n");
    for ( s = sysdata.tcpfirst ; s ; s = s->next ) { 
	time=s->timeout-current_time();
	if (time<0) time=0;
	printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\n",htons(s->myport),htons(s->hisport),s->flags,time,s->recvoffs,s->sendoffs,st[s->state]);
    }
}


figures()
{
    int	*ap = (int *)&netstats;
    unsigned char	*text;
    int	i;

    for (i=0;i<NETSTAT_NUM;++i) {
	text=netstat_txt[i];
	if (*text =='l') {
	    printf(text+1,*((long *)ap));
	    ++ap;
	} else {
	    printf(text,*ap);
	}
	putchar('\n');
	++ap;       	
	if (i && i%6 == 0 ) {
	    GETKEY();
	}
    }
}

