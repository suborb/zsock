/*
 * Packet De-Fragmentaion code for WATTCP
 * Written by and COPYRIGHT (c)1993 to Quentin Smart
 *                               smart@actrix.gen.nz
 * all rights reserved.
 *
 *   This software is distributed in the hope that it will be useful,
 *   but without any warranty; without even the implied warranty of
 *   merchantability or fitness for a particular purpose.
 *
 *   You may freely distribute this source code, but if distributed for
 *   financial gain then only executables derived from the source may be
 *   sold.
 *
 * Murf = Murf@perftech.com
 * other fragfix = mdurkin@tsoft.net
 *
 * Based on RFC815
 *
 * Adapted to ZSock 10/2/2001
 */

#include "zsock.h"

/* Murf's code structuring macros */   /* add 94.09.16 */
#define LOOP       for (;;)
#define LOOPASSERT( c ) if (!(c)) break

/* Murf's macro to get a structure address from a member address */
#define structure(S,m,p) ((S *)( (BYTE *)(p) - (BYTE *) &((S *)0)->m ))
/*
#define ANIMATE
 */

#define MAXBUFS         5       /* maximum number of Ethernet buffers */
#define MAXFRAGS        MAXBUFS-1
#define FRAGHOLDTIME    15       /* 15 secs to hold before discarding */
#define INFINITY        30000
#define IP_HEADER_SIZE  20
#define IP_MF           0x0020  // More fragment in INTEL form


typedef struct {
	LWORD    source;
        BYTE        proto;         // use proto now
	WORD        identification;
} fragkey;

typedef struct hd {
	struct hd   * next;
	int           start;
	int           end;
} hole_descr;

typedef struct fh {
	BYTE         used;           // this position in table in use
	fragkey      key;
	hole_descr  *hole_first;
	LWORD     timer;
	struct ip_header	*ip;
	BYTE        *data_offset;
} fraghdr;

static fraghdr fraglist[MAXFRAGS];


int active_frags = 0;
extern WORD _pktipofs;  /* offset from header to start of pkt */


void frag_init()
{
	memset(fraglist,0,sizeof(fraglist));
}


