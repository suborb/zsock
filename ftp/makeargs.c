/* Routine to make arguments */

#include "ftp.h"


char *cmdargv[MAXARGS];
int   cmdargc;

int makeargs(char *line)
{
    int     i;
	char	*ptr;

	ptr = line + strlen(line) - 1;	/* Point to the end */

	/* Strip off th ending */
	while ( ptr != line ) {
		if ( isspace(*ptr) )
			*ptr = 0;
		else
			break;
	}

	cmdargc = 0;
	for ( i = 0; i <MAXARGS; i++ )
		cmdargv[i] = NULL;

	ptr = line;

	while ( cmdargc < MAXARGS ) {
		while (*ptr && isspace(*ptr) )
			ptr ++;
		if ( *ptr == 0 )
			break;
		cmdargv[cmdargc++] = ptr;
		while ( *ptr && !isspace(*ptr) ) {
			/* Put command in lower case */
			if ( cmdargc == 1 )
				*ptr = tolower(*ptr);
			++ptr;
		}
		if ( *ptr == 0 )	/* end of line */
			break;
		*ptr = 0;	/* terminate word */
		++ptr;
	}
	return cmdargc;
}

int makeargs_rip(char *buf)
{
    char *temp;
    if (temp = strchr(buf,'\n') ) 
	*temp = 0;
    if ( temp = strchr(buf,'\r') ) 
	*temp = 0;
}
