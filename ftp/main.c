/* Start of ftp */



#include "ftp.h"


int main()
{
    char  buf[80];


    while ( 1 ) {
	printf("> ");
	fflush(stdout);
	fgets(buf,sizeof(buf),stdin);
	makeargs(buf);
	if ( cmdargc )
	    exec_cmd(cmdargc,cmdargv);	    
    }
}




