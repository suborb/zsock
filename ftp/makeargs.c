/*
 * Copyright (c) 2001-2002 Dominic Morris
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Dominic Morris.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the ZSock TCP/IP stack.
 *
 * $Id: makeargs.c,v 1.2 2002-06-08 17:19:03 dom Exp $
 *
 */


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
