/*
 *     Routines to handle device drivers
 *
 *     $Id: device.c,v 1.1 2002-05-11 21:00:55 dom Exp $
 *
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

