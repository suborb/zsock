#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <net/socket.h>
#include <net/hton.h>

/* Used by gethostbyname */
struct hostent {
    char   *h_name;
    char  **h_aliases;
    int     h_addrtype;
    int     h_length;
#if 0
    char   *h_addr_list;
#else
    char  **h_addr_list;
#define h_addr h_addr_list[0]
#endif
};

/* Used by inet_aton */
struct in_addr {
    ipaddr_t s_addr;
};

typedef unsigned short sa_family_t;



struct sockaddr_in {
    sa_family_t    sin_family;
    tcpport_t      sin_port;
    struct in_addr sin_addr;
};

struct sockaddr {
    union {
	struct sockaddr_in sin;
    } addr;
};

/* Supported address families */
#define AF_INET        2   /* Internet IP Protocol         */ 


/* Protocol families - same as address families */
#define PF_INET        AF_INET


/* Socket types */
#define SOCK_STREAM    1
#define SOCK_DGRAM     2

extern int __LIB__  connect(int fd, struct sockaddr *addr, int addrlen);
extern struct hostent __LIB__ *gethostbyname(char *hostname);
extern struct hostent __LIB__ *gethostbyname_r(char *hostname,struct hostent *);
extern int __LIB__ socket(int domain, int type, int protocol);
extern int __LIB__ socket_close(int fd);
extern int __LIB__ socket_read(int fd, void *buf,size_t count);
extern int __LIB__ socket_write(int fd, void *buf,size_t count);

/* Internal stuff */

#ifndef NULL
#define NULL  ((void *)0)
#endif

struct __socket {
    void  *socket;
    u8_t   protocol;
};

extern struct __socket *socket_get(int fd);


#endif /* __SOCKET_H__ */
