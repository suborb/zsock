/*
 *
 *      Telnet Application for ZSock
 *
 *      djm 26/2/99
 *
 *      djm 20/4/99 Modding to compile!
 *
 *      djm 28/4/99 Changing so that it reads from a buffer (that way
 *      we can be interrupted etc and is generally much nicer!
 *
 *      djm 14/12/99 Working!!!
 *
 *      djm 29/12/99 Rewritten to be standalone..not daemon..
 *
 *	djm 6/1/2000 Using the new sock_putc to make things nicer..
 *
 *	djm 7/1/2000 Line mode is now working by twitching on
 *		     SGA mode..nasty but it works....good for
 *		     telnetting to mail ports etc...
 *
 *	djm 9/1/2000 Ooops, removed the IAC check code, reinistated
 *
 *	djm 13/2/2000 Standalone..
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/inet.h>
#include <net/socket.h>
#include <net/resolv.h>
#include <net/misc.h>
#include <z88.h>


/*
 * Include in all of the BSD options stuff - save us defining them!
 */
#include <net/telnet.h>

#define NEEDOPT 1
#define NEEDOPT2 2

/* Temporary kludges.. */

/*
 * Max number of the options that we understand
 *
 * 34=linemode - probably necessary...
 */

#define MAXOPTS 4


/* Answer string for terminal enquire (this is for VT52) */
char answer[]= { 27,'/','Z'};

/*
 * Each telnet session has an associated structure
 */

struct telstr {
        SOCKET *s;
        u8_t options[MAXOPTS];
        u8_t optcomm;         /* What type of command did we get? */
        u8_t optflag;           /* How far are we in the process */
        u8_t status;		/* Negotiated opts etc */
        u8_t buffer[100];	/* Not used in char mode */
        u8_t bufcnt;		/* Ditto */
        u8_t *msg;		/* Any msg returned by GoTelnet() */
} t;

#define TELSTR struct telstr


/*
 * External Functions we access 
 */



int telnet_handler();
int telnet_loop();
void send_iac();

void SetTelOptions(u8_t x);



telnet()
{
        char    hostname[80];
        tcpport_t port;
        ipaddr_t  addr;
        u8_t    flag;


	nameapp("No conn");

	memset(t,0,sizeof(t));

        printf("Enter remote hostname/IP\n");
        fgets(hostname,79,stdin);
        addr=resolve(hostname);
        if (addr == NULL ) {
                        printf("Unknown host\n");
			goto out;
        }

	printf("\nEnter port to connect to (default=23)\n");
	fgets(hostname,79,stdin);
	if ( ( port=atoi(hostname)) == 0 ) port=23;


        inet_ntoa(addr,hostname);
        ResetTerm();
	t.options[TELOPT_ECHO]=1;
	t.options[TELOPT_SGA]=1;
        printf("Trying %s:%d...\n",hostname,port);
        t.s=sock_open(addr,port,0,prot_TCP);
        if (t.s == 0 ) {
                printf("Socket not created..memory?!?!?\n");
		goto out;
        }
/* Resize input buffer to 1k (from 512 bytes) */
//	tcp_recvresize(t->s,1024);

/* Name the application after the hostname */
	hostname[15]=0;
	nameapp(hostname);

        flag=1;
        while (flag) {
		GoTCP();	/* Sadly we're busy wait... */
                flag=GoTelnet(getk());
        }
        if (t.msg) printf("\n%s\n",t.msg);
	sock_shutdown(t.s);
out:
}

