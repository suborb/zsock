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



int open_terminal()
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
        snprintf(cmd,sizeof(cmd),"/home/dom/bin/slattach -e -s 38400 -p slip %s\n",slave_name);
        printf("%s\n",cmd);
        system(cmd);
        exit(0);
    default: /* Parent */
    }

    // fcntl(fd, F_SETFL, O_NDELAY);

    fcntl(master, F_SETFL, O_NONBLOCK);

    printf("sucessfull\n");
    return master;
}
