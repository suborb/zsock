/*
  hldc.c

  Implementation of hldc-on-serial
*/
#ifndef __OS_HLDC_H
#define __OS_HLDC_H

#include "types.h"


struct _queue_hdr {
    void   *next;
    u16_t   len;
    u16_t   crc;
};


typedef struct _queue_hdr queuehdr_t;

#ifdef SCCZ80
#define PPP_OVERHEAD                    8
#else
#define PPP_OVERHEAD  (sizeof(queuehdr_t) + 2)       /* +2 for dll */
#endif

/* The various HLDC options, set by the LCP negotiation stage */
#define HLDC_SEND_COMPRESS_HEADER	1
#define HLDC_SEND_COMPRESS_PROTO	2
#define HLDC_RECV_COMPRESS_HEADER	4
#define HLDC_RECV_COMPRESS_PROTO	8

extern int hldc_init(void);
extern int hldc_open(void);
extern int hldc_close(void);
extern u8_t *hldc_prep(void);
extern u16_t hldc_poll(void **pkt);

/* Set the HLDC options, getting the last options */
extern u8_t hldc_set_options(u8_t new_options);
extern u8_t hldc_get_options(void);

/* Stuff to do byte at time gunk */
extern u16_t hldc_byte_in(void **pkt);
extern u16_t hldc_poll_in(void **ret);
extern void *hldc_byte_out();
extern void *hldc_byte_send_head();
extern void *hldc_byte_send_dll();
extern void *hldc_byte_send_esc();
extern void *hldc_byte_send_body();
extern void *hldc_byte_send_cksum();
extern void *hldc_byte_send_flags();

extern int   serial_in();
extern       serial_out(u8_t);
#ifndef SCCZ80
extern int   ser_error;
#endif



#endif /* __OS_HLDC_H */
