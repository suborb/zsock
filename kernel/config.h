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
 * $Id: config.h,v 1.7 2002-06-01 21:43:18 dom Exp $
 *
 * Machine tweaking
 *
 */




#ifndef CONFIG_H
#define CONFIG_H

/*
 * Interrupts aren't working quite as they should be
 * so we have to busy loop, this file sets up various
 * things for this..
 */

#ifdef Z88
#define BUSYLOOP() _GoTCP()
#define BUSYINT()  Interrupt()
#else
#define BUSYLOOP() Interrupt()
#define BUSYINT()  Interrupt()
#define PageDevIn() 0
#define PageDevOut(x)
#endif

#define BUSY_VERSION 1

#ifdef Z80
#define HCALL Handler_Call
#endif

#ifndef Z80
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef char i8_t;
typedef short i16_t;
typedef int i32_t;
#endif

/* Remove qualifiers used by sccz80 */
#ifndef SCCZ80
#define __FASTCALL__
#define __SHARED__
#define __APPFUNC__
#define __CALLEE__
#define __LIB__
#define return_c(x,y) { errno = x; return (y); }
#define return_ncv(x)  { errno = 0; return (x); }
#define return_nc    { errno = 0; return; }
#define GETKEY()  fgetc(stdin)
#define getk()    0
#define fgets_cons(x,y) fgets(x,y,stdin);
#else
#define return_ncv(x)  return_nc(x)
#define return_nc return_nc
#define GETKEY() getkey()
#ifdef __CPM__
#define getk()   0
#endif

#endif

#endif


