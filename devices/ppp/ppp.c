
#define FDSTDIO 1

#include "ppp.h"
#include "dll.h"
#include "errorval.h"
#include "hldc.h"
#include "lcp_options.h"
#include <stdlib.h>
#include <net/hton.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <net/device.h>

int ppp_status();	/* Status function */
int null_fn();		/* Nowt */

struct pktdrive z88ppp = {
	"ZS0PKTDRV",
	"PPP",
	"ZSock PPP device by Michael Hope/Dominic Morris",
	ppp_init,
	ppp_send,
	hldc_byte_out,
	hldc_poll_in,
	ppp_open,
	ppp_close,
	ppp_status
};

/* We piggy back ontop of the peers state machine which simplifies the # states
   greatly */

UBYTE ppp_state;

#define PPP_STATE_DOWN		0
#define PPP_STATE_LCP		1
#define PPP_STATE_LCP_SENT	2
#define PPP_STATE_PAP		3
#define PPP_STATE_PAP_SENT	4
#define PPP_STATE_IPCP		10
#define PPP_STATE_IPCP_SENT	11
#define PPP_STATE_UP		20
#define PPP_STATE_TERM_SENT	30
#define PPP_STATE_CLOSED	40

#define PPP_LCP_DEFAULT_OPTIONS_LEN		20

const UBYTE ppp_lcp_default_options[] = {
	LCP_CONFIG_REQUEST,
	0x01,						/* Sequence 1 */
	0, 20,						/* Length */
	LCP_CONFIG_ASYNCMAP,
	6,
	0, 0, 0, 0,					/* All characters good */
	LCP_CONFIG_MAGIC,
	6,
	0x12, 0x34, 0x56, 0x78,				/* 0x12345678 :) */
	LCP_CONFIG_PROTOCOL_COMPRESSION,		/* Enable protocol compression */
	2,
	LCP_CONFIG_ADDRESS_COMPRESSION,			/* Enable protocol compression */
	2
};

#define PPP_IPCP_DEFAULT_OPTIONS_LEN	6

const UBYTE ppp_ipcp_default_options[] = {
	LCP_CONFIG_REQUEST,
	0x01,
	0, 10,						/* Length */
	IPCP_IP_ADDRESS,
	6						/* Remainder is in ppp_ipcp_ip_address */
};


UBYTE ppp_ipcp_ip_address[] = {
    0, 0, 0, 0
};

#define PPP_MAGIC		0x12345678
UBYTE ppp_magic[] = {
    0x12, 0x34, 0x56, 0x78
};

#ifndef SCCZ80
const char *ppp_lcp_options_names[] = {
	"00", "MRU", "asyncmap", "auth", "quality", "magic", "06", "pcomp", "acomp"};

const char *ppp_ipcp_options_names[] = {
	"00", "01", "02", "address", "04", "05", "06"};
#endif

void lcp_reply(UWORD dll_type, UBYTE ident, UBYTE *data, UBYTE data_length)
{
    UBYTE *into;
    mlcp_header *lcp;

    into = ppp_sys_alloc_pkt(2+sizeof(mlcp_header));

    *(into++) = PPP_DLL_LCP>>8;
    *(into++) = PPP_DLL_LCP&0xff;

    lcp = (mlcp_header *)into;
    lcp->code = dll_type;
    lcp->ident = ident;
    lcp->length = htons(sizeof(mlcp_header) + data_length);

    /* Move on to the data field */
    into += sizeof(mlcp_header);

    memcpy(into, data, data_length);

    hldc_queue(into, sizeof(mlcp_header) + data_length, FALSE);
}

int ppp_init(void)
{
#ifndef SCCZ80
    serial_init();
#endif
    ppp_state = PPP_STATE_LCP;
    hldc_init();
    return PPP_OVERHEAD;
}

