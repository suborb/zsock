/*
 *
 *	Setup Routine for ZSock Package
 *
 *	Mostly the same as the old main() but all printing
 *	is removed
 *
 *	djm 12/2/2000
 *
 */

// #define FDSTDIO 1

#include "zsock.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>



extern struct pktdrive z88slip;


/*
 * Because the Memory Handling of the Small C crt0 is a bit bad ATM
 * we're using a static variable as our heap - it whacks the size of
 * the file up, but it means it is kept safe over preemption - and we
 * can't use an external pointer extern char k(address) because we
 * don't know how large the program is going to be - this really is
 * safer all round..
 */

#define HPSIZE 15000
extern int process;
/* This gubbins must be in first file.. */
#pragma -zorg=32768
#pragma -reqpag=1
#pragma -defvars=16384

/* 
 * Structure for system data (put first so sits @16384
 */

struct sys sysdata;

#ifdef NETSTAT
struct sysstat_s netstats;
#endif

HEAPSIZE(HPSIZE)

static char version[]="$VER:ZSock v2.2 (c) 2001 D.Morris\n\r";

extern BYTE *allocatepkt();
extern PktDrvRegister();
extern TCPInit();
extern tcp_retransmit();
extern void PktRcvIP(void *,WORD);

extern ipaddr_t defaultip;

extern GetFree();
extern telnet();

#pragma asm
._defaultip       defb 10,0,0,88
#pragma endasm


/*
 * Where out device driver is
 */

struct pktdrive *device;


/*
 * Function prototypes
 */

void main();
int loop();
int StdLoop1();
int StackInit();
void MemExit(void);
void MyExit(int);
void ApplicationQuit();
int CheckDriver();

/*
 *	Code called on the interrupt
 *
 *	Shudder...
 */

Interrupt()
{
	WORD	bind;
	BYTE	*pkt;
	WORD	value;
	bind=PageDevIn();
/* Read in some bytes and handle the packet */
	if (value=device->readfn(&pkt) ) {
		PktRcvIP(pkt,value);    
	}
/* Send out some bytes... */
        if (value=(int)device->sendfn()) FreePacket((void *)value);
	PageDevOut(bind);
/*
 *      Handling TCP timeouts here, this is unbelievable kludgy, but
 *      there you go!
 */
        --sysdata.counter;
        if (sysdata.counter==0) {
/* 
 *      Counter has expired, call retransmit
 */
                tcp_retransmit();
                CheckUDPtimeouts();
                sysdata.counter=50;
        }
/* Get the local stuff */
	GetLocal();
}


/*
 *      Set up the stack, form the malloc pool
 *	Read in config (depending on flag)
 *	Start up the servics
 */

int StackInit(int flag)
{
    char    name[18];
    ipaddr_t addr;
    int	i;
    heapinit(HPSIZE);
    sysdata.usericmp=0;
    sysdata.mss = 512;
    if (flag) {
	ReadDomConfig();	/* Read in domain info */
	sysdata.myip=defaultip;	/* Default 10.0.0.88 */

        if (gethostname_i(name,sizeof(name)) ==0  ) {
                if (gethostaddr_i(name,sizeof(name)) == 0 ) {
                        addr=inet_addr_i(name);
                        if (addr) 
                                sysdata.myip=addr;
                }
        } 
/* Now try to find a device..read from config file */
	ReadDevConfig();
    }
    sysdata.counter=50;
    sysdata.debug=0;
    if (AttachDriver(device) == NULL ) {
		return(1);
    }
    TCPInit();
    UDPInit();
    return(0);
}

/*
 * Attach a packet driver to the system
 *
 * Returns true/false on success
 */

int AttachDriver(struct pktdrive *ptr)
{
	int	bind;
	bind=PageDevIn();
	if (CheckDriver(ptr)==NULL) {
		PageDevOut(bind);
		return 0;
	}
/* Call initialisation routine */
        sysdata.overhead = ptr->initfunc();
/* Matched the magic, so it must be okay! */
/* Now get an input buffer */
    	sysdata.pktin=(ptr->packetfn)();
	if (sysdata.pktin == 0 ) {
		return 0;
	}
/* Set up our loopback queues */
	InitLocal();
	PageDevOut(bind);
        return(1);
}

/*
 *	Read in a device driver to DRIVER_ADDR (8192)
 *
 *	Returns length read or 0 for no file etc..
 */

int LoadDriver(char *name)
{
	int	bind;
	int	len;
	int	fd;

	if	((fd=open(name,O_RDONLY,0)) == EOF) return NULL;
/* No fstat in z88 lib yet, so read as much as we can */
	bind=PageDevIn();
	len=read(fd,DRIVER_ADDR,8192);
	close(fd);
	PageDevOut(bind);
	return(len);
}

/*
 *	Check To See If Driver Is OK
 *
 *	Returns true/false
 */

int CheckDriver(struct pktdrive *ptr)
{
        if (strcmp(ptr->magic,"ZS0PKTDRV") ) return(0);
	return (1);
}

