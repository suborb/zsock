/*
 * Copyright (c) 1999-2002 Dominic Morris
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Dominic Morris.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the ZSock TCP/IP stack.
 *
 * $Id: device.c,v 1.3 2002-05-13 20:00:48 dom Exp $
 *
 * Handle device drivers
 */



#include "zsock.h"




struct pktdrive       *device;       /* Our device */





void device_report()
{
	int	bind;
	bind = PageDevIn();
	printf("Report on device: %s\n",device->copymsg);
	printf("Status is: %d\n",device->statusfn());
	PageDevOut(bind);
}


struct pktdrive *device_insert(char *devfile)
{
    int	bind;
    int loaded;

    loaded = device_load(devfile);
    bind = PageDevIn();
    if ( loaded  && device_check( ((void *)DRIVER_ADDR) ) ) {
	printf("Driver loaded - %d bytes\n",loaded);
	device = DRIVER_ADDR;
    } else {
	if ( loaded ) 
	    printf("Loaded, but not driver\n");
	else 
	    printf("Driver not found\n");
	device = z88slip;
    }
    PageDevOut(bind);
}

/*
 *	Read in a device driver to DRIVER_ADDR (8192)
 *
 *	Returns length read or 0 for no file etc..
 */

int device_load(char *name)
{
    int	bind;
    int	len;
    int	fd;

    if ((fd=open(name,O_RDONLY,0)) == EOF) 
	return 0;
    /* No fstat in z88 lib yet, so read as much as we can */
    bind = PageDevIn();
    len = read(fd,DRIVER_ADDR,8192);
    close(fd);
    PageDevOut(bind);
    return(len);
}



/* Check to see if the device has the magic */
int device_check(struct pktdrive *ptr)
{
        if (strcmp(ptr->magic,"ZS0PKTDRV") ) 
	    return(FALSE);
	return (TRUE);
}



/* Attach a device into the system */
int device_attach(struct pktdrive *ptr)
{
    int	bind;
    bind = PageDevIn();
    if (device_check(ptr)==NULL) {
	PageDevOut(bind);
	return(FALSE);
    }
    /* Call initialisation routine */
    sysdata.overhead = ptr->initfunc();   
    PageDevOut(bind);
    return(TRUE);
}