int ppp_state_machine(UBYTE *packet, UWORD len)
{
    UBYTE	*into;
    UWORD	dll;
    mlcp_header *lcp;
    mlcp_option *option;
    mlcp_options options;
    int          tempopt;

  

    /* Comms link up? */
    if (ppp_state == PPP_STATE_DOWN)
	return EFAIL;
    if (len!=0) {
	/* Get the PPP DLL */
	dll = *(packet++);
	len--;
	if (!(dll&1)) {
	    dll =  ( dll << 8 ) | *(packet++) ;
	    len--;
	}
#ifdef DEBUG_PPP_STATE_MACHINE
	printf("ppp_state_machine: Recieved packet with DLL %04x.\n", dll);
#endif
	switch (dll) {
	case PPP_DLL_LCP:
	    lcp = (mlcp_header *)packet;
	    switch (lcp->code) {
		/* Shouldnt realy respond to echo requests in all states, but will in any case */
	    case LCP_ECHO_REQUEST:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: Recieved LCP echo request.\n");
#endif
		/* A reply uses our magic number to detect a loopback */
		lcp_reply(LCP_ECHO_REPLY, lcp->ident, ppp_magic, 4);
		break;
	    case LCP_TERMINATE_REQUEST:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: Recieved terminate request.\n");
#endif
		/* Reply... */
		lcp_reply(LCP_TERMINATE_ACK, lcp->ident, NULL, 0);
		ppp_state = PPP_STATE_CLOSED;
		break;
	    case LCP_CONFIG_REQUEST:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: Recieved configure request.\n");
#endif
		/* Setup the options parser */
		lcp_options_init(&options, packet);
		while ((option = lcp_options_next(&options))) {
#ifdef DEBUG_PPP_STATE_MACHINE_CONFIG
		    printf("\tlcp option type %u - %s\n", option->type, ppp_lcp_options_names[option->type]);
#endif
		    tempopt = hldc_get_options();
		    switch (option->type) {
		    case LCP_CONFIG_ASYNCMAP:
			/* Accept it */
			lcp_options_accept(&options, option);
			break;
#ifdef PAP
		    case LCP_CONFIG_AUTH:
			if ( option->data == htons(PPP_DLL_AUTH_PAP) ) 
			     lcp_options_accept(&options, option);
			else
			    lcp_options_reject(&options, option);
			break;
#endif
		    case LCP_CONFIG_MAGIC:
			/* Accept it */
			lcp_options_accept(&options, option);
			break;
		    case LCP_CONFIG_PROTOCOL_COMPRESSION:
			/* Accept it */
			lcp_options_accept(&options, option);
			tempopt |= HLDC_RECV_COMPRESS_PROTO;
			break;
		    case LCP_CONFIG_ADDRESS_COMPRESSION:
			/* Accept it */
			lcp_options_accept(&options, option);
			tempopt |= HLDC_RECV_COMPRESS_HEADER;
			break;
		    default:
			/* Reject */
			lcp_options_reject(&options, option);
			break;
		    }
		}
		/* Now send my options - if reply with positives only */
		if ( lcp_options_reply(&options,PPP_DLL_LCP) == 0 ) {
#ifndef SCCZ80
		    printf("Sending my default options\n");
#endif
		    hldc_set_options(tempopt);
		}
		into = ppp_sys_alloc_pkt(PPP_LCP_DEFAULT_OPTIONS_LEN);
		memcpy(into, ppp_lcp_default_options, PPP_LCP_DEFAULT_OPTIONS_LEN);
		hldc_queue(into,PPP_LCP_DEFAULT_OPTIONS_LEN,PPP_DLL_LCP);
		ppp_state = PPP_STATE_LCP_SENT;	    
		break;
	    case LCP_CONFIG_ACK:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: Config options accepted.\n");
#endif
		/* Turn on protocol compression */
		hldc_set_options(hldc_get_options() | HLDC_SEND_COMPRESS_PROTO | HLDC_SEND_COMPRESS_HEADER);

		/* Switch to IPCP mode */
		if (ppp_state == PPP_STATE_LCP_SENT)
#ifdef PAP
		    pap_send_options();
		    ppp_state = PPP_STATE_PAP_SENT;
#else
		    ppp_state = PPP_STATE_IPCP;
#endif
		break;
	    case LCP_CONFIG_REJECT:
	    case LCP_CONFIG_NAK:
#if 0
#ifndef SILENT
		printf("ppp_state_machine: panic - peer rejected some options.\n");
#endif
#endif
		break;
	    }
	    break;
#ifdef PAP
        case PPP_DLL_AUTH_PAP:
	/* Same header and value as always */
		lcp_options_init(&options,packet);
		switch (options.header->code) {
		case LCP_CONFIG_ACK:
			ppp_state = PPP_STATE_IPCP;
			break;
		case LCP_CONFIG_NAK:
			/* Send again..till timeout */
			ppp_state = PPP_STATE_PAP_SENT;
			pap_send_options();
			break;
		}
	break;
#endif
	case PPP_DLL_IPCP:
				/* IPCP uses the same header as lcp */
	    /* Prepare to scan */
	    lcp_options_init(&options, packet);
	    switch (options.header->code) {
	    case LCP_CONFIG_REQUEST:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: Recieved ipcp configure request.\n");
#endif
		while ((option = lcp_options_next(&options))) {
#ifdef DEBUG_PPP_STATE_MACHINE_CONFIG
		    printf("\tipcp option type %u - %s\n", option->type, ppp_ipcp_options_names[option->type]);
#endif
		    switch (option->type) {
		    case IPCP_IP_ADDRESS:
			/* Accept it */
			lcp_options_accept(&options, option);
			/* Should set ip address here */
			SetHostAddr(* ((long *)&option->data));
			break;
		    default:
			/* Reject */
			lcp_options_reject(&options, option);
		    }
		}
		lcp_options_reply(&options, PPP_DLL_IPCP);

		/* Now send my options */
		if (ppp_state == PPP_STATE_IPCP) {
		    into = ppp_sys_alloc_pkt(PPP_IPCP_DEFAULT_OPTIONS_LEN+4);
		    memcpy(into, ppp_ipcp_default_options, PPP_IPCP_DEFAULT_OPTIONS_LEN);
		    memcpy(into + PPP_IPCP_DEFAULT_OPTIONS_LEN, ppp_ipcp_ip_address, 4);
		    hldc_queue(into,PPP_IPCP_DEFAULT_OPTIONS_LEN+4, PPP_DLL_IPCP );
		    ppp_state = PPP_STATE_IPCP_SENT;
		}
		break;
	    case LCP_CONFIG_ACK:
#ifdef DEBUG_PPP_STATE_MACHINE
		printf("ppp_state_machine: IPCP options accepted.\n");
#endif
		ppp_state = PPP_STATE_UP;
		break;
	    case LCP_CONFIG_NAK:
		/* Should have our IP address in it */
		while ((option = lcp_options_next(&options))) {
#ifdef DEBUG_PPP_STATE_MACHINE
		    printf("ppp_state_machine: ipcp option naked type %u - %s\n", option->type, ppp_ipcp_options_names[option->type]);
#endif
		    switch (option->type) {
		    case IPCP_IP_ADDRESS:
			/* Accept it */
			memcpy(ppp_ipcp_ip_address, &(option->data), 4);
			SetHostAddr(* ((long *)&option->data));
			break;
		    default:
#if 0
#ifndef SILENT
			printf("ppp_state_machine: panic: other options NAKED!\n");
#endif
#endif
			break;
		    }
		}
		/* Send a new request packet */
		into = ppp_sys_alloc_pkt(PPP_IPCP_DEFAULT_OPTIONS_LEN + 4);
		memcpy(into, ppp_ipcp_default_options, PPP_IPCP_DEFAULT_OPTIONS_LEN);
		memcpy(into + PPP_IPCP_DEFAULT_OPTIONS_LEN, ppp_ipcp_ip_address, 4);
		hldc_queue(into,PPP_IPCP_DEFAULT_OPTIONS_LEN + 4, PPP_DLL_IPCP);
		ppp_state = PPP_STATE_IPCP_SENT;
		break;
	    }
	    break;
	default:
				/* Reject the protocol */
#ifdef DEBUG_PPP_STATE_MACHINE
	    printf("ppp_state_machine: rejecting protocol %04X\n", dll);
#endif
	    dll = htons(dll);
	    lcp_reply(LCP_PROTOCOL_REJECT, 1, (UBYTE *)&dll, 2);
	}
    } /* len == 0 */
    return EOK;
}
		
