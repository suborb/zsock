/*
 *	Z88 Routines To Handle Time of Day And Other
 *	Such things...
 *
 *	Timeouts are handled in OZ timer jimmies
 *
 *	djm 7/12/99
 *
 *	djm 7/1/2000 And I wondered why my timeouts weren't
 *	quite working properly set_ttimeout was using a 
 *	*very* random value *sigh* Fixed 'n' shorter..
 */

#pragma asm
	INCLUDE	"#time.def"
#pragma endasm

#include "zsock.h"

LWORD current();

#define OZDAY 100*60*60*24

/*
 * OZSPLIT.. is used as a bit of a fudge...
 * ..to handle midnight problems...
 */

#define OZSPLIT1 100*60*5

/*
 *	Set the timeout, entry in seconds, exit in
 *	oz units
 */

LWORD set_ttimeout(int secs)
{
	return (current()+((WORD)secs*100));
}

LWORD set_timeout(int tensms)
{
	return (current()+(WORD)tensms);
}


/*
 * Get the current time
 */

LWORD current()
{
#pragma asm
	ld	de,1
	call_oz(gn_gmt)	;abc, a=MSB
	ld	d,0
	ld	e,a
	ld	h,b
	ld	l,c
#pragma endasm
}


/*
 * Check the timer supplied to see if it has timed out
 * Return TRUE for timeout
 */

int chk_timeout(LWORD time)
{
	LWORD now;

	now=current();

/*
 * 	Bit of fudge time
 */
	if (time>OZDAY && now<OZSPLIT1) now+=OZDAY;


/*
 *	This will screw up big time over midnight...
 */
	if	(now > time ) return(1);
	return 0;
}

/*
 * Set various TCP timeout times 
 */

SetTIMEOUTtime(TCPSOCKET *s)
{	
	s->timeout=set_ttimeout(tcp_TIMEOUT);
}

SetLONGtimeout(TCPSOCKET *s)
{
	s->timeout=set_ttimeout(tcp_LONGTIMEOUT);
}


/*
 *      Get an initial TCP sequence number,
 *      obtained from time of day, so a little bit cheaty!
 */

LWORD GetSeqNum()
{
#pragma asm
          ld   de,1
          call_oz (gn_gmt)
	  ld	l,c
	  ld	h,b
          ld   e,a
          ld   d,c	;more random
#pragma endasm
}
