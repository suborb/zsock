/*
	ZSock PAP authentication

	djm 14/3/2001

	(Reference to the minix PAP was made)

*/

#define FDSTDIO 1
#include <stdio.h>
#include <string.h>   
#include "ppp.h"
#include "dll.h"
#include "hldc.h"
#ifdef SCCZ80
#include <net/hton.h>
#else
#include "net/hton.h"
#endif

#ifdef PAP
int auth_last_snd_seq;
int auth_last_rcv_seq;
char auth_username[32];
char auth_password[32];

#define PAP_TYPE_CONFIGURE_REQ              1

void pap_init()
{
	auth_last_snd_seq = 0;   /* last sequence send */
	auth_last_rcv_seq = 0;   /* last sequence received */
	/* Have to read a file to get username/password */
	strcpy(auth_username,"z88");
	strcpy(auth_password,"z882");
}


void pap_send_options()
{
	int    i,l,len;
	UBYTE *into,*orig;

   	i = strlen(auth_username);
   	l = strlen(auth_password);

	len = i + l + 6;
	into = orig = ppp_sys_alloc_pkt(len+4);
	*into++ = PAP_TYPE_CONFIGURE_REQ;
	*into++ = ++auth_last_snd_seq;
	*into++ = (UBYTE) (len >> 8);
	*into++ = (UBYTE) (len & 0xFF);
	*into = i;
	memcpy(&orig[5],auth_username,i);
	i += 5;
	orig[i] = (UBYTE) l;
   	i++;  /* points to new */
	memcpy(&orig[i],auth_password,l);
	hldc_queue(orig,len,PPP_DLL_AUTH_PAP);
}

#endif