GoTelnet(x)
        u8_t  x;
{
	u8_t	rdbuffer[85];
	u16_t	len;
	u8_t	*ptr;
        SOCKET *s;

        s=t.s;
/*
 *      Check for unopen conn
 */
        if (sock_opened(s) == 0 ) {
		/* Check for ^C abort */
                if (x==3) {
			t.msg="Connect user aborted";
			return 0;    
		}
                return 1;
        }

/* Okay, socket has been opened, but has it been killed? */
        if (sock_closed(s) ) {
		if (t.status)
                	t.msg="Connection closed by foreign host";
		else
			t.msg="Connect timed out";
                return 0;
        }

/*
 * Okay, got to the open stage, after having checked for 
 * For aborted connection
 */

        if (t.status == 0 ) {
                t.status=1;
                printf("Connection established...\n");
        }

        if ( x == 29 ) {        /* ^] */
		t.msg="Connection closed";
                sock_close(s);
                return 0;
        }

/*
 * Do some printing!!
 */
        if  (len=sock_read(s,rdbuffer,80) ) {
                ptr=rdbuffer;
                while (len--)
                        telnet_print(*ptr++,1);
        } else sock_flush(s);

/*
 * Onto input...
 */

        if ( x ) {
/*
 * Oh, this is too kludgey for words..if SGA has been
 * suppressed we have negotiated options therefore act
 * like a vt52 terminal
 */
	  if (t.options[TELOPT_SGA]==0) {
                if (x>=0xFC) {
			sock_putc(27,s);
                        switch(x) {
                                case 0xFC:
                                        x='D';
                                        break;
                                case 0xFD:
                                        x='C';
                                        break;
                                case 0xFE:
                                        x='B';
                                        break;
                                case 0xFF:
                                        x='A';
                        }
                } else {
                        telnet_print(x,0);
                }
		sock_putc(x,s);
		sock_flush(s);
	   } else {
/*
 * On the otherhand it's still active, so enter linemode
 * kludgey but it seems to work..
 */
		u16_t	bufcnt;
		bufcnt=t.bufcnt;
		if (x == 127 ) {
			if (bufcnt) { --bufcnt; x=8; }
			else return 1;
		} else {
			t.buffer[bufcnt++]=x;
			if (x == 13 ) 
				t.buffer[bufcnt++]=10;
			if (x == 13 || bufcnt > 80 ) {
				sock_write(s,t.buffer,bufcnt);
				bufcnt=0;
				sock_flush(s);
			}
		}
		t.bufcnt=bufcnt;
                telnet_print(t,x,0);
	  }
        }
        return(1);
}




/*
 *      Print Routine for telnet
 */

telnet_print(x,flag)
        u8_t x;
        u8_t flag;
{
        if ( x == IAC ) { t.optflag |= NEEDOPT; return; }

        if ( t.optflag & NEEDOPT ) {
/* Check to see if valid option */
		if (x>=WILL && x<=DONT) {
                	t.optcomm=x;
                	t.optflag = NEEDOPT2;
                	return;
		}
		t.optflag=0;
        }

/* 
 * We've already received the WILL/WON't etc, so this is what we should do
 */

        if ( t.optflag & NEEDOPT2 ) {
/* Turn the flag off */
                t.optflag = 0;
                SetTelOptions(x);
                return;
        }

        if ( flag || t.options[TELOPT_ECHO] ) {
/* Finally, we're getting to the printing! */
/* If status == 2 then we've negotiated options so use terminal */
		if (t.status == 2 ) {
                	if (PrintVT52(x) ) 
                		/* Reply string */
                        	sock_write(t.s,answer,3);
		} else
			if (x != 10 ) putchar(x);
        }
}

/*
 * This is where we try to set options and send back appropriately
 */

void SetTelOptions(x)
u8_t    x;
{
        u8_t    reply;
        u8_t    flag;
        u8_t    *ptr;

        if ( x < MAXOPTS ) {
/*
 * Okay, it's one we can handle!
 */
                ptr=&t.options[x];	/* Barf that compiler! */
		t.status=2;		/* Negotiated opts */

                switch (t.optcomm) {
                        case WILL:
                                if (*ptr ) {
                                        *ptr=0;
                                        send_iac(DO,x);  /* Please do */
                                }
                                break;
                        case WONT:      /* Host won't */
                                if (*ptr==0) {  /* I wasn't doing it */
                                        *ptr=1; /* Am now! */
                                        send_iac(DONT,x);
                                }
                                break;
                        case DO:
                                if (*ptr==0) {  /* I wasn't doing it */
                                        *ptr=1; /* Am now! */
                                        send_iac(WILL,x);
                                }
                                break;
                        case DONT:
                                if (*ptr) {  /* I was doing it */
                                        *ptr=0; /* I'll stop then */
                                        send_iac(WONT,x);
                                }
                }
        } else {
/*
 * Deal with options we don't understand...we're a bit thick and only
 * understand binary and echo...
 */
                switch(t.optcomm) {
                        case WILL:
                                send_iac(DONT,x);        /* Please don't */
                                break;
                        case DO:
                                send_iac(WONT,x);       /* I won't */
                                send_iac(DONT,x);       /* You neither */
                                break;
                        case DONT:
                                send_iac(WONT,x);       /* I won't */
                                break;
                        case WONT:                      
                                break;                  /* Good! */
                }
        }
                        
}

void send_iac(act,code)
u8_t act,code;
{
        u8_t buf[3];
        buf[0]=IAC;
        buf[1]=act;
        buf[2]=code;
        sock_write(t.s,buf,3);
}


