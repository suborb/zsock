/* Utils to handle memory management */

#include "ppp.h"
#include <net/misc.h>

void *ppp_sys_alloc_pkt(int len)
{
    char  *ptr = tcp_malloc(len+PPP_OVERHEAD);
    //printf("ALlocated %d bytes @ %p (%p)\n",len,ptr,ptr + PPP_OVERHEAD);
	/* TCP alloc? */
    return ( ptr + PPP_OVERHEAD );
}

void ppp_sys_free_pkt(void *pkt)
{
    // printf("Freeing %p\n",pkt);
    // tcp_free(pkt);
}
