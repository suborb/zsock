/*
 *	This Sets Up The Dors for app and Package
 *
 *	djm 12/2/2000
 *
 */

#include "zsock.h"

#ifndef Z88
        
#include <dor.h>

#define HELP1	"A demo of ZSock the Z88 TCP/IP Stack made with z88dk"
#define HELP2	"Read the documentation for configuration info"
#define HELP3	"This demo contains shared library facilities,"
#define HELP4	"echo/qotd/finger services and a ping client"
#define HELP5   ""
#define HELP6   "v2.3 (c) xx/5/2002 D.J.Morris"

#define APP_INFO "TCP Stack"
#define APP_KEY  'S'
#define APP_NAME "ZSock"

#define APP_TYPE AT_Bad | AT_Ones
/* Turn caps lock off */
#define APP_TYPE2 0

#define TOPIC1   "Commands"

#define TOPIC1_2	"Ping"
#define TOPIC1_2KEY	"P"
#define TOPIC1_2CODE	$81

#define TOPIC1_3	"Netstat"
#define TOPIC1_3KEY	"N"
#define TOPIC1_3CODE	$82

#define TOPIC1_4	"Config"
#define TOPIC1_4KEY	"C"
#define TOPIC1_4CODE	$83

#define TOPIC1_5	"Device Status"
#define TOPIC1_5KEY	"S"
#define TOPIC1_5CODE	$85

#define TOPIC1_6	"Statistics"
#define TOPIC1_6KEY	"T"
#define TOPIC1_6CODE	$86

#define TOPIC1_7         "Quit App"
#define TOPIC1_7KEY      "Q"
#define TOPIC1_7CODE     $84

#define TOPIC2		"Connection"

#define TOPIC2_1	"Offline"
#define TOPIC2_1KEY	"OFF"
#define TOPIC2_1CODE	$87

#define TOPIC2_2	"Hangup"
#define TOPIC2_2KEY	"HANG"
#define TOPIC2_2CODE	$88

#define TOPIC2_3	"Online"
#define TOPIC2_3KEY	"ON"
#define TOPIC2_3CODE	$89

/* Package stuff */

#define PACKAGE_ID	$15
#define MAX_CALL_NUM    $16
#define PACK_NAME	"ZSock"
#define PACK_BOOT	1

#define MAKE_PACKAGE

#include <package.h>

#ifdef DOPACKSTR
/*
 * Structure for package
 *
 * First do some prototyping..then define the structure
 */

extern void pack_ayt();
extern void pack_bye();
extern void pack_dat();
extern int _syscall();
extern int _Interrupt();
extern int _DeviceOnline();
extern int _DeviceOffline();
extern int user_pagein();
extern int user_pageout();
extern int _GoTCP();

package_str zx_pstr[] = {
	{_Interrupt},
	{_syscall},
	{_DeviceOnline},
	{_DeviceOffline},
	{user_pagein},
	{user_pageout},
	{_GoTCP}
};



#else
#pragma asm
	XREF	_pack_ayt
	XREF	_pack_bye
	XREF	_pack_dat
	XREF	Interrupt
	XREF	syscall
	XREF	__DeviceOnline
	XREF	__DeviceOffline
	XREF	_user_Pagein
	XREF	_user_Pageout
	XREF	__GoTCP
	defw	Interrupt	;proper int code
	defw	syscall
	defw	__DeviceOffline
	defw	__DeviceOnline
	defw	_user_Pagein
	defw	_user_Pageout
	defw	__GoTCP		;GoTCP
#pragma endasm
#endif



#include <application.h>

#endif

/* THE END! */
