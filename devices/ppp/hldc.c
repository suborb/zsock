#define FDSTDIO 1

#include "ppp.h"



#include <stdio.h>
#include <net/hton.h>

#define PACKET_BUF_SIZE 1024

u8_t hldc_rx_buf[PACKET_BUF_SIZE];

#define HLDC_FLAG	0x7E
#define HLDC_CTRL_ESC	0x7D
#define HLDC_ESC_XOR	0x20

#define HLDC_ALL_STATIONS	0xFF
#define HLDC_PPP_CONTROL	0x03

#define HLDC_STATE_ESC	1
#define HLDC_STATE_NORM	2

#define HLDC_FCS_SEED	0xFFFF		/* Inital FSC value */
#define HLDC_FCS_GOOD	0xF0B8		/* Final FCS value */

/* hldc_flags: the various compress flags set in hldc.h */
u8_t hldc_flags;

u8_t hldc_flag_count;
u8_t hldc_state;
u8_t *hldc_rx_start;
u8_t *hldc_rx_pos;

/* Queue for sending stuff out */
queuehdr_t *sendq_last;

/* To make the computations later easier */
u16_t hldc_frame_len;	

/* This table is in assembler to get around lcc limitations on the Gameboy */
#ifndef GAMEBOY
/* Ripped straight from RFC 1662 */
const u16_t hldc_fcs_table[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};
#else
extern const u16_t hldc_fcs_table[];
#endif

/* Various options for sending byte by byte */

unsigned char hldc_sending;
unsigned char hldc_send_flags;
unsigned char hldc_send_state;
unsigned char hldc_send_esc_char;
unsigned int  length_left;
unsigned char *cur_pos;
queuehdr_t   *cur_pkt;

#define HLDC_SEND_FLAGS	0
#define HLDC_SEND_HEAD  1
#define HLDC_SEND_DLL	2
#define HLDC_SEND_BODY  3
#define HLDC_SEND_CKSUM 4
#define HLDC_SEND_ESC   5

#define FIRST_BYTE    0
#define SECOND_BYTE   1
#define THIRD_BYTE    2
#define INITIAL_FLAG  0
#define TRAILING_FLAG 1



/* Set the options (protocol compress etc */
u8_t hldc_set_options(u8_t new_options)
{
    u8_t ret;

    ret = hldc_flags;
    hldc_flags = new_options;

    return ret;
}

u8_t hldc_get_options(void)
{
    return hldc_flags;
}

int hldc_init(void)
{
    sendq_last = 0;
    hldc_flag_count = 0;
    hldc_sending = 0;
    hldc_state = HLDC_STATE_NORM;
    hldc_rx_start = hldc_rx_buf;
    hldc_rx_pos = hldc_rx_buf;
    hldc_flags = 0;
    return EOK;
}


/*
 * Calculate a new fcs given the current fcs and the new data.
 */
u16_t hldc_fcs16_update(u16_t fcs, u8_t *data, u16_t len)
{
	while (len--)
		fcs = (fcs >> 8) ^ hldc_fcs_table[(fcs ^ *(data++)) & 0xff];
	return fcs;
}

/* Insert a packet into the send queue */

void hldc_queue(u8_t *pkt, u16_t len, u16_t dll)
{
    u8_t       *temp;
    u16_t       crc;
    queuehdr_t *queue;

    queue = (queuehdr_t *)(pkt-PPP_OVERHEAD);

    if ( sendq_last ) {
#ifndef SCCZ80
	printf("Adding length %d to queue\n",len);
#endif
	sendq_last->next = queue;
    }
    sendq_last = queue;

    queue->len = len + 2;
    temp = pkt-2;
    temp[0] = (dll >> 8) & 0xff;
    temp[1] = dll & 0xff;

    if ( ( temp[0] & 0x01 )  ) {
	++temp;
	++queue->len;
    }

  


    if ( cur_pkt == 0 ) {
	hldc_sending = 1;
	hldc_send_state = HLDC_SEND_FLAGS;
	hldc_send_flags = INITIAL_FLAG;
	cur_pkt = queue;
	length_left = cur_pkt->len;
	cur_pos = temp;
    }



#if 1
    if (hldc_flags&HLDC_SEND_COMPRESS_HEADER) 
	crc = hldc_fcs16_update(HLDC_FCS_SEED, temp, len+2);
    else 
	crc = hldc_fcs16_update(0x3DE3,temp, len+2); /* Static header so don't crc */
#else
	/* We say we've got a static header, but we do need to include the dll
	   in here as well..(
	 */
     crc = hldc_fcs16_update(0x3DE3,temp,len + 2); /* Add in dll one */
#endif

	
    crc ^= 0xFFFF;
    queue->crc = crc;
}
   

/* Send a byte out over ppp */
 
