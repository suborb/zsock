/* Utils to handle memory management */

#include "ppp.h"
#include <net/misc.h>

void *ppp_sys_alloc_pkt(int len)
{
	/* TCP alloc? */
	return (tcp_malloc(len+PPP_OVERHEAD)+PPP_OVERHEAD);
}

void ppp_sys_free_pkt(void *pkt)
{
	tcp_free(pkt-PPP_OVERHEAD);
}
