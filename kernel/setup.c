/*
 *
 *	Setup Routine for ZSock Package
 *
 *	Mostly the same as the old main() but all printing
 *	is removed
 *
 *	djm 12/2/2000
 *
 *      $Id: setup.c,v 1.4 2002-05-12 22:20:42 dom Exp $
 *
 */



#define NETSTAT_TXT
#include "zsock.h"
#include <stdlib.h>   /* We need the HEAP macros */



/* We use an array as a MALLOC heap */
#define HPSIZE 15000
extern int process;

#ifdef SCCZ80
#pragma -zorg=32768
#pragma -reqpag=1
#pragma -defvars=16384
#pragma output userheapvar=1
#endif



struct sys sysdata;

#ifdef NETSTAT
struct sysstat_s netstats;
#endif

HEAPSIZE(HPSIZE)

static char version[]="$VER:ZSock v2.3 (c) 2002 D.Morris\n\r";


static ipaddr_t       defaultip = IP_ADDR(192,168,155,88);


void Interrupt()
{
    u16_t	bind;
    void       *pkt;
    int         value;

    bind = PageDevIn();
    /* Read in some bytes and handle the packet */
    if (value = device->readfn(&pkt) ) {
	PktRcvIP(pkt,value);    
    }
    /* Send out some bytes... */
    if ( value = device->sendfn() ) 
	pkt_free((void *)value);
    PageDevOut(bind);
    /* Kludgey TCP timeout */
    if ( --sysdata.counter == 0) {  
	tcp_retransmit();
	udp_check_timeouts();
	sysdata.counter = 50;
    }
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
    sysdata.counter = 50;
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
	printf("\nFilename of network device driver: ");
	fgets_cons(buffer,39);
	putchar('\n');
	if (strlen(buffer) ) {
	    device = device_insert(buffer);
	}
	StackInit(FALSE);
	if (j == 0) 
	    return;

	sysdata.numnameserv = j;
}

do_netstat()
{
    static char *st[]=
    { "LISTEN","SYNSENT","SYNREC","ESTAB","ESTABCL",
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
    int	*ap=netstats;
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
	    getkey();
	}
    }
}

