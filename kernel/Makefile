
CFLAGS	 = -DNETSTAT -g
INCLUDES = -I. 
LIBS	 = -lutil
OBJS 	 = unixmain.o pbuf.o icmp.o ping.o pppset.o socket.o socket_int.o \
	tcp.o tcp_int.o udp.o udp_dom.o generic.o loopback.o time.o \
	setup.o ip.o net_utils.o pty.o slip_gen.o device.o httpd.o 

%.o: %.c zsock.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $<



all:	$(OBJS)
	$(CC) $(LDFLAGS) -o zsock $(CFLAGS) $(OBJS) $(LIBS)
	
clean:
	rm -f *.o *~ zsock
