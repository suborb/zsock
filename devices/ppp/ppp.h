#ifndef __OS_PPP_H
#define __OS_PPP_H

#include <stdio.h>
#include "hldc.h"
#include "types.h"


#define IPCP_IP_COMPRESS_PROTOCOL	2
#define IPCP_IP_ADDRESS			3



struct sipcp_header {
	UBYTE	type;
	UBYTE	length;
	union uipcp_header_data {
		UWORD	compression_protocol;
		UDWORD	addr;
	} u;
};

typedef struct sipcp_header mipcp_header;

extern int ppp_init(void);
extern int ppp_open(void);
extern int ppp_close(void);
extern int ppp_send(void *pkt, UWORD len);
extern UBYTE *ppp_poll(void);
extern UBYTE *ppp_prep(void);

extern void *ppp_sys_alloc_pkt(int len);
extern void ppp_sys_free_pkt(void *pkt);


#endif /* __OS_PPP_H */
