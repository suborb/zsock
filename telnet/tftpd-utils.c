/* Non package calls for the tftpd */

#include "tftpd.h"

/* Longest message is 34 include \0 */

struct errmsg {
        int     e_code;
        char    *e_msg;
} errmsgs[] = {
        { EUNDEF,       "Undefined error code" },
        { ENOTFOUND,    "File not found" },
        { EACCESS,      "Access violation" },
        { ENOSPACE,     "Disk full or allocation exceeded" },
        { EBADOP,       "Illegal TFTP operation" },
        { EBADID,       "Unknown transfer ID" },
        { EEXISTS,      "File already exists" },
        { ENOUSER,      "No such user" },        /* Not relevent */
        { -1, 0 }
};



/* Send block n (in 512 byte chunks) */

int SendBlock(SOCKET *s, u16_t num)
{
        long    pos;
        int     len;
        int     fd;
        struct tftpinfo *blk;
        struct tftphdr *tp;

        blk=sock_getptr(s);
        if (blk->fd == NULL ) {
                if ( (fd=open(blk->filename,O_RDONLY,0)) == EOF ) {
                        nak(s,ENOTFOUND);
                        tftp_close(s);
                        return(0);
                }
                blk->fd=fd;
        }
/* Bit of code to avoid seeking....if rexmitting block
 * then it's in the buffer, we are always called in order..
 */
        tp=blk->buf;
        if ( (num+1) != blk->block ) {
                pos=(long)num << 9;
                lseek(pos,SEEK_SET,blk->fd);
                len=read(blk->fd,tp->th_data,SEGSIZE);
                blk->lastsize=len;
                blk->block=++num;
                
        }
        tp->th_opcode=htons(DATA);
        tp->th_block=htons(num);


        // printk("tftpd send %s block %d\n",blk->filename,num);

        sock_write(s,tp,len+TFTPHDRLEN);
}

/* Write block to file, append to end of file, if failure return 0 *
 * Length to be written is len, from address dp
 */

int WriteBlock(blk,dp,len,block)
        struct tftpinfo blk;
        u8_t    *dp;
        u16_t   len;
        u16_t   block;
{
        lseek(blk->fd,0L,SEEK_END); 
        len=write(blk->fd,dp,len);
        // printk("tftpd recv %s block %d\n",blk->filename,block);
        return(len);
}


int CreateFile(struct tftpinfo *blk)
{
        int     fd;
        if ( (fd=creat(blk->filename,0) ) == EOF ) 
                return 0;
        blk->fd=fd;
        return(1);
}


/* Send an ack in for block number num */

void SendACK(SOCKET *n,int num)
{
        struct tftphdr tp;
        tp.th_opcode=htons(ACK);
        tp.th_block=htons(num);
        // printk("tftpd: Sending ACK to block %d\n",num);
        sock_write(n,tp,TFTPHDRLEN);
}


/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 *
 * This is an horrendous kludge! Do not do this!!!
 */
void pnak(ip,up,error)
        struct ip_header *ip;
        struct udp_header *up;
        int error;
{
        UDPSOCKET       s;

/* Get the information we need into a fake socket */
        s.myaddr=ip->dest;
        s.myport=up->dstport;
        s.hisport=up->srcport;
        s.hisaddr=ip->source;
        s.sockmode=0;
        nak(s,error);
}


void nak(s,error)
        SOCKET *s;
        int     error;
{
        register struct tftphdr *tp;
        int length;
        register struct errmsg *pe;

        tp=(struct tftphdr *)tcp_calloc(50,1);

        tp->th_opcode = htons(ERROR);
        tp->th_code = htons(error);

        for (pe = errmsgs; pe->e_code != -1 ; pe++) {
                if (pe->e_code == error)
                        goto founderr;
        }
        /* Fail safe if we pop out without error, prevents
         * corruption though it shouldn't do it...
         */
        pe=errmsgs;
founderr:
        strcpy(tp->th_msg, pe->e_msg);
        length=strlen(pe->e_msg);
        length += 5;
        // printk("tftpd Sending NAK %d %s\n",error,pe->e_msg);
        sock_write(s,tp,length);
        tcp_free(tp);
}

/*
 * Close a tftp "connection" free up our buffer and also
 * close our file..v important to do this, or OZ gets
 * into a little bit of a strop
 */

tftp_close(SOCKET *s)
{
        struct tftpinfo *blk;   

        blk=sock_getptr(s);
        if (blk && blk->buf ) tcp_free (blk->buf);
        if (blk && blk->fd )
                        close(blk->fd);
        sock_close(s);
}

