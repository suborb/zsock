
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
int cmd_help(int argc, char *argv[]);
int cmd_binary(int argc, char *argv[]);
int cmd_open(int argc, char *argv[]);
int cmd_passive(int argc, char *argv[]);
int cmd_quote(int argc, char *argv[]);
int cmd_size(int argc, char *argv[]);
int cmd_pwd(int argc, char *argv[]);
int cmd_rhelp(int argc, char *argv[]);
int cmd_user(int argc, char *argv[]);
/* Data commands */
int cmd_put(int argc, char *argv[]);
int cmd_get(int argc, char *argv[]);
int cmd_ls(int argc, char *argv[]);

struct _commands commands[] = {
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
	{"help",0,"Print local help informatiion",cmd_help},
	{"image",1,"Set binary transfer mode",cmd_binary},
	{"ls",1,"List contents of remote directory",cmd_ls},
	{"open",0,"Connect to remote site",cmd_open},
	{"passive",0,"Toggle passive mode",cmd_passive},
	{"put",1,"Send one file",cmd_put},
	{"pwd",1,"Print working directory on remote machine",cmd_pwd},
	{"quit",0,"Terminate ftp session and exit",cmd_quit},
	{"quote",1,"Send arbitary ftp command",cmd_quote},
	{"recv",1,"Receive file",cmd_get},
	{"rhelp",1,"Get help from remotet server",cmd_rhelp},
	{"send",1,"Send one file",cmd_put},
	{"size",1,"Get size of remote file",cmd_size},
	{"user",1,"Send new user information",cmd_user},
	{"?",0,"Print local help informatiion",cmd_help},
	{NULL,0,NULL,NULL}
	
};

unsigned char 	passive;	/* Passive mode toggle */

char buffer[160];               /* Command buffer */


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
#if 0
    if ( argc != 1 )
	return -1;

    passive ^= 1;
    printf("Passive mode is %s\n", passive ? "ON" : "OFF");
#else
    printf("Passive mode is not supported yet\n");
#endif
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
    ftp_send("TYPE I\r\n");
    return 0;
}

int cmd_ascii(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    ftp_send("TYPE A\r\n");
    return 0;
}

int cmd_cdup(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    ftp_send("CDUP\r\n");
    return 0;
}

int cmd_pwd(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    ftp_send("PWD\r\n");
    return 0;
}

int cmd_rhelp(int argc, char *argv[])
{
    if ( argc != 1 )
	return -1;
    ftp_send("HELP\r\n");
    return 0;
}

int cmd_cd(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"CWD %s\r\n",argv[1]);
    ftp_send(buffer);
    return 0;
}

int cmd_delete(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"DELE %s\r\n",argv[1]);
    ftp_send(buffer);
    return 0;
}

int cmd_open(int argc, char *argv[])
{
    tcpport_t port = 21;

    if ( argc == 3 && ( port = atoi(argv[2] ) ) == 0 ) {
	printf("Invalid port specified\n");
	return -1;
    }
    if ( net_open_ctrl(argv[1],port) != - 1 ) {
	while ( ftp_returncode() != 220 )
	    ;;
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
    ftp_send("QUIT\r\n");
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
    strcat(buffer,"\r\n");
    ftp_send(buffer);
    return 0;
}

int cmd_user(int argc, char *argv[])
{
    char user[80];
    int  ret;
    if ( argc != 2 ) {
	printf("(username) "); fflush(stdout);
	fgets(user,sizeof(user),stdin);
    } else {
	strncpy(user,argv[1],sizeof(user));
    }
    makeargs_rip(user);
    sprintf(buffer,"USER %s\r\n",user);
    ret = ftp_send(buffer);
    if ( ret != 331 ) {
	printf("Bad returncode\n");
	return 0;
    }
    printf("Password: "); fflush(stdout);
    /* Need to cook terminal here */
    fgets(user,sizeof(user),stdin);
    makeargs_rip(user);
    sprintf(buffer,"PASS %s\r\n",user);
    ftp_send(buffer);
    return 0;
}
	


int cmd_size(int argc, char *argv[])
{
    if ( argc != 2 )
	return -1;
    sprintf(buffer,"SIZE %s\r\n",argv[1]);
    ftp_send(buffer);
    return 0;
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
    strcat(buffer,"\r\n");
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
	sprintf(buffer,"RETR %s\r\n",argv[i]);
	fp = fopen(argv[i],"w");
	ftp_data(buffer,RETR,fp);
	fclose(fp);
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
	sprintf(buffer,"STOR %s\r\n",argv[i]);
	fp = fopen(argv[i],"r");
	ftp_data(buffer,STOR,fp);
	fclose(fp);
    }
    return 0;	
}

