/* Start of ftp */



#include "ftp.h"


int main(int argc, char *argv[])
{
    char  buf[80];

    if ( argc == 2 ) {
	cmd_open(argc,argv);
    }

    while ( 1 ) {
	printf("> ");
	fflush(stdout);
	fgets(buf,sizeof(buf),stdin);
	makeargs(buf);
	if ( cmdargc )
	    exec_cmd(cmdargc,cmdargv);	    
    }
}




