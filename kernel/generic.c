/*
 *   A bunch of generic routines for cross platform usage
 *
 *   (C) 2002 D.J.Morris
 *
 */

#include "zsock.h"
#include <sys/time.h>

/* Returns number of 10ms since start of day */
u32_t current_time()
{
    struct timeval tv;
    struct timezone tz;
    long  millisec;
    
    gettimeofday(&tv,&tz);
 
    millisec = tv.tv_sec * 100 + ( tv.tv_usec / 10000 );

    return millisec;
}

/* Return a sequence number of a TCP connection */
u32_t GetSeqNum()
{
    struct timeval tv;
    struct timezone tz;
    long  millisec;
    
    gettimeofday(&tv,&tz);

    return ( tv.tv_usec );   /* Not great, but okay-ish */
}


/* I think this is correct... */
u16_t inet_cksum_pseudo(void *ip,void *tcp,u8_t protocol,u16_t length)      
{
    u16_t  acc;

    acc = inet_cksum(&protocol,1);

    acc += inet_cksum(&ip->source,8);
    acc += inet_cksum(tcp,length);

    return acc;
}

u16_t ip_check_cksum(ip_header_t *buf)
{
    return ( inet_cksum(buf,buf->version&15 << 2 ) )
}

void inet_cksum_set(ip_header_t *buf)
{
    buf->cksum = 0;
    buf->cksum = inet_cksum(buf,buf->version&15 << 2 );
}

static u16_t inet_cksum(u16_t *buf, u16_t len)
{
    u16_t acc;
  
    for(acc = 0; len > 1; len -= 2) {
	acc += *buf;
	if(acc < *buf) {
	    /* Overflow, so we add the carry to acc (i.e., increase by
	       one). */
	    ++acc;
	}
	++buf;
    }

    /* add up any odd byte */
    if ( len == 1 ) {
	acc += htons(((u16_t)(*(u8_t *)buf)) << 8);
	if(acc < htons(((u16_t)(*(u8_t *)buf)) << 8)) {
	    ++acc;
	}
    }

    return acc;
}
