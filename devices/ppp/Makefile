
CFLAGS	 = -g -DLINUX -DPAP
INCLUDES = -I.
LIBS	 = 
OBJS 	 = hldc.o lcp_options.o pap.o ppp.o utils.o serial.o

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

all: $(OBJS)
	$(CC) $(LDFLAGS) -o zsock $(CFLAGS) $(OBJS) $(LIBS)

libppp.a: $(OBJS)
	ar cru libppp.a $(OBJS)
	ranlib libppp.a

	
clean:
	rm -f *.o *~ 
