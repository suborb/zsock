/*  struct hostent *gethostbyname(hostname); */


#include "socket.h"


struct hostent *gethostbyname(char *hostname)
{
    struct hostent hent;
    
    return ( gethostbyname_r(hostname,&hent) );
}