/* Fragment is called if the frag section of the IP header is not zero and DF bit not set */
BYTE *fragment( struct ip_header * ip )
{
	fraghdr    *my_frag;
	hole_descr *hole = NULL;
	hole_descr *prev_hole = NULL;

	fragkey     key;
	int         found = 0;
int        got_hole = 0;
int         data_start;
int         data_end;
int         data_length;
int         temp,i;
int         more_frags;   // Set to true if this is the last frag
/*BYTE     * buffer; */

// Should check that the packet is actually valid and do a checksum on it.
// Assemble key
key.proto = ip->proto;     // use proto now
key.source=ip->source;
key.identification = ip->identification;

// Check if we have a match

for (i=0;i<MAXFRAGS && !found;i++)
    if (fraglist[i].used && !memcmp(&key,&fraglist[i].key,sizeof(fragkey)))
       {
	  found = TRUE;
	  my_frag = &fraglist[i];
       }

if (!found && active_frags == MAXFRAGS) {
#ifdef ANIMATE
   printf( "NO_BUFS\n" );/*DEBUG STUFF*/
#endif
   // Can't handle any new frags, biff packet so that we can continue
   // Could do direct!
   pkt_buf_release((char*)ip);
   return(NULL);  // We can't handle any new frags!
   }
						 // Should biff packet?

// Calc where data should go

// fragfix - next line replaces... no more bitfields
   data_start  = htons(ip->frags) << 3; //* 8
   data_length = htons(ip->length)-in_GetHdrlenBytes(ip);
   data_end    = data_start + data_length - 1;  // Murf 94.09.16
   more_frags = ip->frags & IP_MF;
#ifdef ANIMATE
   printf( "Data=%d..%d", data_start, data_end );/*DEBUG STUFF*/
#endif


if (!found)
 {
   // Mark as used
   *((BYTE *)ip - (2 + _pktipofs)) = 2;
   // Find first empty slot
   for (i=0;i < MAXFRAGS && fraglist[i].used;i++);
   my_frag = &fraglist[i];
   // mark as used
   my_frag->used = 1;
   // inc active frags counter
   active_frags++;
   // Setup frag header data, first packet
   memcpy(&my_frag->key,&key,sizeof(key));
   my_frag->timer  = set_timeout(max(FRAGHOLDTIME,ip->ttl));
   my_frag->ip = ip;
   // Set pointers to beinging of IP packet data
   my_frag->data_offset = (BYTE *)my_frag->ip + in_GetHdrlenBytes(ip);
   // Setup initial hole table
   if (data_start) // i.e. not Zero
    {
      memcpy(my_frag->data_offset + data_start,(BYTE *)ip+in_GetHdrlenBytes(ip),data_length);
      // Bracket beginning of data
      hole = my_frag->hole_first = (hole_descr *)my_frag->data_offset;
      hole->start = 0;
      hole->end = data_start-1;
      if (more_frags) {
         // data_start was missing in next line - Murf 94.09.16
         hole->next = (hole_descr *)(my_frag->data_offset + data_start + data_length + 1);
	 hole = hole->next;
	 }
      else
	{
	 hole = my_frag->hole_first->next = NULL;
	 // Adjust length
	 ip->length = htons(data_end + in_GetHdrlenBytes(ip));
	}
    }
   else
    {
     // Setup
     hole = my_frag->hole_first = (hole_descr*)(my_frag->data_offset + data_length + 1);
    }
   // Bracket end
   if (hole) {
     // data_start was missing in next line - Murf 94.09.16
     hole->start = data_start + data_length;// + 1;
     hole->end = INFINITY;
     hole->next = NULL;
    }
#ifdef ANIMATE
   printf( " new\n" );/*DEBUG STUFF*/
#endif
   return NULL; // Go back for more!
 } // End !found
// Adjust length
   if (!more_frags)
      my_frag->ip->length = htons(data_end + 1 + in_GetHdrlenBytes(ip)); // Murf 94.09.16

// Hole handling
   hole = structure( hole_descr, next, &my_frag->hole_first ); // Murf 94.09.16

   LOOP {                                                      // Murf 94.09.16
      prev_hole = hole;                                        // Murf 94.09.16
      hole = hole->next;                                       // Murf 94.09.16
      LOOPASSERT (hole);                                       // Murf 94.09.16
#ifdef ANIMATE
      printf( " hole=%d..%d(%d)", hole->start, hole->end, hole->next );/*DEBUG STUFF*/
#endif
      if (!(data_start > hole->end) && !(data_end < hole->start)) {
         // We've found the spot
         // Mark as got.
         got_hole =1;
         // Find where to insert
         // Check is there a hole before the new frag
         temp = hole->end;   // Pick up old hole end for later;

         if (data_start > hole->start) {
            hole->end = data_start-1;
            prev_hole = hole;  // We have a new prev
#ifdef ANIMATE
            printf( "->%d..%d(%d)", hole->start, hole->end, hole->next );/*DEBUG STUFF*/
#endif
            }
          else {
            // No, delete current hole
#ifdef ANIMATE
            printf( " del" );/* DEBUG STUFF*/
#endif
            prev_hole->next = hole->next;
            hole = prev_hole;     // Leave hole valid - Murf 94.09.16
            }

         // Is there a hole after the current fragment
         // Only if we're not last and more to come
         if (data_end < temp && more_frags) {   // fragfix - Murf 94.09.14
            hole = (hole_descr *)(data_end + 1 + my_frag->data_offset);
            hole->start = data_end+1;
            hole->end = temp;
            hole->next = prev_hole->next;
            prev_hole->next = hole;
#ifdef ANIMATE
            printf( ",%d..%d(%d)", hole->start, hole->end, hole->next );/*DEBUG STUFF*/
#endif
            }
         }
      }
       // Thats all setup so copy in the data
       if (got_hole)
	   memcpy(my_frag->data_offset + data_start,(BYTE *)ip+in_GetHdrlenBytes(ip),data_length);
       // And release the buffer;
       pkt_buf_release((char *)ip);
       // Now do we have all the parts?
       if (!my_frag->hole_first)
	{
#ifdef ANIMATE
      printf( " COMPLETE\n" );/*DEBUG STUFF*/
#endif
	  my_frag->used = 0;
	  active_frags--;
	  // Redo checksum as we've changed the length in the header
	  my_frag->ip->checksum = 0; // Zero
	  my_frag->ip->checksum = ~ checksum( my_frag->ip, sizeof( struct ip_header ));
	  return((BYTE *)my_frag->ip - _pktipofs);
	}
#ifdef ANIMATE
   printf( " MORE_TO_COME\n" );/*DEBUG STUFF*/
#endif
return NULL;
}

void timeout_frags( void )
{
int i;

for (i=0;i<MAXFRAGS;i++)
    if (fraglist[i].used)
       if (chk_timeout(fraglist[i].timer))
	 {
#ifdef ANIMATE
	  printf( "BUF timed out\n" );/*DEBUG STUFF*/
#endif
	  fraglist[i].used = 0;
	  active_frags--;
	  pkt_buf_release((char *)fraglist[i].ip);
	 }
}

