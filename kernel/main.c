/*
 *	Application of ZSock
 *
 *	Contains Basic stuff such as app level stuff
 *
 *	All this utilities the API, functions which
 *	are called from this don't necessarily...
 *
 *	djm 18/2/2000
 */


#include <stdio.h>
#include <stdlib.h>
#include <z88.h>
#include <net/misc.h>
#include "config.h"

int	process;

extern char bigwindow[];
void RedrawScreen();

int main()
{

	/* Open ZSock library */
	printf(bigwindow);
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
		BUSYLOOP();
		getk();
	}
}


void __APPFUNC__ handlecmds(unsigned char cmd)
{


	if (cmd >= 0x87 ) {
/* On/offline options */
		switch (cmd) {
			case 0x87:
				_DeviceOffline(HANGUP);
				printf("Hungup");
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
					Netstat();
					break;
				case 0x83:
					UserConfig();
					break;
				case 0x84:
					exit(0);
					break;
				case 0x85:
					report();
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
	printf(bigwindow);
}


#pragma asm
._bigwindow
	defb	1,'7','#','1',32,32,32+94,32+8,128
	defb	1,'2','C','1',1,'S',1,'C',0
#pragma endasm