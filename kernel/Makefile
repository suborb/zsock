
CFLAGS	 = -DNETSTAT -g -DLIBRARY
INCLUDES = -I. -I../include
LIBS	 = -lutil
OBJS 	 = unixmain.o pbuf.o icmp.o ping.o pppset.o socket.o socket_int.o \
	tcp.o tcp_int.o udp.o udp_dom.o generic.o loopback.o time.o \
	setup.o ip.o net_utils.o pty.o device.o httpd.o telnet.o
DEV     = ../devices/ppp/libppp.a

%.o: %.c zsock.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

all:	$(OBJS)
	$(CC) $(LDFLAGS) -o zsock $(CFLAGS) $(OBJS) $(DEV) $(LIBS)

libzsock.a: $(OBJS) $(DEV)
	ar cru libzsock.a $(OBJS) $(DEV)
	ranlib libzsock.a

	
clean:
	rm -f *.o *~ zsock
