/*
  lcp_options.c

  Prototypes for a simple LCP style options block parser
  Much more computationally expensive than before, but cleaner
*/

#define FDSTDIO 1
#include "lcp_options.h"
#include "dll.h"
#include "errorval.h"
#include "hldc.h"
#include "ppp.h"
#include <net/hton.h>
#include <string.h>
#include <stdio.h>

int lcp_options_init(mlcp_options *options, UBYTE *packet)
{
    /* Cant do any error checking (sigh) */
    options->header = (mlcp_header *)packet;
    options->packet = packet;
    options->scan = packet + sizeof(mlcp_header);
    options->left = ntohs(options->header->length) - sizeof(mlcp_header);

    options->reject_end = 0;
    options->accept_end = 0;

    return EOK;
}

mlcp_option *lcp_options_next(mlcp_options *options)
{
    mlcp_option *ret;

    /* See if theres any data remaining */
    if (options->left == 0) {
	/* Nope */
	return NULL;
    }

    /* Return the current one and decrease the length */
    ret = (mlcp_option *)(options->scan);
    options->left -= ret->length;
    options->scan += ret->length;

    return ret;
}

int lcp_options_reject(mlcp_options *options, mlcp_option *rejected)
{
    memcpy(&options->reject[options->reject_end], rejected, rejected->length);
    options->reject_end += rejected->length;

    return EOK;
}

int lcp_options_accept(mlcp_options *options, mlcp_option *accepted)
{
    memcpy(&options->accept[options->accept_end], accepted, accepted->length);
    options->accept_end += accepted->length;

    return EOK;
}

int lcp_options_reply(mlcp_options *options, UWORD dll_type)
{
    UBYTE *buffer;
    /* DLL length plus header */
    UWORD len = sizeof(mlcp_header);

    buffer = ppp_sys_alloc_pkt(MAX_LCP_REPLY_SIZE);

#if 0
    /* Put in the appropriate header */
    *(buffer++) = dll_type >> 8;
    *(buffer++) = dll_type & 0xFF;
#endif

    /* Anything rejected? */
    if (options->reject_end != 0) {
	/* Yip - send that */
	((mlcp_header *)buffer)->code = LCP_CONFIG_REJECT;
	len += options->reject_end;
	memcpy(buffer + sizeof(mlcp_header), options->reject, options->reject_end);
    }
    else {
	/* No rejects - reply with the accepted data */
	((mlcp_header *)buffer)->code = LCP_CONFIG_ACK;
	len += options->accept_end;
	memcpy(buffer + sizeof(mlcp_header), options->accept, options->accept_end);
    }

    /* Fill in the rest of the header */
    ((mlcp_header *)buffer)->ident = options->header->ident;
    /* Doesnt include the DLL type */
    ((mlcp_header *)buffer)->length = htons(len);
    
    return hldc_queue(buffer,len,dll_type);
}