/*
 *	All The Application Gunk..
 *	djm 8/1/2000
 *	Quick and simple APP not proper API one
 */



#include <stdlib.h>
#include <stdio.h>
#include <z88.h>

extern void pack_ayt();
extern void pack_bye();
extern void pack_dat();
extern void tftp_daemon();
extern void recv_daemon();
extern void xmit_daemon();

int process;

void main()
{
	process=0;

	printf("Please select a sub-application from the menu\n");
	if (QueryPackage(LIB_TCP,0,0) == NULL) {
		printf("Couldn't open ZSock Library - Is it installed?\n");
		sleep(3);
		exit(0);
	}
	while ( 1 ) {
		fgetc(stdin);
	}
}


void __APPFUNC__ handlecmds(unsigned char cmd)
{
	if (process) {
		putchar(7);
		return;
	}
	if (cmd >= 0x84 ) {
	  switch (cmd) {
		case 0x84:
			if (QueryPackage(LIB_TFTPD,0,0) == NULL) {
				printf("Couldn't open tftpd package\n");
			} else {
				printf("Tftpd socket is now registered\n");
			}
			return;
		case 0x85:
			pack_bye();
			printf("Tftpd socket deregistered\n");
			return;
	  }
	} else {
		process = cmd;
		switch (cmd) {
			case 0x81:
				chat();
				break;
			case 0x82:
				telnet();
				break;
			case 0x83:
				exit(0);
		}
		process=0;
		printf("Please select a sub-application from the menu\n");
		nameapp("");
	}
}
        
#include <dor.h>

#define HELP1   "A simple VT52 telnet client using the ZSock shared"
#define HELP2	"library system, includes a chat program to allow"
#define HELP3	"easy connection to an ISP. A tftpd is also included"
#define HELP4	"Part of the ZSock System"
#define HELP5   ""
#define HELP6   "v1.2 (c) 28/2/2001 D.J.Morris"

#define APP_INFO "Telnet"
#define APP_KEY  'T'
#define APP_NAME "Telnet"

/* Turn caps lock off */
#define APP_TYPE2 0

#define TOPIC1       "Programs"
#define TOPIC1ATTR   TP_Help
#define TOPIC1HELP1  "Use this menu to decide between sub-programs"


#define TOPIC1_1	"Telnet"
#define	TOPIC1_1KEY	""
#define TOPIC1_1CODE	$82
#define TOPIC1_1ATTR	MN_Help
#define TOPIC1_1HELP1	"Use this to connect to a remote host via telnet"

#define TOPIC1_2	"Modem Chat"
#define TOPIC1_2KEY	""
#define TOPIC1_2CODE	$81
#define TOPIC1_2ATTR	MN_Help
#define TOPIC1_2HELP1	"Use this to connect to your ISP"

#define TOPIC1_3	"Quit"
#define TOPIC1_3KEY	""
#define TOPIC1_3CODE	$83
#define TOPIC1_3ATTR	MN_Help
#define TOPIC1_3HELP1	"Quit the application"

#define TOPIC2	"Tftpd"
#define TOPIC2ATTR TP_Help
#define TOPIC2HELP1 "Commands to (de)register the Trivial FTP daemon"

#define TOPIC2_1  	"Register"
#define TOPIC2_1KEY	""
#define TOPIC2_1CODE	$84
#define TOPIC2_1ATTR	MN_Help
#define TOPIC2_1HELP1	"This registers the tftpd socket for file transfer"

#define TOPIC2_2  	"DeRegister"
#define TOPIC2_2KEY	""
#define TOPIC2_2CODE	$85
#define TOPIC2_2ATTR	MN_Help
#define TOPIC2_2HELP1	"This deregisters the tftpd socket and aborts"
#define TOPIC2_2HELP2	"all file transfers in progress"


/* Package stuff */
#define PACKAGE_ID 	$18
#define MAX_CALL_NUM	$10
#define PACK_NAME	"tftpd"
#define PACK_BOOT	0
#define PACK_VERSION	$0100

#define MAKE_PACKAGE

#include <package.h>


package_str tf_pstr[] = {
	{tftp_daemon},
	{recv_daemon},
	{xmit_daemon}
};

#include <application.h>
/* THE END! */





