#include "ftp.h"


char input_buffer[160];

char file_buffer[1024];

int  hash;

int ftp_returncode()
{
    char  result[5];
    int   res;

    while ( 1 ) {
	net_getline(input_buffer,sizeof(input_buffer));
	fputs(input_buffer,stdout);
	fputc('\n',stdout);
	strncpy(result,input_buffer,4);
	if ( isdigit(result[0]) && isdigit(result[1]) &&
	     isdigit(result[2]) && result[3] == ' ' ) {
	    res = atoi(result);
	    break;
	}
    }
    return res;
}
	
	
/* Send a command to the session */
int ftp_send(char *buf)
{
    net_sendline(buf);
    
    return (ftp_returncode());
}

int ftp_data(char *buf, int direction, FILE *fp)
{
    char      activebuf[50];
    ipaddr_t  dest;
    tcpport_t port;
    int       ret;
    u8_t     *ptr;
    int	      i;
  
    if ( passive ) {
	ret = ftp_send("PASV\r\n");
	if ( ret != 227 )
	    return ret;
	ptr = input_buffer;
	while (*ptr && *ptr != '(')
	    ptr++;
	++ptr;
	dest = 0;
	for ( i = 0; i < 4; i++ ) {
	    dest = ( dest << 8 ) + atoi(ptr);
	    if ( ( ptr = strchr(ptr,',') ) == NULL ) {
		printf("Couldn't parse PASV response\n");
		return -1;
	    }
	    ++ptr;
	}
	port = atoi(ptr);
	if ( ( ptr = strchr(ptr,',') ) == NULL ) {
	    printf("Couldn't parse PASV response\n");
	    return -1;
	}
	++ptr;
	port = ( port << 8 ) + atoi(ptr);
	dest = htonl(dest);
#if 0
	port = htons(port);
#endif
	ftpdata_fd = net_connect_ip(dest,port);
    } else {
#define hiword(x)       ((u16_t)((x) >> 16))
#define loword(x)       ((u16_t)(x & 0xffff)) 
#define hibyte(x)       (((x) >> 8) & 0xff)
#define lobyte(x)       ((x) & 0xff)
	ftpdata_fd = net_listen();
	net_getsockinfo(&dest,&port);
	sprintf(activebuf,"PORT %u,%u,%u,%u,%u,%u\r\n",
		hibyte(hiword(ntohl(dest))), lobyte(hiword(ntohl(dest))),
		hibyte(loword(ntohl(dest))), lobyte(loword(ntohl(dest))),
		hibyte(ntohs(port)), lobyte(ntohs(port)));
	ret = ftp_send(activebuf);
	if ( ret != 200 ) {
	    net_close_fd(ftpdata_fd);
	    return (ret);
	}
    }
    ret = ftp_send(buf);
    if ( passive || ret == 125 || ret == 150 ) {         /* 150 opening ascii connection 125 = data conn already open */
	if ( !passive ) {
	    if ( net_wait_connect() == -1 ) {
		printf("Child listener timed out");
		net_close_fd(ftpdata_fd);
	    }
	}
	switch ( direction ) {
	case RETR:
	    ret = recvfile(fp);
	    break;
	case STOR:
	    ret = sendfile(fp);
	}
	net_close_fd(ftpdata_fd);
	ret = ftp_returncode();
    } else {
	net_close_fd(ftpdata_fd);
    }
    return (ret);
}

/* These return vals are wrong */
int sendfile(FILE *fp)
{
    int           s,i;
    unsigned long c = 0,total = 0UL;

    while ( ( s = fread(file_buffer,1,sizeof(file_buffer),fp) ) != 0  ) {
	net_write(ftpdata_fd,file_buffer,s);
	total += s;
	if ( hash && total/ 256 > c ) {
	    i = total / 256 - c;
	    c = total/256;
	    while ( i-- )
		printf("#"); 
	    fflush(stdout);
	}
    }
    printf("\n");
    return total;
}
		

int recvfile(FILE *fp)
{
    int           s,i;
    unsigned long c = 0,total = 0UL;
    while ( ( s = net_read(ftpdata_fd,file_buffer,sizeof(file_buffer)) ) != -1 ) {
	if ( s == 0 )
	    break;
        fwrite(file_buffer,1,s,fp);
	total += s;
	if ( hash && fp != stdout &&  total/ 256 > c ) {
	    i = total / 256 - c;
	    c = total / 256;
	    while ( i-- )
		printf("#"); 
	    fflush(stdout);
	}
    }
    if ( hash && fp != stdout )
	printf("\n");
    return total;
	
}


