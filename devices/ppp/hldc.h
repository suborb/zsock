/*
  hldc.c

  Implementation of hldc-on-serial
*/
#ifndef __OS_HLDC_H
#define __OS_HLDC_H

#include "types.h"


struct _queue_hdr {
    BYTE    *next;
    UWORD   len;
    UWORD   crc;
    UWORD   dll;
    BYTE    data;
};

typedef struct _queue_hdr queuehdr_t;

/* The various HLDC options, set by the LCP negotiation stage */
#define HLDC_SEND_COMPRESS_HEADER	1
#define HLDC_SEND_COMPRESS_PROTO	2
#define HLDC_RECV_COMPRESS_HEADER	4
#define HLDC_RECV_COMPRESS_PROTO	8

extern int hldc_init(void);
extern int hldc_open(void);
extern int hldc_close(void);
extern UBYTE *hldc_prep(void);
extern int hldc_send(UBYTE *buffer,UWORD len, BOOL can_compress);
extern UWORD hldc_poll(void **pkt);

/* Set the HLDC options, getting the last options */
extern UBYTE hldc_set_options(UBYTE new_options);
extern UBYTE hldc_get_options(void);

/* Stuff to do byte at time gunk */
extern UWORD hldc_byte_in(void **pkt);
extern void *hldc_byte_out();
extern void *hldc_byte_send_head();
extern void *hldc_byte_send_dll();
extern void *hldc_byte_send_esc();
extern void *hldc_byte_send_body();
extern void *hldc_byte_send_cksum();
extern void *hldc_byte_send_flags();




#endif /* __OS_HLDC_H */
