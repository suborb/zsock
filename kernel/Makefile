
CFLAGS	 = -DNETSTAT -g -DLIBRARY
INCLUDES = -I.
LIBS	 = -lutil
OBJS 	 = unixmain.o pbuf.o icmp.o ping.o pppset.o socket.o socket_int.o \
	tcp.o tcp_int.o udp.o udp_dom.o generic.o loopback.o time.o \
	setup.o ip.o net_utils.o pty.o device.o httpd.o telnet.o
DEV	= slip_gen.o
#DEV	= ../devices/ppp/hldc.o ../devices/ppp/pap.o ../devices/ppp/ppp.o \
	../devices/ppp/serial.o ../devices/ppp/utils.o ../devices/ppp/lcp_options.o

%.o: %.c zsock.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

libzsock.a: $(OBJS) $(DEV)
	ar cru libzsock.a $(OBJS) $(DEV)
	ranlib libzsock.a


all:	$(OBJS)
	$(CC) $(LDFLAGS) -o zsock $(CFLAGS) $(OBJS) $(DEV) $(LIBS)
	
clean:
	rm -f *.o *~ zsock
