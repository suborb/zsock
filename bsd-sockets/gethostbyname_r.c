/*  struct hostent *gethostbyname(hostname, struct hostent *); */


#include "socket.h"


struct hostent *gethostbyname_r(char *hostname, struct hostent *hent)
{
    hent->h_addrtype = AF_INET;
    hent->h_name     = hostname;
    hent->aliases    = NULL;
    hent->h_length   = sizeof(ipaddr_t);

    if ( ( hent->h_addr = resolve(hostname) ) == 0L ) {
	return NULL;
    }

    return hent;
}
