

#include "ftp.h"


int net_connect_ip(ipaddr_t addr, tcpport_t port)
{
    int     fd;

    fd = (int) sock_open(addr,port,NULL,prot_TCP);

    if ( fd == 0 )
	return fd;

    switch ( (char) sock_waitopen(fd) ) {
    case 0:         /* Closed */
    case -1:        /* abort */
	sock_shutdown(fd;
	return 0;
    case 1:
	return fd;
    }   
}

int net_close(int fd)
{
    sock_close(s);
    sock_waitclose(s);
    sock_shutdown(s);
}


int net_putc(int fd,int c)
{
        int     num=0;

        while (num==0) {
                GoTCP();

                num=sock_putc(c,fd);
                iferror return EOF;
        }
        return(c);
}

int net_getc(int fd)
{
        char    pad;
        unsigned char   c;
        int     num=0;

        while (1) {
                GoTCP();
                if (sock_dataready(fd) ) break;
                if (sock_closed(fd) ) return_nc -1;
        }
        sock_read(fd,&c,1);
        return_nc(c);
}

int net_write(int fd,char *buf, int len)
{
    int  ret,count;

    count = 0;;
    while ( len ) {
	ret = net_putc(fd,buf[count]);
	if ( ret == -1 ) {
	    return count;
	}
	count++;
	len--;
    }
    return count;
}


int net_read(int fd, char *buf, int maxlen)
{
    int   count;

    while ( ( count = sock_read ) != -1 ) {
    }
}









