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



#include "zsock.h"


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

u32_t set_ttimeout(int secs)
{
	return (current_time()+((u32_t)(secs*100)));
}

u32_t set_timeout(int tensms)
{
	return (current_time()+tensms);
}



/*
 * Check the timer supplied to see if it has timed out
 * Return TRUE for timeout
 */

int chk_timeout(u32_t time)
{
	u32_t now;

	now=current_time();

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

void SetTIMEOUTtime(TCPSOCKET *s)
{	
	s->timeout=set_ttimeout(tcp_TIMEOUT);
}

void SetLONGtimeout(TCPSOCKET *s)
{
	s->timeout=set_ttimeout(tcp_LONGTIMEOUT);
}

