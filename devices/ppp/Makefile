CC	 = zcc
TARGET   = +z88
CFLAGS	 = -vn -DPAP -O3 -startup=4 -preserve
INCLUDES = -I.  -I../../kernel -I../../include
LIBS	 = -lnetdev
LDFLAGS	 = 

SOURCES = ppp.c hldc.c lcp_options.c utils.c pap.c serial.c

OBJS	= $(SOURCES:.c=.o)

all:	ppp.dev

ppp.dev: $(OBJS)
	$(CC) $(TARGET) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ $(LIBS) 

%.o: %.c
	$(CC) $(TARGET) $(CFLAGS) $(INCLUDES) -c $<


clean:
	rm -f *.opt $(OBJS) *~ ppp.dev zcc_opt.def
