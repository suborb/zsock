/*
 *   "High" level memory routines for ZSock
 *
 *   (C) 1999-2002 D.J.Morris
 */

#include "zsock.h"


/*      Allocate/free memory for a packet - covers the overhead required by the device driver */
void pkt_free(void *buf)
{
        free(buf-sysdata.overhead);
}


void *pkt_alloc(size)
        int    size;
{
        void *ptr;

        if ( (ptr=malloc(size+sysdata.overhead)) == NULL) 
	    return NULL;
        return(ptr+sysdata.overhead);
}
