/*
 *	Application routines that hack the kernel
 *	data structures
 *
 *	djm 18/2/2000
 */


#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <net/hton.h>

#define NETSTAT_TXT
#include "zsock.h"


extern struct pktdrive z88slip;


/* 
 * Structure for system data (put first so sits @16384 
 */

/*
 * Where out device driver is
 */

extern struct pktdrive *device;


/*
 * Function prototypes
 */

extern void StackInit();
extern int CheckDriver();

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
/* Hey, we caught sommat! */
		int	bind;
		i=LoadDriver(buffer);
		bind=PageDevIn();
		if ( i  && CheckDriver((void *)DRIVER_ADDR) ) {
			printf("Driver loaded - %d bytes\n",i);
			device=DRIVER_ADDR;
		} else {
			if ( i ) printf("Loaded, but not driver\n");
			else printf("Driver not found\n");
			device=z88slip;
		}
		PageDevOut(bind);
	}
	StackInit(FALSE);
	if (j==0) return;

	sysdata.numnameserv=j;
}

netstat()
{
        static char *st[]=
        { "LISTEN","SYNSENT","SYNREC","ESTAB","ESTABCL",
          "FINWT1","FINWT2","CLOSEWT","CLOSING","LASTACK",
          "TIMEWT","CLOSEMSL","CLOSED" };
                                
	int	time;
        TCPSOCKET *s;

        printf("Free: %d Largest: %d debug %d Conns are:\n",getfree(),getlarge(),sysdata.debug);

        printf("Lport Dport Flags Timeout Dataq State\n");
        for ( s = sysdata.tcpfirst ; s ; s = s->next ) 
        {
			time=s->timeout-current();
			if (time<0) time=0;
	                printf("%d  %d  %d  %d  %d  %d %s\n",htons(s->myport),htons(s->hisport),s->flags,time,s->recvoffs,s->sendoffs,st[s->state]);
        }
}

report()
{
	int	bind;
	bind=PageDevIn();
	printf("Report on device: %s\n",device->copymsg);
	printf("Queue is at: %d\n",device->packetfn());
	printf("Status is: %d\n",device->statusfn());
	PageDevOut(bind);
}

figures()
{
	int	*ap=netstats;
	unsigned char	*text;
	int	i;


	for (i=0;i<NETSTAT_NUM;++i) {
		text=netstat_txt[i];
		if (*text =='l') {
			printf(text+1,*(long *)ap);
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

