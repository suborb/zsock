#ifndef __OS_TYPES_H
#define __OS_TYPES_H

#include "config.h"

#ifdef LINUX
#if 0
	typedef	unsigned int	UDWORD;
	typedef unsigned short	UWORD;
	typedef unsigned char	UBYTE;
	typedef signed short	WORD;
	typedef signed int		DWORD;
typedef char            BYTE;

	typedef signed char	BOOL;

	#define FALSE		0
	#define TRUE		(!FALSE)
#endif

	#define BUFFER_SIZE	256
#endif
#ifdef GAMEBOY
/* Most are defined in types.h */
	#include <types.h>

	typedef unsigned long long	UDWORD;
	typedef signed long long	DWORD;

	typedef signed char		BOOL;
	#define BUFFER_SIZE	120
#endif

#ifdef SCCZ80
        typedef unsigned long UDWORD;
        typedef unsigned int  UWORD;
        typedef unsigned char   UBYTE;
        typedef signed short    WORD;
	typedef unsigned char   BOOL;
	typedef char            BYTE;
	#define FALSE		0
	#define TRUE		1
	#define BUFFER_SIZE	256
#endif

#ifndef SCCZ80
//#define DEBUG_PACKET_POLL
//#define DEBUG_HLDC_POLL_GOT
//#define DEBUG_HLDC_POLL
#define DEBUG_PPP_STATE_MACHINE
#define DEBUG_PPP_STATE_MACHINE_CONFIG
//#define DEBUG_PPP_POLL
#endif


#endif /* __OS_TYPES_H */
