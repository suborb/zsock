/*
  lcp_options.h 

  Prototypes for a simple LCP style options block parser
  Much more computationally expensive than before, but cleaner
*/

#ifndef __OS_LCP_OPTIONS_H
#define __OS_LCP_OPTIONS_H

#include "types.h"

/* Max size to reply to...prob enough! */
#define MAX_LCP_REPLY_SIZE 128

struct slcp_option {
	UBYTE	type;		/* One of LCP_CONFIG_* */
	UBYTE	length;		/* Of this option including header */
/*	union ulcp_options_data {
		UWORD	mru;
		UWORD	auth_protocol;
		UWORD	quality_protocol;
		UDWORD	magic;
	} u;*/
	UWORD	data;
};

typedef struct slcp_option mlcp_option;

struct slcp_header {
	UBYTE	code;
	UBYTE	ident;		/* Like sqeuence - increase on new data */
	UWORD	length;		/* Length of packet including header */
};

typedef struct slcp_header mlcp_header;

struct slcp_options {
    mlcp_header *header;
    UBYTE	*packet;
    UBYTE	*scan;
    UWORD	left;
    UBYTE	reject[60];
    UBYTE	accept[60];
    int		reject_end;
    int		accept_end;
};

typedef struct slcp_options mlcp_options;

/* Init a new options struct. packet points to the lcp options header */
extern int lcp_options_init(mlcp_options *options, UBYTE *packet);
/* Get the next option from a struct - NULL on no more options */
extern mlcp_option *lcp_options_next(mlcp_options *options);
/* Reject the given option */
extern int lcp_options_reject(mlcp_options *options, mlcp_option *rejected);
/* Accept the given option */
extern int lcp_options_accept(mlcp_options *options, mlcp_option *accepted);
/* Add a lcp options reply to the given buffer.  Returns length of added data */
extern int lcp_options_reply(mlcp_options *options, UWORD dll_type);

#endif /* __OS_LCP_OPTIONS_H */
