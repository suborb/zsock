#include "ftp.h"


tcpsock_t    ftpdata_fd;
tcpsock_t    ftpctrl_fd;

tcpsock_t    listen_sock;

ipaddr_t     server_addr;

tcpport_t    local_port;

int          connected;           /* Whether ctrl is connected */



int net_open_ctrl(char *hostname, tcpport_t port)
{
    struct sockaddr_in  sock;
    struct in_addr      host_addr;
    struct hostent     *hent;
    int    namelen;

    connected = FALSE;

    /* Clear the sockaddr_in */
    memset((void *)&sock, 0, sizeof(struct sockaddr_in));

    /* Resolve the host name */
    if (inet_aton(hostname, &host_addr)!=0) {
	memcpy((void *)&sock.sin_addr, (void *)&host_addr.s_addr, sizeof(struct in_addr));
    } else {
	hent=gethostbyname(hostname);
	if (hent!=NULL) {
	    memcpy((void *)&sock.sin_addr, (void *)hent->h_addr,  hent->h_length );
	    sock.sin_family=hent->h_addrtype;
	} else {
	    printf("No host\n");
	    return -1;
	}
    }

    memcpy(&server_addr,&sock.sin_addr,sizeof(struct in_addr));

    if ( (ftpctrl_fd = net_connect_ip(server_addr,port) ) != -1L ) {
	connected = TRUE;
	printf("Connected to %s:%d\n",hostname,port);
    } else {
	printf("Connection refused by %s:%d\n",hostname,port);
    }
  
    /* find out what port was assigned */
    namelen = sizeof(sock);
    memset(&sock,0,namelen);
    if( getsockname( ftpctrl_fd, (struct sockaddr *) &sock, &namelen) < 0 ) {
	fprintf(stderr, "getsockname() failed to get port number\n");
	exit(0);
    }
    server_addr = sock.sin_addr.s_addr;
    return 0;
}

tcpsock_t net_connect_ip(ipaddr_t addr, tcpport_t port)
{
    struct sockaddr_in sock;
    tcpsock_t  ret;

    sock.sin_port = htons(port);
    sock.sin_family = AF_INET;
    memcpy(&sock.sin_addr,&addr,sizeof(addr));

    ret = socket(sock.sin_family, SOCK_STREAM, 0);
    if (ret<0) {
	return -1;
    }
    if (connect(ret, (struct sockaddr *)&sock, sizeof(struct sockaddr_in))<0) {
	close(ret);
	return -1;
    }
    return ret;
}


int net_close_ctrl()
{
    close(ftpctrl_fd);

}


int net_close_fd(tcpsock_t sock)
{
    close(sock);

}

int net_wait_connect(tcpsock_t sock)
{
    struct sockaddr_in client;
    int    namelen = sizeof(struct sockaddr_in);
    if( (ftpdata_fd = accept(listen_sock, (struct sockaddr *)&client, &namelen)) == -1) {
	fprintf(stderr, "accept() failed to accept client connection request.\n");
	exit(0);
    }
    close(listen_sock);
    return 1;
}


/* Send a line to the ctrl fd */
int net_sendline(char *buf)
{
    write(ftpctrl_fd,buf,strlen(buf));

}



int net_listen()
{   
    struct sockaddr_in  sin;


    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = 0;

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1;

    if (bind(listen_sock, (struct sockaddr *) &sin, sizeof(sin)) <0)
	return -1;

    if (listen(listen_sock, 1) == -1)
	return -1;

    return listen_sock;
}



int net_read(tcpsock_t fd, char *buf, int buflen)
{
    return ( read(fd,buf,buflen) );
}


int net_write(tcpsock_t fd, char *buf, int buflen)
{
    return ( write(fd,buf,buflen) );
}

int net_getsockinfo(ipaddr_t *addr, tcpport_t *port)
{
    struct sockaddr_in local;
    int namelen;

    /* find out what port was assigned */
    namelen = sizeof(local);
    memset(&local,0,namelen);
    if( getsockname( listen_sock, (struct sockaddr *) &local, &namelen) < 0 ) {
	fprintf(stderr, "getsockname() failed to get port number\n");
	exit(0);
    }
    *port = local.sin_port;
    *addr = server_addr;
}



   







char *net_readline(int sock)
{
    static char buf[512];
    static int  pos = 0;
    static int  len = 0;
    int         start,offset,readflag;
    char       *ptr,ret;

    if ( len == 0 )
        readflag = 1;
    else
        readflag = 0;



    while ( 1 ) {
        if ( readflag ) {
            offset = read(sock,buf+pos,sizeof(buf) - pos - 1 );
            if ( offset <= 0 )
                return NULL;
            len += offset;

        }

        start = pos;
        while ( buf[pos] != '\r' && buf[pos] != '\n' && pos < len )
            ++pos;

        if ( pos == len ) {
            len -= start;
            memcpy(buf,buf+start,len);
            pos = 0;
            readflag = 1;
            continue;
        }
        /* We now have a line */
        buf[pos] = 0;
        pos += 2;
        if ( pos >= len ) {
            len = 0;
            pos = 0;
        }
        return buf + start;
    }
    return NULL;
}

int net_getline(char *buf, int len)
{
    char  *ptr;

    ptr = net_readline(ftpctrl_fd);

    if ( ptr == NULL ) {
        buf[0] = 0;
    } else {      /* Hope it's shorter than what we ask for.. */
        strncpy(buf,ptr,len);
    }
}