int ppp_open(void)
{
	UBYTE *packet;
	UWORD len;
	time_t	timeout;

#ifdef PAP
	pap_init();
#endif
	timeout = time(NULL)+30;
	
	/* Wait for a packet */
	while ( (time(NULL)<timeout) && (ppp_state!=PPP_STATE_UP)) {
	    do {
		len = hldc_loop(&packet);
	    } while ( len == 0 && time(NULL) < timeout );

	    if (packet)
		ppp_state_machine(packet, len);
	}
	if (ppp_state==PPP_STATE_UP)
		return EOK;
	return EFAIL;
}

int ppp_close(void)
{
    /* Do things properly */
    BOOL ack_received;
    UBYTE *packet;
    UWORD len;
    UWORD dll;
    time_t	timeout;
    
    timeout = time(NULL)+30;

    ack_received = FALSE;
    
    /* Wait for a packet */
    while ((time(NULL)<timeout)&&(!ack_received)) {
	/* Send a terminate request */
	lcp_reply(LCP_TERMINATE_REQUEST, 1, NULL, 0);

	while (((len=hldc_loop(&packet))==0)&&(time(NULL)<timeout));
	
	if (packet) {
	    /* Is it an ACK? */
	    /* Get the PPP DLL */
	    dll = *(packet++);
	    len--;
	    if (!(dll&1)) {
		dll = dll << 8 | *(packet++);
		len--;
	    }
	    if (dll == PPP_DLL_LCP) {
		if ((*packet) == LCP_TERMINATE_ACK) {
		    ack_received = TRUE;
		}
	    }
	    else {
		/* Else, send again */
		lcp_reply(LCP_TERMINATE_REQUEST, 1, NULL, 0);
	    }
	}
    }

    return EOK;
}


UWORD ppp_byte_in(void **pkt)
{
	UBYTE	*ret, *packet;
	UWORD	dll, len,rlen;
	if ( (len=hldc_byte_in((void *)&packet)) != 0) {
		rlen = len;
		ret = packet;
		dll = *(ret++); rlen--;
		if (!(dll&1)) {
			dll = dll << 8 | *(ret++);
			rlen--;
		}
#ifdef DEBUG_PPP_POLL
		printf("ppp_poll: packet dll of %d\n", dll);
#endif
		if (dll==PPP_DLL_IP) {
			*pkt = ret;
			return (rlen);
		}
		else
			ppp_state_machine(packet, len);
	}
	return 0;
}

int ppp_send(void *pkt, UWORD len)
{
	return hldc_queue(pkt, len, PPP_DLL_IP);
}

int null_fn()
{
	return 0;
}

int ppp_status()
{
	if (ppp_state == PPP_STATE_UP) return 1;
	return 0;
}
