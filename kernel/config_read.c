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
 * $Id: config_read.c,v 1.3 2002-05-13 20:00:48 dom Exp $
 *
 * Z88 Config Reading
 *
 */


#define FDSTDIO   1            /* Enable old style stdio */

#ifdef SCCZ80
#define FILECHEAT 1            /* Short loading routines */
#endif


#include "zsock.h"


/* Rip removes LF & CR - it's in udp_dom2.c */
extern char *rip(char *);


/*
 * Get the host address, stick it into buffer supplied
 */

int config_hostaddr(u8_t *host, size_t size)
{
    FILE *hostfile;
    char *line;

#ifdef FILECHEAT
    if ( (hostfile = (FILE *)open(HOSTNAME_FILE,O_RDONLY,0) ) == EOF ) 
	return EOF;
#else
    if ( (hostfile= fopen(HOSTNAME_FILE, "r") ) == NULL)
	return EOF;
#endif

    line = fgets(host, size, hostfile);  /* Hostname */
    line = fgets(host, size, hostfile);  /* dotted ip addr */
    fclose(hostfile);
    if ( line == NULL )
	return EOF;
    rip(host);

    return 0;
}

/*
 * 	Get the hostname into supplied buffer
 */

int config_hostname(u8_t *host, size_t size)
{
    FILE *hostfile;
    char *line;

#ifdef FILECHEAT
    if ( (hostfile = (FILE *)open(HOSTNAME_FILE,O_RDONLY,0) ) == EOF ) 
	return EOF;
#else
    if ( (hostfile= fopen(HOSTNAME_FILE, "r") ) == NULL)
	return EOF;
#endif

    line = fgets(host, size, hostfile);
    fclose(hostfile);
    if ( line == NULL )
	return EOF;
    rip(host);
    return 0;
}

/*
 *      Read in domain configuration
 *
 *      File has set up
 *      domainanme
 *      ns0
 *      ns1
 */

int config_dns()
{
    FILE *domainfile;
    char  buffer[20];
    int   i;
    int	  num;
    char *line;

    num=0;
#ifdef FILECHEAT
    if ( (domainfile = (FILE *)open(DOMAIN_FILE,O_RDONLY,0) ) == EOF ) 
	return EOF;
#else
    if ( (domainfile = fopen(DOMAIN_FILE, "r") ) == NULL)
	return EOF;
#endif

    fgets(sysdata.domainname, MAXDOMSIZ-1, domainfile);

    for ( i=0 ; i < MAXNAMESERV; i++) {
	line = fgets(buffer,19,domainfile);
	if (line)  {
	    if (sysdata.nameservers[i]=inet_addr_i(buffer)) 
		++num;
	} else
	    break;
    }
    fclose(domainfile);
    sysdata.numnameserv = num;
    if ( i == 0 ) 
	return (EOF);
    return(0);
}


void config_device()
{
	char	name[FILENAME_MAX];
	char	*line;
	FILE	*devfile;

	device = z88slip;
#ifdef FILECHEAT
	if ( (devfile = (FILE *)open(DEVICE_FILE,O_RDONLY,0) ) == EOF ) 
		return;
#else
        if ( (devfile= fopen(DEVICE_FILE, "r") ) == NULL)
                return;
#endif
        line = fgets(name, FILENAME_MAX-1, devfile);
        fclose(devfile);
        if (line == NULL)
		return;
	rip(name);
	if (device_load(name) == NULL ) {
		return;
	}
	if ( device_check( ((void *)DRIVER_ADDR) ) ) 
	    device = DRIVER_ADDR;	
}



