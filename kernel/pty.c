#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef linux
#include "pty.h"
#else
/* openbsd */
#include <util.h>   
#endif



int open_terminal(char *mode)
{
    int    master,slave;
    char   cmd[2048];
    char   slave_name[FILENAME_MAX];


    if ( openpty(&master,&slave,slave_name,NULL,NULL) == -1 ) {
        perror("openpty");
        exit(1);
    }
    close(slave);

    switch ( fork() ) {
    case -1:   /* Fork failed */
        perror("fork");
        exit(1);
    case 0:   /* Child */
#ifdef PPP
	snprintf(cmd,sizeof(cmd),"/usr/sbin/pppd -detach %s",slave_name);
#else
        snprintf(cmd,sizeof(cmd),"/home/dom/bin/slattach -e -s 38400 -p %s %s\n",mode,slave_name);
#endif
        printf("%s\n",cmd);
        system(cmd);
        exit(0);
    default: /* Parent */
	sleep(1);
#ifndef PPP
	system("/sbin/ifconfig sl0 192.168.155.12 dstaddr 192.168.155.88 up\n");
#endif

    }

    fcntl(master, F_SETFL, O_NONBLOCK);

    printf("sucessfull\n");
    return master;
}
