/*
 *	Short chat program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef Z88
#include <z88.h>
#include <net/misc.h>
#endif

#define SERIAL 1


#define MAX_ABORT 10

extern int cmd_abort(char *what);
extern int cmd_timeout(char *what);
extern int cmd_say(char *what);
extern int cmd_expect(char *what);

#ifdef __Z88__
extern void  __FASTCALL__ raw_out(char );
#else
extern void raw_out(char );
#endif
extern int raw_in();

#ifdef SMALL_C
#define HPSIZE 2048
HEAPSIZE(HPSIZE)
#endif

static char *rip (char *s);

struct cmds {
	char *cmd;
	int (*fn)();
};

struct cmds commands[] = {
	{"ABORT",cmd_abort},
	{"TIMEOUT",cmd_timeout},
	{"SAY",cmd_say},
	{"EXPECT",cmd_expect},
	{0,0}
};

char *abort_on[MAX_ABORT];
int   abortnum;

time_t	timeouttime;

int chat_init(char *filename)
{
	char buffer[80],*ptr;
	struct cmds *cmdptr;
	int ret,i;
	FILE *fp;
	abortnum=0;

	if ( (fp=fopen(filename,"r")) == NULL ) {
		printf("Chat file %s not found\n",filename);
		return -1;
	}
	while (fgets(buffer,80,fp) != NULL ) {
		ptr=strchr(buffer,' ');
		if (ptr) {
			*ptr=0;
			cmdptr=commands;
			while ( cmdptr->cmd ) {
				if (strcmp(cmdptr->cmd,buffer) == 0 ) {
					ptr=strchr(ptr+1,'"');
					if ( ptr == NULL) {
						break;
					}
					ret=(*cmdptr->fn)(ptr+1);
					if (ret == 1) break;
					else {
						ret=EOF;
						goto cleanup;
					}
				} else
					cmdptr++;
			}
		}
next:
	}
	ret = 0;
cleanup:
	for (i=0; i<MAX_ABORT; i++)
		free(abort_on[i]);
	return ret;
}

int cmd_abort(char *what)
{
	if (abortnum+1 >= MAX_ABORT) return 0;
	rip(what);
	abort_on[abortnum++]=strdup(what);
	printf("Abort on: %s\n",what);
	return 1;
}

int cmd_say(char *what)
{
	comms_puts(what);
	rip(what);
	printf("Saying:    <%s>\n",what);
	return 1;
}

int cmd_timeout(char *what)
{
	int i;
	if ( (i=atoi(what)) != 0 ) {
		timeouttime = time((time_t *)0)+i;
	} else
		timeouttime = 0;
	return 1;
}


int cmd_expect(char *what)
{
	int i;
	char buffer[80];

	rip(what);
	printf("Expecting: <%s>\n",what);
	while ( 1 ) {
	  	if ( comms_gets(buffer,80) == 0 ) return 0;
		rip(buffer);
		if ( strlen(buffer) == 0 ) continue;
		printf("Received:  <%s>\n",buffer);
		for (i=0; i<abortnum; i++) {
			if (mystrncmp(buffer,abort_on[i],strlen(abort_on[i])) == 0 ) 
				return EOF;
		}
		if (mystrncmp(buffer,what,strlen(what))==0) {
			return 1;
		}
	}	
}

int mystrncmp(char *buffer,char *what, int len)
{
	char *start;

	start = buffer+strlen(buffer)-len;
	if (start < buffer ) {
		return 1;
	}

	do {
		if (strncmp(start,what,len) == 0) return 0;
		--start;
	} while (start >= buffer);
	return 1;
}

int comms_puts(unsigned char *what)
{
	while (*what)
		raw_out(*what++);
	raw_out('\n');
	raw_out('\r');
}

int comms_gets(unsigned char *buf,int size)
{
	int	i=0;
	int c;

	while (1) {
		c=raw_in();
		if (c && c!=EOF ) {
			if ( c=='\n' ||  c=='\r' )  {
				buf[i]=0;
				return 1;
			}
			if (i<size) {
				buf[i++]=c;
			}
		}
		if (timeouttime && time(NULL) > timeouttime) {
			printf("----Timeout----\n");
			return 0;
		}
	}
}



#ifdef SERIAL
void __FASTCALL__ raw_out(char char)
{
#asm
.loop
	INCLUDE "#serintfc.def"
	pop	hl
	push	hl
	ld	a,l
	ld	l,si_pbt
	ld	bc,0
	call_oz (os_si)
	jr	c,loop
#endasm
}
		
int raw_in()
{
#asm
.loop1
	ld	l,si_gbt
	ld	bc,5
	call_oz (os_si)
	ld	l,a
	ld	h,0
	ret	nc
	ld	l,0
#endasm
}
#else
void raw_out(char c)
{
	putc(c,stdout);
}
int raw_in()
{
	return (fgetc(stdin));
}
#endif

#ifdef SERIAL
void init_serial()
{
#asm
	ld	l,si_sft
	call_oz(os_si)
#endasm
}
#endif

#ifndef SMALL_C
int main(int argc, char *argv[])
#else
int chat()
#endif
{
	char buffer[81];
	int ret;
	char *filename;
	nameapp("Chat");
#ifdef SMALL_C
	if ( readmail(FILEMAIL,(far char *)buffer,80) == NULL) {
		filename = "script.cht";
	} else
		filename = buffer;

#else
	filename = argv[1];
#endif
start:
#ifdef SMALL_C
	heapinit(HPSIZE);
#endif
#ifdef SERIAL
	init_serial();
#endif
	ret = chat_init(filename);
	switch (ret) {
		case 0:
			printf("\b----CONNECT SUCCEEDED---\n");
			printf("Switching device online\n");
			DeviceOnline();
			return;
		case -1:
			printf("----CONNECT FAILED----\n");
			printf("Press ENTER to try again\n");
			ret = fgetc(stdin);
			if ( ret == '\n' ) goto start;
	}
}

char *rip (char *s)
{
        char *temp;
	while ( (temp=strchr(s,'\n')) != NULL)
		*temp=0;
	while ( (temp=strchr(s,'\r')) != NULL)
		*temp=0;
        return s;
}

