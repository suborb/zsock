#include "ftp.h"


tcpsock_t    ftpdata_fd;
tcpsock_t    ftpctrl_fd;


ipaddr_t     client_addr;
ipaddr_t     server_addr;

tcpport_t    local_port;

int          connected;           /* Whether ctrl is connected */



int net_open_ctrl(char *hostname, tcpport_t port)
{
    struct sockinfo_t info;
    ipaddr_t          remote;

    if ( ( remote = resolve(hostname) ) == 0L ) {
	printf("Unknown host\n");
	return -1;
    }

    server_addr = remote;

    if ( (ftpctrl_fd = net_connect_ip(remote,port) ) != -1 ) {
	connected = TRUE;
	printf("Connected to %s:%d\n",hostname,port);
    } else {
	printf("Connection refused by %s:%d\n",hostname,port);
    }

    if ( sock_getinfo(ftpctrl_fd,&info) != 0 ) {
	net_close_fd(ftpctrl_fd);
	return -1;
    }
    client_addr = info.local_addr;
    
    return 0;
}

tcpsock_t net_connect_ip(ipaddr_t addr, tcpport_t port)
{
    tcpsock_t  ret;

    if ( ( ret = sock_open(addr,port,0,prot_TCP) ) == 0 ) {
	return -1;
    }

    if ( sock_waitopen(ret) == - 1 ) {
	net_close_fd(ret);
	return -1;
    }
    return ret;
}


int net_close_ctrl()
{
    net_close_fd(ftpctrl_fd);

}


int net_close_fd(tcpsock_t sock)
{
    sock_close(sock);
    if ( sock_waitclose(sock) == -1 )
	return -1;
    sock_shutdown(sock);
    return 0;
}

/* Wait for a listening socket to connect */
int net_wait_connect(tcpsock_t sock)
{
    if ( sock_waitclose(sock) == -1 ) {
	net_close_fd(sock);
	return -1;
    }
    return 1;
}



/* Send a line to the ctrl fd */
int net_sendline(char *buf)
{
    sock_write(ftpctrl_fd,buf,strlen(buf));

}


/* Listen to connection from server_addr */
int net_listen()
{   
    ftpdata_fd = sock_listen(server_addr,0,0,prot_TCP);

    if ( ftpdata_fd == 0 ) 
	return -1;

    return ftpdata_fd;
}



int net_read(tcpsock_t fd, char *buf, int buflen)
{
    return ( sock_read(fd,buf,buflen) );
}


int net_write(tcpsock_t fd, char *buf, int buflen)
{
    return ( sock_write(fd,buf,buflen) );
}

int net_getsockinfo(ipaddr_t *addr, tcpport_t *port)
{
    struct sockinfo_t  info;

    if ( sock_getinfo(ftpdata_fd,&info) != 0 ) {
	net_close_fd(ftpdata_fd);
	return -1;
    }

    *addr = info.local_addr;
    *port = info.local_port;
    return 0;
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
            offset = sock_read(sock,buf+pos,sizeof(buf) - pos - 1 );
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
