/*
 *	Functions which read the config at ZSock startup
 *
 *	djm 29/1/2000
 *
 *	This file contains routines to read the config files
 *	and find out:
 *
 *	- Hostname
 *	- Host IP address
 *	- Default Search Domain
 *	- DNS servers
 */

/* This enables the old style stdio library */
#define FDSTDIO 1
#define FILECHEAT 1

#include <stdio.h>
#include <string.h>
#include "zsock.h"

int gethostaddr_i(u8_t *, size_t);
int gethostname_i(u8_t *, size_t);
int ReadDomConfig(void);
/* Rip removes LF & CR - it's in udp_dom2.c */
extern char *rip(char *);

extern struct pktdrive *device;
extern struct pktdrive z88slip;

/*
 * Get the host address, stick it into buffer supplied
 */

int gethostaddr_i(host, size)
u8_t *host;
size_t size;
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

        line= fgets(host, size, hostfile);  /* Hostname */
        line= fgets(host, size, hostfile);  /* dotted ip addr */
        fclose(hostfile);
        if (line==0)
                return EOF;
	rip(host);
	return 0;
}

/*
 * 	Get the hostname into supplied buffer
 */

int gethostname_i(host, size)
u8_t *host;
size_t size;
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

        line= fgets(host, size, hostfile);
        fclose(hostfile);
        if (line==0)
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

int ReadDomConfig()
{
        FILE *domainfile;
        char buffer[20];
        int     i;
	int	num;
        char *line;

	num=0;
#ifdef FILECHEAT
	if ( (domainfile = (FILE *)open(HOSTNAME_FILE,O_RDONLY,0) ) == EOF ) 
		return EOF;
#else
        if ( (domainfile= fopen(HOSTNAME_FILE, "r") ) == NULL)
                return EOF;
#endif

        fgets(sysdata.domainname, MAXDOMSIZ-1, domainfile);

        for (i=0 ; i < MAXNAMESERV; i++) {
                line=fgets(buffer,19,domainfile);
                if (line)  {
                        if (sysdata.nameservers[i]=inet_addr_i(buffer)) num++;
                } else
                        break;
        }
        fclose(domainfile);
	sysdata.numnameserv=num;
        if (i == 0 ) return (EOF);
        return(0);
}

void ReadDevConfig()
{
	char	name[FILENAME_MAX];
	char	*line;
	FILE	*devfile;

	device = z88slip;
#ifdef FILECHEAT
	if ( (devfile = (FILE *)open(HOSTNAME_FILE,O_RDONLY,0) ) == EOF ) 
		return EOF;
#else
        if ( (devfile= fopen(HOSTNAME_FILE, "r") ) == NULL)
                return EOF;
#endif
        line= fgets(name, FILENAME_MAX-1, devfile);
        fclose(devfile);
        if (line==0)
		return;
	rip(name);
	if (LoadDriver(name) == NULL ) {
		return;
	}
	if (CheckDriver(DRIVER_ADDR)==NULL ) return;
	device=DRIVER_ADDR;
}