void *hldc_byte_out()
{
	if ( hldc_sending == 0 )
		return NULL;

	switch (hldc_send_state) {
		case HLDC_SEND_BODY:
			return (hldc_byte_send_body());
		case HLDC_SEND_ESC:
			return (hldc_byte_send_esc());
		case HLDC_SEND_FLAGS:
			return (hldc_byte_send_flags());
		case HLDC_SEND_HEAD:
			return (hldc_byte_send_head());         
		case HLDC_SEND_CKSUM:
			return (hldc_byte_send_cksum());
	}
	return NULL;
}
			
void *hldc_byte_send_flags()
{
	switch(hldc_send_flags) {
	case INITIAL_FLAG:
		serial_out(HLDC_FLAG);
		iferror { 
			return NULL;
		} else {
			hldc_send_flags = FIRST_BYTE;
			hldc_send_state = HLDC_SEND_HEAD;
			return NULL;
		}
	case TRAILING_FLAG:
		serial_out(HLDC_FLAG);
		iferror {
			return NULL;	
		} else {
			/* Success pick up next packet */
			void *ptr = cur_pkt;
			cur_pkt = cur_pkt->next;
			if ( cur_pkt == 0 ) {
				sendq_last = 0;
				hldc_sending = 0;
				return NULL;
				return ptr;
			}
			length_left = cur_pkt->len;
			cur_pos = (void * ) cur_pkt + PPP_OVERHEAD - 2;
			hldc_send_flags = FIRST_BYTE;
			hldc_send_state = HLDC_SEND_HEAD;
			return NULL;
			return ptr;
		}
	}
	return NULL;
}

/* Send the header */
void *hldc_byte_send_head()
{
	if (hldc_flags&HLDC_SEND_COMPRESS_HEADER) {
		/* skip the header */
		hldc_send_state=HLDC_SEND_BODY;
		hldc_send_flags=FIRST_BYTE;
		return NULL;
	}
	switch (hldc_send_flags) {
	case FIRST_BYTE:
		serial_out(HLDC_ALL_STATIONS);
		iferror {
			return NULL;
		} else {
			hldc_send_flags++;
			return NULL;
		}
	case SECOND_BYTE:
		if (hldc_flags&HLDC_SEND_COMPRESS_PROTO) {
			serial_out(HLDC_PPP_CONTROL);
			iferror {
				return NULL;
			} else {
				hldc_send_flags=FIRST_BYTE;
				hldc_send_state=HLDC_SEND_BODY;
				return NULL;
			}
		} else {
			serial_out(HLDC_CTRL_ESC);
			iferror {
				return NULL;
			} else {
				hldc_send_flags++;
				return NULL;
			}
		}
	case THIRD_BYTE:
		serial_out(HLDC_PPP_CONTROL ^ HLDC_ESC_XOR);
		iferror {
			return NULL;
		} else {
			hldc_send_flags=FIRST_BYTE;
			hldc_send_state=HLDC_SEND_BODY;
			return NULL;
		}
	}
	return NULL;
}
		
		


		
	
void *hldc_byte_send_esc()
{
	switch (hldc_send_flags) {
		case SECOND_BYTE:
			serial_out(hldc_send_esc_char);
			iferror {
				return NULL;
			} else {
				hldc_send_state=HLDC_SEND_BODY;
				return NULL;
			}
		case FIRST_BYTE:
			serial_out(HLDC_CTRL_ESC);
			iferror {
				return NULL;
			} else {
				hldc_send_flags++;
			}
	}
	return NULL;
}

void *hldc_byte_send_cksum()
{
	switch (hldc_send_flags) {
	case FIRST_BYTE:
		serial_out(cur_pkt->crc);
		iferror {
			return NULL;
		} else {
			hldc_send_flags++;
			return NULL;
		}
	case SECOND_BYTE:
		serial_out(htons(cur_pkt->crc));
		iferror {
			return NULL;
		} else {
			hldc_send_state = HLDC_SEND_FLAGS;
			hldc_send_flags = TRAILING_FLAG;
			return NULL;
		}
	}
	return NULL;
}

/* This is where we actually send the body of the packet */

void *hldc_byte_send_body()
{
	u8_t  pad;
	u8_t  c;

	if (length_left == 0 ) {
		hldc_send_state = HLDC_SEND_CKSUM;
		hldc_send_flags = FIRST_BYTE;
		return (hldc_byte_send_cksum());
	}

	c = *cur_pos;
	switch ( c ) {
	case HLDC_FLAG:
	case HLDC_CTRL_ESC:
		hldc_send_esc_char = c ^ HLDC_CTRL_ESC;
		hldc_send_state = HLDC_SEND_ESC;
		serial_out(HLDC_CTRL_ESC);
		iferror {
			hldc_send_flags = FIRST_BYTE;
		} else {
			hldc_send_flags = SECOND_BYTE;
		}
		break;
	default:
		if ( hldc_flags&HLDC_SEND_COMPRESS_PROTO ) {
			serial_out(c);
			iferror {
				return NULL;
			} else {
				break;
			}
		} else if ( c < 0x20 ) {
			hldc_send_esc_char = c ^ HLDC_ESC_XOR;
			hldc_send_state = HLDC_SEND_ESC;
			serial_out(HLDC_CTRL_ESC);
			iferror {
				hldc_send_flags = FIRST_BYTE;
			} else {
				hldc_send_flags = SECOND_BYTE;
			}
		} else {
			serial_out(c);
			iferror {
				return NULL;
			}
		}
	}
	cur_pos++;
	length_left--;
	return NULL;
}


