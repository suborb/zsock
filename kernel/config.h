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
#define return_c return
#define return_nc return
#endif

#endif


