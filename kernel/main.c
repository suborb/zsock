/*
 *	Application of ZSock
 *
 *	Contains Basic stuff such as app level stuff
 *
 *	All this utilities the API, functions which
 *	are called from this don't necessarily...
 *
 *	djm 18/2/2000
 *
 *      $Id: main.c,v 1.5 2002-05-13 21:30:22 dom Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <z88.h>
#include <net/misc.h>



int	process;

static void   RedrawScreen();


int main()
{

	/* Open ZSock library */
	printf("\0017#1  \176(\200\0012C1\001S\001C");
	printf("Querying library\n");
	if (QueryPackage(LIB_TCP,0,0) == NULL ) {
		printf("Couldn't open library\n");
		sleep(3);
		exit(0);
	}
	user_pagein();
	process=0;
		
	printf("ZSock Waiting For Your Command\n");
	while(1) {
		BUSYINT();
		getk();
	}
}


void __APPFUNC__ handlecmds(int cmd)
{
    if (cmd >= 0x87 ) {
	/* On/offline options */
	switch (cmd) {
	case 0x87:
	    _DeviceOffline(HANGUP);
	    printf("Hungup\n");
	    break;
	case 0x88:
	    _DeviceOffline(NOHANGUP);
	    printf("Offline\n");
	    break;
	case 0x89:
	    _DeviceOnline();
	    printf("Turned device online\n");
	}
    } else {
	if (process) { 
	    putchar(7);	/* Beep */
	    return; 
	} else {
	    process=cmd;
	    switch (cmd) {
	    case 0x81:
		Ping();
		break;
	    case 0x82:
		do_netstat();
		break;
	    case 0x83:
		UserConfig();
		break;
	    case 0x84:
		exit(0);
		break;
	    case 0x85:
		device_report();
		break;
	    case 0x86:
		figures();
	    }
	    process=0;
	}
    }
}







/*
 *	Function to redraw screen after preemption
 */


void __APPFUNC__ RedrawScreen()
{
	user_pagein();			/* Page in Zsock seg 1 */
	printf("\0017#1  \176(\200\0012C1\001S\001C");
}



