/*
 * Copyright (c) 1999-2002 Dominic Morris
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Dominic Morris.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the ZSock TCP/IP stack.
 *
 * $Id: nfuncs.h,v 1.3 2002-05-13 20:00:48 dom Exp $
 *
 */


#ifndef __FUNCS_H__

/* config_read.c */
extern int      config_hostname(u8_t *buf, size_t len);
extern int      config_hostaddr(u8_t *buf, size_t len);
extern int      config_dns();
extern void     config_device();

/* device.c */
extern void     device_report();
extern struct pktdrive *device_insert(char *devfile);
extern int      device_load(char *name);
extern int      device_attach(struct pktdrive *ptr);
extern int      device_check(struct pktdrive *ptr);



/* icmp.c */
extern void    *icmp_register(int (*fn)());
extern void     icmp_deregister();
extern void     icmp_send(ip_header_t *buf,u8_t type,u8_t code,u32_t *unused);

extern u32_t    icmp_ping_pkt(ipaddr_t ipaddr,u32_t *unused,u16_t len);
extern void     icmp_handler(void *buf,u16_t len);




/* ip.c */
extern void     PktRcvIP(void *buf,u16_t len);
extern void     SendPacket(ip_header_t *buf,u16_t len);



/* loopback.c */
extern int       loopback_init();
extern void      loopback_send(void *buf,u16_t len);
extern void      loopback_recv();

/* net_utils.h */
extern u8_t     *inet_ntoa_i(ipaddr_t in,char *buffer);
extern ipaddr_t __FASTCALL__ inet_addr_i(u8_t *cp);


/* pbuf.c */
extern void       pkt_free(void *buf);
extern void      *pkt_alloc(u16_t size);

/* pppset.c */
extern ipaddr_t   GetHostAddr(void);
extern u8_t      *GetDomain(u8_t *buffer);
extern ipaddr_t   SetHostAddr(ipaddr_t newip);
extern size_t     SetNameServers(ipaddr_t ns1, ipaddr_t ns2);

/* socket_int.c */
extern int        sock_write_i(TCPSOCKET *s,u8_t *dp,u16_t len);
extern int        sock_puts_i(TCPSOCKET *s,u8_t *dp);
extern void       sock_close_i(TCPSOCKET *s);
extern int        sock_dataready_i(TCPSOCKET *s);
extern int        sock_read_i( TCPSOCKET *s, u8_t *dp, u16_t len, u8_t flags);
extern void       kill_socket(tcpport_t myport,TCPSOCKET *s);

/* tcp.c */
extern void       tcp_init();
extern int        tcp_recvresize(TCPSOCKET *s,u16_t size);
extern int        tcp_sendresize(TCPSOCKET *s,u16_t size);
extern TCPSOCKET *tcp_open(ipaddr_t ipdest,
			   tcpport_t lport,tcpport_t rport,
			   int (*datahandler)(),u8_t type);
extern TCPSOCKET *tcp_listen(ipaddr_t ipaddr,
			     tcpport_t lport,
			     int (*datahandler)(),
			     u8_t type, u16_t timeout);


extern void       tcp_close(TCPSOCKET *);
extern void       tcp_abort(TCPSOCKET *);
extern u16_t      tcp_write(TCPSOCKET *, void *buf, u16_t len);
extern u16_t      tcp_read(TCPSOCKET *, void *buf, u16_t len,u8_t flags);
extern void       tcp_flush(TCPSOCKET *s);
extern void       tcp_retransmit();
extern void       tcp_shutdown(TCPSOCKET *);
extern void       tcp_cancel(void *buf, int action, ipaddr_t *new);

extern void       tcp_handler(ip_header_t *ip,u16_t length);


/* tcp_int.c */
extern void       service_registertcp();
extern void       service_registerudp();


/* udp.c */
extern void       udp_init();
extern UDPSOCKET *udp_open(ipaddr_t ipdest,
			   tcpport_t lport,tcpport_t dport,
			   int (*datahandler)(),u8_t type);
extern void       udp_close(UDPSOCKET *ds);
extern void       udp_handler(ip_header_t *ip,u16_t length);
extern int        udp_read(UDPSOCKET *s, void *dp, int len,u8_t flags);
extern int        udp_write(UDPSOCKET *s, void *dp, int len);
extern void       udp_check_timeouts();

extern int        get_uniqport(void *t,tcpport_t *pick); /* FIXME */
extern int        CheckPort(void *t, tcpport_t src);  /* FIXME */

/* udp_dom.c */
extern ipaddr_t   resolve_i(char *);
extern int        reverse_addr_lookup_i(ipaddr_t, char *);

/* z80.c */
extern int __FASTCALL__  tcp_dataoffset(tcp_header_t *tp);
extern void              icmp_fill_ping(char *ptr, u16_t len);
extern u16_t             inet_cksum_pseudo(ip_header_t *ip,void *tcp,u8_t protocol,u16_t length);
extern u16_t             ip_check_cksum(ip_header_t *buf);
extern void __FASTCALL__ inet_cksum_set(ip_header_t *buf);
extern u16_t             inet_cksum(void *buf,u16_t len);


/* z88.c */
extern u32_t    current_time();
extern u32_t    set_ttimeout(int secs);
extern u32_t    set_timeout(int tensms);
extern int      chk_timeout(u32_t time);
extern void     SetTIMEOUTtime(TCPSOCKET *s);
extern void     SetLONGtimeout(TCPSOCKET *s);
extern u32_t    GetSeqNum();


#endif /* __FUNCS_H__ */
