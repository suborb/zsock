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
 * $Id: app.c,v 1.6 2002-06-08 16:26:03 dom Exp $
 *
 *
 * Set up Z88 application structures
 *
 */




#include "zsock.h"

#ifdef Z88
        
#include <dor.h>

#define HELP1	"A demo of ZSock the Z88 TCP/IP Stack made with z88dk"
#define HELP2	"Read the documentation for configuration info"
#define HELP3	"This demo contains shared library facilities,"
#define HELP4	"echo/qotd/finger services and a ping client"
#define HELP5   ""
#define HELP6   "v2.31 (c) 8/6/2002 D.J.Morris"

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
