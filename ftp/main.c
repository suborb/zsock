/* Start of ftp */



#include "ftp.h"


int main(int argc, char *argv[])
{
    char  buf[80];

#ifndef SCCZ80
    StackInit();
#endif

    if ( argc == 2 ) {
	cmd_open(argc,argv);
    }

    while ( 1 ) {
	printf("ftp> ");
	fflush(stdout);
	fflush(stdin);
#ifdef SCCZ80
	fgets_net(buf,sizeof(buf),TRUE);
#else
	fgets(buf,sizeof(buf),stdin);
#endif
	makeargs(buf);
	if ( cmdargc )
	    exec_cmd(cmdargc,cmdargv);	
	buf[0] = 0;
    }
}




