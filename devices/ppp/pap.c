/*
	ZSock PAP authentication

	djm 14/3/2001

	(Reference to the minix PAP was made)

*/

#define FDSTDIO 1
#include "ppp.h"
#include "dll.h"
#include "hldc.h"
#include <net/hton.h>
#include <stdio.h>
#include <string.h>   

int auth_last_snd_seq;
int auth_last_rcv_seq;
char auth_username[32];
char auth_password[32];

void pap_init()
{
	auth_last_snd_seq = 0;   /* last sequence send */
	auth_last_rcv_seq = 0;   /* last sequence received */
	/* Have to read a file to get username/password */
}


void pap_send_options()
{
	int    i,l,len;
	UBYTE *into,*orig;

   	i = strlen(auth_username);
   	l = strlen(auth_password);

	len = i+l+2;
	into = orig = ppp_sys_alloc_pkt(len+4);
	*into++ = LCP_CONFIG_REQUEST;
	*into++ = ++auth_last_snd_seq;
	*into++ = (UBYTE) htons(len);
	*into = (UBYTE) len;
	memcpy(&orig[4],auth_username,i);
	i+=4;
	orig[i] = (UBYTE) l;
   	i++;  /* points to new */
	memcpy(&orig[i],auth_password,i);
	hldc_queue(orig,len,PPP_DLL_AUTH_PAP);
}
