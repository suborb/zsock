#ifndef __OS_TYPES_H
#define __OS_TYPES_H

#ifdef LINUX
	typedef	unsigned int	UDWORD;
	typedef unsigned short	UWORD;
	typedef unsigned char	UBYTE;
	typedef signed short	WORD;
	typedef signed int		DWORD;

	typedef signed char	BOOL;

	#define FALSE		0
	#define TRUE		(!FALSE)

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

#ifdef SMALL_C
        typedef unsigned long UDWORD;
        typedef unsigned short  UWORD;
        typedef unsigned char   UBYTE;
        typedef signed short    WORD;
	typedef unsigned char   BOOL;
	typedef char            BYTE;
	#define FALSE		0
	#define TRUE		1
	#define BUFFER_SIZE	256
#endif

#endif /* __OS_TYPES_H */
