/*
 *	Some function defns for ZSock
 */


extern void FillHeader(struct ip_header *,u8_t,u8_t);
extern LWORD PkPing(LWORD ipaddr, LWORD *unused,WORD len);
extern LWORD current();
extern LWORD set_ttimeout(int);
extern LWORD set_timeout(int);
extern int chk_timeout(LWORD);
extern void tcp_unwind(TCPSOCKET *);



/* Convert dotted string to network address 
 * CAUTION: RETURNS 0 ON FAILURE!!!
 */
extern ipaddr_t  __FASTCALL__ inet_addr_i(u8_t *cp);

/* Convert network address to dotted string, returns where the string is */
/* First one assumes base 10 and full complement of digits */
extern u8_t *inet_ntoa_i(ipaddr_t in, u8_t *b);

/* full version, may not make it into the final cut! */
extern u8_t *inet_ntoa_full(ipaddr_t in, u8_t *b);

/* Config stuff.. */

extern gethostaddr_i(u8_t *, size_t);
extern gethostname_i(u8_t *, size_t);
extern ReadDomConfig(void);
/* Rip removes LF & CR - it's in udp_dom2.c */
extern char *rip(char *);
/* Memory allocation stuff - malloc2.c */
extern void *malloc(int);
extern void *calloc(int, int);
extern void free(void *);
//extern int getlarge(void);
//extern int getfree(void);
//extern void heapinit(int);

extern TCPSOCKET __SHARED__ *tcp_listen(ipaddr_t, tcpport_t, int *, u8_t, int);
extern UDPSOCKET __SHARED__ *udp_open(ipaddr_t, tcpport_t, tcpport_t, int *, u8_t);
extern TCPSOCKET __SHARED__ *tcp_open(ipaddr_t, tcpport_t, tcpport_t, int *, u8_t);



/* z80.c */
extern int __FASTCALL__  tcp_dataoffset(tcp_header_t *tp);
extern void              icmp_fill_ping(char *ptr, u16_t len);
extern u16_t             inet_cksum_pseudo(void *buf,u8_t protocol,u16_t length);
extern u16_t             ip_check_cksum(ip_header_t *buf);
extern void __FASTCALL__ inet_cksum_set(ip_header_t *buf);
extern u16_t             inet_cksum(void *buf,u16_t len);



