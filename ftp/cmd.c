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
 * $Id: cmd.c,v 1.7 2002-06-09 22:28:55 dom Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "ftp.h"


struct _commands {
	char *cmd;
	int   needconnect;
	char *help;
	int (*fn)();
};

int cmd_ascii(int argc, char *argv[]);
int cmd_binary(int argc, char *argv[]);
int cmd_quit(int argc, char *argv[]);
int cmd_cd(int argc, char *argv[]);
int cmd_cdup(int argc, char *argv[]);
int cmd_close(int argc, char *argv[]);
int cmd_delete(int argc, char *argv[]);
int cmd_hash(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_lcd(int argc, char *argv[]);
int cmd_lls(int argc, char *argv[]);
int cmd_binary(int argc, char *argv[]);
int cmd_open(int argc, char *argv[]);
int cmd_passive(int argc, char *argv[]);
int cmd_quote(int argc, char *argv[]);
int cmd_size(int argc, char *argv[]);
int cmd_pwd(int argc, char *argv[]);
int cmd_shell(int argc, char *argv[]);
int cmd_rhelp(int argc, char *argv[]);
int cmd_user(int argc, char *argv[]);
/* Data commands */
int cmd_put(int argc, char *argv[]);
int cmd_get(int argc, char *argv[]);
int cmd_ls(int argc, char *argv[]);

struct _commands commands[] = {
    {"!",0,"Send command to the shell",cmd_shell},
    {"ascii",1,"Set ascii transfer mode",cmd_ascii},
    {"binary",1,"Set binary transfer mode",cmd_binary},
    {"bye",1,"Terminate ftp session and exit",cmd_quit},
    {"cd",1,"Change remote working directory",cmd_cd},
    {"cdup",1,"Change remote working directory to parent",cmd_cdup},
    {"close",1,"Terminate ftp session",cmd_close},
    {"delete",1,"Delete remote file",cmd_delete},
    {"dir",1,"List contents of remote directory",cmd_ls},
    {"disconnect",1,"Terminate ftp session",cmd_close},
    {"get",1,"Receive file",cmd_get},
    {"hash",0,"Toggle printing '#' for transferred buffers",cmd_hash},
    {"help",0,"Print local help information",cmd_help},
    {"image",1,"Set binary transfer mode",cmd_binary},
    {"lcd",0,"Change local working directory",cmd_lcd},
    {"lls",0,"List local files",cmd_lls},
    {"ls",1,"List contents of remote directory",cmd_ls},
    {"open",0,"Connect to remote site",cmd_open},
    {"passive",0,"Toggle passive mode",cmd_passive},
    {"put",1,"Send one file",cmd_put},
    {"pwd",1,"Print working directory on remote machine",cmd_pwd},
    {"quit",0,"Terminate ftp session and exit",cmd_quit},
    {"quote",1,"Send arbitary ftp command",cmd_quote},
    {"recv",1,"Receive file",cmd_get},
    {"rhelp",1,"Get help from remote server",cmd_rhelp},
    {"send",1,"Send one file",cmd_put},
    {"size",1,"Get size of remote file",cmd_size},
    {"user",1,"Send new user information",cmd_user},
    {"?",0,"Print local help informatiion",cmd_help},
    {NULL,0,NULL,NULL}
	
};

unsigned char 	passive;	/* Passive mode toggle */


int exec_cmd(int argc, char *argv[])
{
    struct _commands *cmd;
    int   i = 0;

    while ( commands[i].cmd ) {
	cmd = &commands[i];
	if ( strncmp(cmd->cmd,argv[0],3) == 0 ) {
	    if  (cmd->needconnect ) {
		if ( connected ) {
		    i = (*cmd->fn)(argc,argv);
		    return 0;
		} else {
		    printf("Not connected.\n");
		    return 0;
		}
	    } else {
		i = (*cmd->fn)(argc,argv);
		return 0;
	    }
	}
	++i;
    }
    printf("Command not understood\n");
    return 0;
}

int cmd_passive(int argc, char *argv[])
{
#if 1
    if ( argc != 1 )
	return -1;

    passive ^= 1;
    printf("Passive mode %s.\n", passive ? "on" : "off");
#else
    printf("Passive mode is not supported yet\n");
#endif
    return 0;
}

int cmd_hash(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;

    hash ^= 1;
    printf("Hash printing is %s.\n", hash ? "on (256 bytes/hash mark)" : "off");

    return 0;
}

int cmd_help(int argc, char *argv[])
{
    int	i;
    if ( argc == 1 ) {
	/* Print all available commands */
	i = 0;
	do {
	    printf("%s\t\t%s\n",commands[i].cmd,commands[i].help);
	    ++i;
	} while ( commands[i].cmd != NULL );

	
	return 0;
    } else if ( argc == 2 ) {
	/* Help on specific item */
	i = 0;
	do {
	    if ( strcmp(commands[i].cmd,argv[1]) == 0 ) {
		printf("%s\t\t%s\n",commands[i].cmd,commands[i].help);
		return 0;
	    }
	    ++i;
	} while ( commands[i].cmd != NULL );
    }
    return -1;
}

int cmd_binary(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    sprintf(buffer,"TYPE I"CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_ascii(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    sprintf(buffer,"TYPE A"CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_cdup(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    sprintf(buffer,"CDUP"CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_pwd(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    sprintf(buffer,"PWD"CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_rhelp(int argc, char *argv[])
{
    if ( argc > 2 )
	return -1;
    strcpy(buffer,"HELP");
    if ( argc == 2 ) {
	strcat(buffer," ");
	strcat(buffer,argv[1]);
    }
    strcat(buffer,CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_cd(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"CWD %s"CRLF,argv[1]);
    ftp_send(buffer);
    return 0;
}

int cmd_delete(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"DELE %s"CRLF,argv[1]);
    ftp_send(buffer);
    return 0;
}

int cmd_open(int argc, char *argv[])
{
    tcpport_t port = 21;
    int       ret;

    if ( connected ) {
	printf("Already connected\n");
	return 0;
    }

    if ( argc < 2 ) {
	printf("open hostname [port]\n");
	return 0;
    }

    if ( argc == 3 && ( port = atoi(argv[2] ) ) == 0 ) {
	printf("Invalid port specified\n");
	return -1;
    }
    if ( net_open_ctrl(argv[1],port) != - 1 ) {
	while ( 1 ) {
	    ret = ftp_returncode();
	    if ( ret == 220 ) 
		break;
	    if ( ret == -1 ) {
		net_close_ctrl();
		exit(0);
	    }
	}
	cmd_user(0,NULL);
    }

    return 0;
}

int cmd_close(int argc, char *argv[])
{
    net_close_ctrl();
    return 0;
}

int cmd_quit(int argc, char *argv[])
{
    if ( connected ) {
	sprintf(buffer,"QUIT"CRLF);
	ftp_send(buffer);
	net_close_ctrl();
    }
    exit(0);
}

int cmd_quote(int argc, char *argv[])
{
    int 	i;
    if ( argc == 1 )
	return -1;
    buffer[0] = 0;
    for ( i = 1 ; i < argc ; i++ ) {
	strcat(buffer,argv[i]);
	if ( i != argc - 1 )
	    strcat(buffer," ");
    }
    strcat(buffer,CRLF);
    ftp_send(buffer);
    return 0;
}

int cmd_user(int argc, char *argv[])
{
    char user[80];
    int  ret;
    if ( argc != 2 ) {
	printf("(username) "); fflush(stdout);
#ifdef SCCZ80
	fgets_net(user,sizeof(user),TRUE);
#else
	fgets(user,sizeof(user),stdin);
#endif
    } else {
	strncpy(user,argv[1],sizeof(user));
    }
    makeargs_rip(user);
    sprintf(buffer,"USER %s"CRLF,user);
    ret = ftp_send(buffer);
    if ( ret != 331 ) {
	return 0;
    }
    printf("Password: "); fflush(stdout);
    /* Need to cook terminal here */
#ifdef SCCZ80
	fgets_net(user,sizeof(user),FALSE);
#else
	fgets(user,sizeof(user),stdin);
#endif
    makeargs_rip(user);
    sprintf(buffer,"PASS %s"CRLF,user);
    ftp_send(buffer);
    return 0;
}
	


int cmd_size(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"SIZE %s"CRLF,argv[1]);
    ftp_send(buffer);
    return 0;
}

/* Local commands */
int cmd_shell(int argc, char *argv[])
{
    int  i;
    if ( argc == 1 ) {
	printf("Cannot escape to shell\n");
    }
    buffer[0] = 0;
    for ( i = 1; i < argc; i++ ) {
	strcat(buffer,argv[i]);
	strcat(buffer," ");
    }
    system(buffer);
    printf("\n");
}

int cmd_lls(int argc, char *argv[])
{
    sprintf(buffer,"ls");
    if ( argc == 2 ) {
	strcat(buffer," ");
	strcat(buffer,argv[1]);
    }
    system(buffer);
    printf("\n");
}

int cmd_lcd(int argc, char *argv[])
{
    if ( argc == 2 ) {
	sprintf(buffer,"cd %s",argv[1]);
	system(buffer);
    }
    printf("Local directory is ");
    system("pwd");
    printf("\n");
}
	




/* Data commands */
int cmd_ls(int argc, char *argv[])
{
    int  i;
    FILE *fp;

    sprintf(buffer,"LIST");
    if ( argc == 2 ) {
	strcat(buffer," ");
	strcat(buffer,argv[1]);
    }
    strcat(buffer,CRLF);
    ftp_data(buffer,RETR,stdout);
    return 0;	
}

int cmd_get(int argc, char *argv[])
{
    int  i;
    FILE *fp;

    if ( argc < 2 ) {
	return -1;
    }
    for ( i = 1 ; i < argc ; i++ ) {
	sprintf(buffer,"RETR %s"CRLF,argv[i]);
	if ( (fp = fopen(argv[i],"w") ) == NULL ) {
	    printf("Cannot open local file %s for writing\n",argv[i]);
	} else {
	    ftp_data(buffer,RETR,fp);
	    fclose(fp);
	}
    }
    return 0;	
}

int cmd_put(int argc, char *argv[])
{
    int  i;
    FILE *fp;

    if ( argc < 2 ) {
	return -1;
    }
    for ( i = 1 ; i < argc ; i++ ) {
	sprintf(buffer,"STOR %s"CRLF,argv[i]);
	if ( (fp = fopen(argv[i],"r") ) == NULL ) {
	    printf("Cannot open local file %s for reading\n",argv[i]);
	} else {
	    ftp_data(buffer,STOR,fp);
	    fclose(fp);
	}
    }
    return 0;	
}