/* This loop is called when closing/opening the device, this way we don't 
   return to ZSock until we've either closed or opened, this is a mini loop
   that sends and receives bytes
 */

u16_t hldc_loop(void **ret)
{
	void	*value;
	if ( value = hldc_byte_out() ) 
	    ppp_sys_free_pkt(value);
	return (hldc_byte_in(ret));
}

u16_t hldc_poll_in(void **ret)
{
    u8_t *pkt;
    u16_t  len;
    u16_t  dll;

    if ( len = hldc_byte_in(&pkt) ) {
	dll = pkt[0];
	if (!(dll&1)) {
	    dll = dll << 8 | (pkt[1]);
	}

	if ( dll == PPP_DLL_IP ) {	
	    *ret = pkt+1;
	    return len-1;
	} else {
	    ppp_state_machine(pkt,len);
	}
    }
    return 0;
}
	


u16_t hldc_byte_in(void **ret)
{
    int   got;
    u16_t len;

#ifdef DEBUG_PACKET_POLL
    printf("hldc_poll: entered.\n");
#endif
    if( (got = serial_in() ) == -1 )
	return 0;
    switch (hldc_state) {
    case HLDC_STATE_NORM:
	switch (got) {
	case HLDC_FLAG:
	    hldc_flag_count++;
	    break;
	case HLDC_CTRL_ESC:
	    hldc_state = HLDC_STATE_ESC;
	    break;
	default:
	    if (hldc_flag_count==0) {
#ifdef DEBUG_HLDC_POLL_GOT
		printf("hldc_poll: dropped pre-frame data %02X\n", got);
#endif
	    }
	    else {
#ifdef DEBUG_HLDC_POLL_GOT
		printf("hldc_poll: got %02X\n", got);
#endif
		*(hldc_rx_pos++) = got;
		hldc_frame_len++;
	    }
	}
	break;
    case HLDC_STATE_ESC:
	got ^= HLDC_ESC_XOR;
#ifdef DEBUG_HLDC_POLL_GOT
	printf("hldc_poll: got %02X\n", got);
#endif
	*(hldc_rx_pos++) = got;
	hldc_frame_len++;
	hldc_state = HLDC_STATE_NORM;
	break;
    default:
#if 0
#ifndef SILENT
	printf("hldc_poll: error in state value.\n");
#endif
#endif
    }
    
    if (hldc_flag_count==2) {
	/* Recieved a frame - check it */
	hldc_flag_count = 1;
	hldc_rx_pos = hldc_rx_start;
	
	if (hldc_frame_len<2) {
	    /* Too short - drop */
	    hldc_frame_len = 0;
	    return 0;
	}
	
	/* See if the header is there */
	if ((hldc_rx_start[0]==HLDC_ALL_STATIONS)&&
	    (hldc_rx_start[1]==HLDC_PPP_CONTROL)) {
	    /* Yes, so skip it */
	    
	    *ret = &hldc_rx_start[2];
	    len = hldc_frame_len - 4;
	    if (hldc_fcs16_update(0x3DE3, *ret, hldc_frame_len - 2)!=HLDC_FCS_GOOD) {
#ifdef DEBUG_HLDC_POLL
		printf("hldc_poll: CRC failed [1].\n");
#endif
		hldc_frame_len = 0;
		/* CRC failed */
		return 0;
	    }
	} else { /* No header */
	    *ret = hldc_rx_start;
	    len = hldc_frame_len - 2;
#ifdef DEBUG_HLDC_POLL
	    printf("hldc_poll: No address field (%02X%02X).\n", ret[0], ret[1]);
#endif
	    /* Check the CRC */
	    if (hldc_fcs16_update(HLDC_FCS_SEED, *ret, (len) + 2)!=HLDC_FCS_GOOD) {
#ifdef DEBUG_HLDC_POLL
		printf("hldc_poll: CRC failed.\n");
#endif
		hldc_frame_len = 0;
		/* CRC failed */
		return 0;
	    }
	}
#ifdef DEBUG_HLDC_POLL
	printf("hldc_poll: Valid frame.\n");
#endif
	hldc_frame_len = 0;

	return len;
    }
    return 0; /* flag == 0 */
}


